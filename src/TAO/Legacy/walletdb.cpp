/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include "walletdb.h"
#include "wallet.h"
#include <boost/filesystem.hpp>

using namespace std;


namespace Wallet
{

    static uint64_t nAccountingEntryNumber = 0;

    extern CCriticalSection cs_db;
    extern map<string, int> mapFileUseCount;
    extern void CloseDb(const string& strFile);

    //
    // CWalletDB
    //

    bool CWalletDB::WriteName(const string& strAddress, const string& strName)
    {
        nWalletDBUpdated++;
        return Write(make_pair(string("name"), strAddress), strName);
    }

    bool CWalletDB::EraseName(const string& strAddress)
    {
        // This should only be used for sending addresses, never for receiving addresses,
        // receiving addresses must always have an address book entry if they're not change return.
        nWalletDBUpdated++;
        return Erase(make_pair(string("name"), strAddress));
    }

    bool CWalletDB::ReadAccount(const string& strAccount, CAccount& account)
    {
        account.SetNull();
        return Read(make_pair(string("acc"), strAccount), account);
    }

    bool CWalletDB::WriteAccount(const string& strAccount, const CAccount& account)
    {
        return Write(make_pair(string("acc"), strAccount), account);
    }

    bool CWalletDB::WriteAccountingEntry(const CAccountingEntry& acentry)
    {
        return Write(std::make_tuple(string("acentry"), acentry.strAccount, ++nAccountingEntryNumber), acentry);
    }

    int64_t CWalletDB::GetAccountCreditDebit(const string& strAccount)
    {
        list<CAccountingEntry> entries;
        ListAccountCreditDebit(strAccount, entries);

        int64_t nCreditDebit = 0;
        BOOST_FOREACH (const CAccountingEntry& entry, entries)
            nCreditDebit += entry.nCreditDebit;

        return nCreditDebit;
    }

    void CWalletDB::ListAccountCreditDebit(const string& strAccount, list<CAccountingEntry>& entries)
    {
        bool fAllAccounts = (strAccount == "*");

        Dbc* pcursor = GetCursor();
        if (!pcursor)
            throw runtime_error("CWalletDB::ListAccountCreditDebit() : cannot create DB cursor");
        uint32_t fFlags = DB_SET_RANGE;
        loop() {
            // Read next record
            CDataStream ssKey(SER_DISK, DATABASE_VERSION);
            if (fFlags == DB_SET_RANGE)
                ssKey << std::::make_tuple(string("acentry"), (fAllAccounts? std::string("") : strAccount), uint64_t(0));
            CDataStream ssValue(SER_DISK, DATABASE_VERSION);
            int ret = ReadAtCursor(pcursor, ssKey, ssValue, fFlags);
            fFlags = DB_NEXT;
            if (ret == DB_NOTFOUND)
                break;
            else if (ret != 0)
            {
                pcursor->close();
                throw runtime_error("CWalletDB::ListAccountCreditDebit() : error scanning DB");
            }

            // Unserialize
            string strType;
            ssKey >> strType;
            if (strType != "acentry")
                break;
            CAccountingEntry acentry;
            ssKey >> acentry.strAccount;
            if (!fAllAccounts && acentry.strAccount != strAccount)
                break;

            ssValue >> acentry;
            entries.push_back(acentry);
        }

        pcursor->close();
    }


    int CWalletDB::LoadWallet(CWallet* pwallet)
    {
        pwallet->vchDefaultKey.clear();
        int nFileVersion = 0;
        std::vector<uint512_t> vWalletUpgrade;
        std::vector<uint512_t> vWalletRemove;

        bool fIsEncrypted = false;

        //// todo: shouldn't we catch exceptions and try to recover and continue?
        {
            LOCK(pwallet->cs_wallet);
            int nMinVersion = 0;
            if (Read((string)"minversion", nMinVersion))
            {
                if (nMinVersion > DATABASE_VERSION)
                    return DB_TOO_NEW;
                pwallet->LoadMinVersion(nMinVersion);
            }

            // Get cursor
            Dbc* pcursor = GetCursor();
            if (!pcursor)
            {
                printf("Error getting wallet database cursor\n");
                return DB_CORRUPT;
            }

            loop() {
                // Read next record
                CDataStream ssKey(SER_DISK, DATABASE_VERSION);
                CDataStream ssValue(SER_DISK, DATABASE_VERSION);
                int ret = ReadAtCursor(pcursor, ssKey, ssValue);
                if (ret == DB_NOTFOUND)
                    break;
                else if (ret != 0)
                {
                    printf("Error reading next record from wallet database\n");
                    return DB_CORRUPT;
                }

                // Unserialize
                // Taking advantage of the fact that pair serialization
                // is just the two items serialized one after the other
                string strType;
                ssKey >> strType;
                if (strType == "name")
                {
                    string strAddress;
                    ssKey >> strAddress;
                    ssValue >> pwallet->mapAddressBook[strAddress];
                }
                else if (strType == "tx")
                {
                    uint512_t hash;
                    ssKey >> hash;
                    CWalletTx& wtx = pwallet->mapWallet[hash];
                    ssValue >> wtx;

                    if(GetBoolArg("-walletclean", false)) {
                        vWalletRemove.push_back(hash);
                    }
                    else if (wtx.GetHash() != hash) {
                        printf("Error in wallet.dat, hash mismatch. Removing Transaction from wallet map. Run the rescan command to restore.\n");

                        vWalletRemove.push_back(hash);
                    }
                    else
                        wtx.BindWallet(pwallet);

                    // Undo serialize changes in 31600
                    if (31404 <= wtx.fTimeReceivedIsTxTime && wtx.fTimeReceivedIsTxTime <= 31703)
                    {
                        if (!ssValue.empty())
                        {
                            char fTmp;
                            char fUnused;
                            ssValue >> fTmp >> fUnused >> wtx.strFromAccount;
                            printf("LoadWallet() upgrading tx ver=%d %d '%s' %s\n", wtx.fTimeReceivedIsTxTime, fTmp, wtx.strFromAccount.c_str(), hash.ToString().c_str());
                            wtx.fTimeReceivedIsTxTime = fTmp;
                        }
                        else
                        {
                            printf("LoadWallet() repairing tx ver=%d %s\n", wtx.fTimeReceivedIsTxTime, hash.ToString().c_str());
                            wtx.fTimeReceivedIsTxTime = 0;
                        }
                        vWalletUpgrade.push_back(hash);
                    }

                    //// debug print
                    //printf("LoadWallet  %s\n", wtx.GetHash().ToString().c_str());
                    //printf(" %12"PRI64d"  %s  %s  %s\n",
                    //    wtx.vout[0].nValue,
                    //    DateTimeStrFormat(wtx.GetBlockTime()).c_str(),
                    //    wtx.hashBlock.ToString().substr(0,20).c_str(),
                    //    wtx.mapValue["message"].c_str());
                }
                else if (strType == "acentry")
                {
                    string strAccount;
                    ssKey >> strAccount;
                    uint64_t nNumber;
                    ssKey >> nNumber;
                    if (nNumber > nAccountingEntryNumber)
                        nAccountingEntryNumber = nNumber;
                }
                else if (strType == "key" || strType == "wkey")
                {
                    std::vector<uint8_t> vchPubKey;
                    ssKey >> vchPubKey;
                    ECKey key;
                    if (strType == "key")
                    {
                        CPrivKey pkey;
                        ssValue >> pkey;
                        key.SetPubKey(vchPubKey);
                        key.SetPrivKey(pkey);
                        if (key.GetPubKey() != vchPubKey)
                        {
                            printf("Error reading wallet database: CPrivKey pubkey inconsistency\n");
                            return DB_CORRUPT;
                        }
                        if (!key.IsValid())
                        {
                            printf("Error reading wallet database: invalid CPrivKey\n");
                            return DB_CORRUPT;
                        }
                    }
                    else
                    {
                        CWalletKey wkey;
                        ssValue >> wkey;
                        key.SetPubKey(vchPubKey);
                        key.SetPrivKey(wkey.vchPrivKey);
                        if (key.GetPubKey() != vchPubKey)
                        {
                            printf("Error reading wallet database: CWalletKey pubkey inconsistency\n");
                            return DB_CORRUPT;
                        }
                        if (!key.IsValid())
                        {
                            printf("Error reading wallet database: invalid CWalletKey\n");
                            return DB_CORRUPT;
                        }
                    }
                    if (!pwallet->LoadKey(key))
                    {
                        printf("Error reading wallet database: LoadKey failed\n");
                        return DB_CORRUPT;
                    }
                }
                else if (strType == "mkey")
                {
                    uint32_t nID;
                    ssKey >> nID;
                    CMasterKey kMasterKey;
                    ssValue >> kMasterKey;
                    if(pwallet->mapMasterKeys.count(nID) != 0)
                    {
                        printf("Error reading wallet database: duplicate CMasterKey id %u\n", nID);
                        return DB_CORRUPT;
                    }
                    pwallet->mapMasterKeys[nID] = kMasterKey;
                    if (pwallet->nMasterKeyMaxID < nID)
                        pwallet->nMasterKeyMaxID = nID;
                }
                else if (strType == "ckey")
                {
                    std::vector<uint8_t> vchPubKey;
                    ssKey >> vchPubKey;
                    std::vector<uint8_t> vchPrivKey;
                    ssValue >> vchPrivKey;
                    if (!pwallet->LoadCryptedKey(vchPubKey, vchPrivKey))
                    {
                        printf("Error reading wallet database: LoadCryptedKey failed\n");
                        return DB_CORRUPT;
                    }
                    fIsEncrypted = true;
                }
                else if (strType == "defaultkey")
                {
                    ssValue >> pwallet->vchDefaultKey;
                }
                else if (strType == "pool")
                {
                    int64_t nIndex;
                    ssKey >> nIndex;
                    pwallet->setKeyPool.insert(nIndex);
                }
                else if (strType == "version")
                {
                    ssValue >> nFileVersion;
                    if (nFileVersion == 10300)
                        nFileVersion = 300;
                }
                else if (strType == "cscript")
                {
                    uint256_t hash;
                    ssKey >> hash;
                    CScript script;
                    ssValue >> script;
                    if (!pwallet->LoadCScript(script))
                    {
                        printf("Error reading wallet database: LoadCScript failed\n");
                        return DB_CORRUPT;
                    }
                }
            }
            pcursor->close();
        }

        for(uint512_t hash : vWalletUpgrade)
            WriteTx(hash, pwallet->mapWallet[hash]);


        if(vWalletRemove.size() > 0) {
            for(uint512_t hash : vWalletRemove) {
                EraseTx(hash);
                pwallet->mapWallet.erase(hash);

                printf("Erasing Transaction at hash %s\n", hash.ToString().c_str());
            }
        }

        printf("nFileVersion = %d\n", nFileVersion);


        // Rewrite encrypted wallets of versions 0.4.0 and 0.5.0rc:
        if (fIsEncrypted && (nFileVersion == 40000 || nFileVersion == 50000))
            return DB_NEED_REWRITE;

        if (nFileVersion < DATABASE_VERSION) // Update
            WriteVersion(DATABASE_VERSION);

        return DB_LOAD_OK;
    }

    void ThreadFlushWalletDB(void* parg)
    {
        const string& strFile = ((const string*)parg)[0];
        static bool fOneThread;
        if (fOneThread)
            return;
        fOneThread = true;
        if (!GetBoolArg("-flushwallet", true))
            return;

        uint32_t nLastSeen = nWalletDBUpdated;
        uint32_t nLastFlushed = nWalletDBUpdated;
        int64_t nLastWalletUpdate = GetUnifiedTimestamp();
        while (!fShutdown)
        {
            Sleep(500);

            if (nLastSeen != nWalletDBUpdated)
            {
                nLastSeen = nWalletDBUpdated;
                nLastWalletUpdate = GetUnifiedTimestamp();
            }

            if (nLastFlushed != nWalletDBUpdated && GetUnifiedTimestamp() - nLastWalletUpdate >= 2)
            {
                TRY_LOCK(cs_db,lockDb);
                if (lockDb)
                {
                    // Don't do this if any databases are in use
                    int nRefCount = 0;
                    map<string, int>::iterator mi = mapFileUseCount.begin();
                    while (mi != mapFileUseCount.end())
                    {
                        nRefCount += (*mi).second;
                        mi++;
                    }

                    if (nRefCount == 0 && !fShutdown)
                    {
                        map<string, int>::iterator mi = mapFileUseCount.find(strFile);
                        if (mi != mapFileUseCount.end())
                        {
                            printf("%s ", DateTimeStrFormat(GetUnifiedTimestamp()).c_str());
                            printf("Flushing wallet.dat\n");
                            nLastFlushed = nWalletDBUpdated;
                            int64_t nStart = GetTimeMillis();

                            // Flush wallet.dat so it's self contained
                            CloseDb(strFile);
                            dbenv.txn_checkpoint(0, 0, 0);
                            dbenv.lsn_reset(strFile.c_str(), 0);

                            mapFileUseCount.erase(mi++);
                            printf("Flushed wallet.dat %" PRI64d "ms\n", GetTimeMillis() - nStart);
                        }
                    }
                }
            }
        }
    }

    bool BackupWallet(const CWallet& wallet, const string& strDest)
    {
        if (!wallet.fFileBacked)
            return false;
        while (!fShutdown)
        {
            {
                LOCK(cs_db);
                if (!mapFileUseCount.count(wallet.strWalletFile) || mapFileUseCount[wallet.strWalletFile] == 0)
                {
                    // Flush log data to the dat file
                    CloseDb(wallet.strWalletFile);
                    dbenv.txn_checkpoint(0, 0, 0);
                    dbenv.lsn_reset(wallet.strWalletFile.c_str(), 0);
                    mapFileUseCount.erase(wallet.strWalletFile);

                    // Copy wallet.dat
                    filesystem::path pathSrc = GetDataDir() / wallet.strWalletFile;
                    filesystem::path pathDest(strDest);
                    if (filesystem::is_directory(pathDest))
                        pathDest /= wallet.strWalletFile;

                    try {
    #if BOOST_VERSION >= 104000
                        filesystem::copy_file(pathSrc, pathDest, filesystem::copy_option::overwrite_if_exists);
    #else
                        filesystem::copy_file(pathSrc, pathDest);
    #endif
                        printf("copied wallet.dat to %s\n", pathDest.string().c_str());
                        return true;
                    } catch(const filesystem::filesystem_error &e) {
                        printf("error copying wallet.dat to %s - %s\n", pathDest.string().c_str(), e.what());
                        return false;
                    }
                }
            }
            Sleep(100);
        }
        return false;
    }

}
