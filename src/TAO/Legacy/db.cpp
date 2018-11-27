/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2018

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include "db.h"
#include "../util/util.h"
#include "../core/core.h"
#include <boost/version.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>

#include "../LLD/index.h"

#ifndef WIN32
#include "sys/stat.h"
#endif

using namespace std;
using namespace boost;


namespace Legacy
{
    uint32_t nWalletDBUpdated;

    CCriticalSection cs_db;
    static bool fDbEnvInit = false;
    bool fDetachDB = false;
    DbEnv dbenv((u_int32_t)0);
    map<string, int> mapFileUseCount;
    static map<string, Db*> mapDb;

    static void EnvShutdown()
    {
        if (!fDbEnvInit)
            return;

        fDbEnvInit = false;
        try
        {
            dbenv.close(0);
        }
        catch (const DbException& e)
        {
            debug::log(0, "EnvShutdown exception: %s (%d)\n", e.what(), e.get_errno());
        }
        DbEnv((u_int32_t)0).remove(GetDataDir().string().c_str(), 0);
    }

    class CDBInit
    {
    public:
        CDBInit()
        {
        }
        ~CDBInit()
        {
            EnvShutdown();
        }
    }
    instance_of_cdbinit;


    CDB::CDB(const char *pszFile, const char* pszMode) : pdb(NULL)
    {
        int ret;
        if (pszFile == NULL)
            return;

        fReadOnly = (!strchr(pszMode, '+') && !strchr(pszMode, 'w'));
        bool fCreate = strchr(pszMode, 'c');
        uint32_t nFlags = DB_THREAD;
        if (fCreate)
            nFlags |= DB_CREATE;

        {
            LOCK(cs_db);
            if (!fDbEnvInit)
            {
                if (fShutdown)
                    return;
                filesystem::path pathDataDir = GetDataDir();
                filesystem::path pathLogDir = pathDataDir / "database";
                filesystem::create_directory(pathLogDir);
                filesystem::path pathErrorFile = pathDataDir / "db.log";
                debug::log(0, "dbenv.open LogDir=%s ErrorFile=%s\n", pathLogDir.string().c_str(), pathErrorFile.string().c_str());

                int nDbCache = GetArg("-dbcache", 25);
                dbenv.set_lg_dir(pathLogDir.string().c_str());
                dbenv.set_cachesize(nDbCache / 1024, (nDbCache % 1024)*1048576, 1);
                dbenv.set_lg_bsize(1048576);
                dbenv.set_lg_max(10485760);
                dbenv.set_lk_max_locks(10000);
                dbenv.set_lk_max_objects(10000);
                dbenv.set_errfile(fopen(pathErrorFile.string().c_str(), "a")); /// debug
                dbenv.set_flags(DB_TXN_WRITE_NOSYNC, 1);
                dbenv.set_flags(DB_AUTO_COMMIT, 1);
                dbenv.log_set_config(DB_LOG_AUTO_REMOVE, 1);
                ret = dbenv.open(pathDataDir.string().c_str(),
                                DB_CREATE     |
                                DB_INIT_LOCK  |
                                DB_INIT_LOG   |
                                DB_INIT_MPOOL |
                                DB_INIT_TXN   |
                                DB_THREAD     |
                                DB_RECOVER,
                                S_IRUSR | S_IWUSR);
                if (ret > 0)
                    throw runtime_error(strprintf("CDB() : error %d opening database environment", ret));
                fDbEnvInit = true;
            }

            strFile = pszFile;
            ++mapFileUseCount[strFile];
            pdb = mapDb[strFile];
            if (pdb == NULL)
            {
                pdb = new Db(&dbenv, 0);

                ret = pdb->open(NULL,      // Txn pointer
                                pszFile,   // Filename
                                "main",    // Logical db name
                                DB_BTREE,  // Database type
                                nFlags,    // Flags
                                0);

                if (ret > 0)
                {
                    delete pdb;
                    pdb = NULL;
                    {
                        LOCK(cs_db);
                        --mapFileUseCount[strFile];
                    }
                    strFile = "";
                    throw runtime_error(strprintf("CDB() : can't open database file %s, error %d", pszFile, ret));
                }

                if (fCreate && !Exists(string("version")))
                {
                    bool fTmp = fReadOnly;
                    fReadOnly = false;
                    WriteVersion(DATABASE_VERSION);
                    fReadOnly = fTmp;
                }

                mapDb[strFile] = pdb;
            }
        }
    }

    void CDB::Close()
    {
        if (!pdb)
            return;
        if (!vTxn.empty())
            vTxn.front()->abort();
        vTxn.clear();
        pdb = NULL;

        // Flush database activity from memory pool to disk log
        uint32_t nMinutes = 0;
        if (fReadOnly)
            nMinutes = 1;
        if (strFile == "addr.dat")
            nMinutes = 2;
        if (strFile == "blkindex.dat")
            nMinutes = 2;
        if (strFile == "blkindex.dat" && Core::IsInitialBlockDownload())
            nMinutes = 5;

        dbenv.txn_checkpoint(nMinutes ? GetArg("-dblogsize", 100)*1024 : 0, nMinutes, 0);

        {
            LOCK(cs_db);
            --mapFileUseCount[strFile];
        }
    }

    void CloseDb(const string& strFile)
    {
        {
            LOCK(cs_db);
            if (mapDb[strFile] != NULL)
            {
                // Close the database handle
                Db* pdb = mapDb[strFile];
                pdb->close(0);
                delete pdb;
                mapDb[strFile] = NULL;
            }
        }
    }

    bool CDB::Rewrite(const string& strFile, const char* pszSkip)
    {
        while (!fShutdown)
        {
            {
                LOCK(cs_db);
                if (!mapFileUseCount.count(strFile) || mapFileUseCount[strFile] == 0)
                {
                    // Flush log data to the dat file
                    CloseDb(strFile);
                    dbenv.txn_checkpoint(0, 0, 0);
                    dbenv.lsn_reset(strFile.c_str(), 0);
                    mapFileUseCount.erase(strFile);

                    bool fSuccess = true;
                    debug::log(0, "Rewriting %s...\n", strFile.c_str());
                    string strFileRes = strFile + ".rewrite";
                    { // surround usage of db with extra {}
                        CDB db(strFile.c_str(), "r");
                        Db* pdbCopy = new Db(&dbenv, 0);

                        int ret = pdbCopy->open(NULL,                 // Txn pointer
                                                strFileRes.c_str(),   // Filename
                                                "main",    // Logical db name
                                                DB_BTREE,  // Database type
                                                DB_CREATE,    // Flags
                                                0);
                        if (ret > 0)
                        {
                            debug::log(0, "Cannot create database file %s\n", strFileRes.c_str());
                            fSuccess = false;
                        }

                        Dbc* pcursor = db.GetCursor();
                        if (pcursor)
                            while (fSuccess)
                            {
                                CDataStream ssKey(SER_DISK, DATABASE_VERSION);
                                CDataStream ssValue(SER_DISK, DATABASE_VERSION);
                                int ret = db.ReadAtCursor(pcursor, ssKey, ssValue, DB_NEXT);
                                if (ret == DB_NOTFOUND)
                                {
                                    pcursor->close();
                                    break;
                                }
                                else if (ret != 0)
                                {
                                    pcursor->close();
                                    fSuccess = false;
                                    break;
                                }
                                if (pszSkip &&
                                    strncmp(&ssKey[0], pszSkip, std::min(ssKey.size(), strlen(pszSkip))) == 0)
                                    continue;
                                if (strncmp(&ssKey[0], "\x07version", 8) == 0)
                                {
                                    // Update version:
                                    ssValue.clear();
                                    ssValue << DATABASE_VERSION;
                                }
                                Dbt datKey(&ssKey[0], ssKey.size());
                                Dbt datValue(&ssValue[0], ssValue.size());
                                int ret2 = pdbCopy->put(NULL, &datKey, &datValue, DB_NOOVERWRITE);
                                if (ret2 > 0)
                                    fSuccess = false;
                            }
                        if (fSuccess)
                        {
                            db.Close();
                            CloseDb(strFile);
                            if (pdbCopy->close(0))
                                fSuccess = false;
                            delete pdbCopy;
                        }
                    }
                    if (fSuccess)
                    {
                        Db dbA(&dbenv, 0);
                        if (dbA.remove(strFile.c_str(), NULL, 0))
                            fSuccess = false;
                        Db dbB(&dbenv, 0);
                        if (dbB.rename(strFileRes.c_str(), NULL, strFile.c_str(), 0))
                            fSuccess = false;
                    }
                    if (!fSuccess)
                        debug::log(0, "Rewriting of %s FAILED!\n", strFileRes.c_str());
                    return fSuccess;
                }
            }
            Sleep(100);
        }
        return false;
    }


    void DBFlush(bool fShutdown)
    {
        // Flush log data to the actual data file
        //  on all files that are not in use
        debug::log(0, "DBFlush(%s)%s\n", fShutdown ? "true" : "false", fDbEnvInit ? "" : " db not started");
        if (!fDbEnvInit)
            return;
        {
            LOCK(cs_db);
            map<string, int>::iterator mi = mapFileUseCount.begin();
            while (mi != mapFileUseCount.end())
            {
                string strFile = (*mi).first;
                int nRefCount = (*mi).second;
                debug::log(0, "%s refcount=%d\n", strFile.c_str(), nRefCount);
                if (nRefCount == 0)
                {
                    // Move log data to the dat file
                    CloseDb(strFile);
                    debug::log(0, "%s checkpoint\n", strFile.c_str());
                    dbenv.txn_checkpoint(0, 0, 0);
                    if ((strFile != "blkindex.dat" && strFile != "addr.dat") || fDetachDB) {
                        debug::log(0, "%s detach\n", strFile.c_str());
                        dbenv.lsn_reset(strFile.c_str(), 0);
                    }
                    debug::log(0, "%s closed\n", strFile.c_str());
                    mapFileUseCount.erase(mi++);
                }
                else
                    mi++;
            }
            if (fShutdown)
            {
                char** listp;
                if (mapFileUseCount.empty())
                {
                    dbenv.log_archive(&listp, DB_ARCH_REMOVE);
                    EnvShutdown();
                }
            }
        }
    }



    bool CTimeDB::ReadTimeData(int& nOffset)
    {
        return Read(0, nOffset);
    }

    bool CTimeDB::WriteTimeData(int nOffset)
    {
        return Write(0, nOffset);
    }



    //
    // CTxDB
    //

    bool CTxDB::ReadTxIndex(uint512_t hash, Core::CTxIndex& txindex)
    {
        assert(!Net::fClient);
        txindex.SetNull();
        return Read(make_pair(string("tx"), hash), txindex);
    }

    bool CTxDB::UpdateTxIndex(uint512_t hash, const Core::CTxIndex& txindex)
    {
        assert(!Net::fClient);
        return Write(make_pair(string("tx"), hash), txindex);
    }

    bool CTxDB::AddTxIndex(const Core::CTransaction& tx, const Core::CDiskTxPos& pos, int nHeight)
    {
        assert(!Net::fClient);

        // Add to tx index
        uint512_t hash = tx.GetHash();
        Core::CTxIndex txindex(pos, tx.vout.size());
        return Write(make_pair(string("tx"), hash), txindex);
    }

    bool CTxDB::EraseTxIndex(const Core::CTransaction& tx)
    {
        assert(!Net::fClient);
        uint512_t hash = tx.GetHash();

        return Erase(make_pair(string("tx"), hash));
    }

    bool CTxDB::ContainsTx(uint512_t hash)
    {
        assert(!Net::fClient);
        return Exists(make_pair(string("tx"), hash));
    }

    bool CTxDB::ReadOwnerTxes(uint512_t hash, int nMinHeight, vector<Core::CTransaction>& vtx)
    {
        assert(!Net::fClient);
        vtx.clear();

        // Get cursor
        Dbc* pcursor = GetCursor();
        if (!pcursor)
            return false;

        uint32_t fFlags = DB_SET_RANGE;
        loop() {
            // Read next record
            CDataStream ssKey(SER_DISK, DATABASE_VERSION);
            if (fFlags == DB_SET_RANGE)
                ssKey << string("owner") << hash << Core::CDiskTxPos(0, 0, 0);
            CDataStream ssValue(SER_DISK, DATABASE_VERSION);
            int ret = ReadAtCursor(pcursor, ssKey, ssValue, fFlags);
            fFlags = DB_NEXT;
            if (ret == DB_NOTFOUND)
                break;
            else if (ret != 0)
            {
                pcursor->close();
                return false;
            }

            // Unserialize
            string strType;
            uint512_t hashItem;
            Core::CDiskTxPos pos;
            int nItemHeight;

            try {
                ssKey >> strType >> hashItem >> pos;
                ssValue >> nItemHeight;
            }
            catch (std::exception &e) {
                return error("%s() : deserialize error", __PRETTY_FUNCTION__);
            }

            // Read transaction
            if (strType != "owner" || hashItem != hash)
                break;
            if (nItemHeight >= nMinHeight)
            {
                vtx.resize(vtx.size()+1);
                if (!vtx.back().ReadFromDisk(pos))
                {
                    pcursor->close();
                    return false;
                }
            }
        }

        pcursor->close();
        return true;
    }

    bool CTxDB::ReadDiskTx(uint512_t hash, Core::CTransaction& tx, Core::CTxIndex& txindex)
    {
        assert(!Net::fClient);
        tx.SetNull();
        if (!ReadTxIndex(hash, txindex))
            return false;
        return (tx.ReadFromDisk(txindex.pos));
    }

    bool CTxDB::ReadDiskTx(uint512_t hash, Core::CTransaction& tx)
    {
        Core::CTxIndex txindex;
        return ReadDiskTx(hash, tx, txindex);
    }

    bool CTxDB::ReadDiskTx(Core::COutPoint outpoint, Core::CTransaction& tx, Core::CTxIndex& txindex)
    {
        return ReadDiskTx(outpoint.hash, tx, txindex);
    }

    bool CTxDB::ReadDiskTx(Core::COutPoint outpoint, Core::CTransaction& tx)
    {
        Core::CTxIndex txindex;
        return ReadDiskTx(outpoint.hash, tx, txindex);
    }

    bool CTxDB::WriteBlockIndex(const Core::CDiskBlockIndex& blockindex)
    {
        return Write(make_pair(string("blockindex"), blockindex.GetBlockHash()), blockindex);
    }

    bool CTxDB::EraseBlockIndex(uint1024_t hash)
    {
        return Erase(make_pair(string("blockindex"), hash));
    }

    bool CTxDB::ReadHashBestChain(uint1024_t& hashBestChain)
    {
        return Read(string("hashBestChain"), hashBestChain);
    }

    bool CTxDB::WriteHashBestChain(uint1024_t hashBestChain)
    {
        return Write(string("hashBestChain"), hashBestChain);
    }

    bool CTxDB::ReadCheckpointPubKey(string& strPubKey)
    {
        return Read(string("strCheckpointPubKey"), strPubKey);
    }

    bool CTxDB::WriteCheckpointPubKey(const string& strPubKey)
    {
        return Write(string("strCheckpointPubKey"), strPubKey);
    }


    //
    // CAddrDB
    //

    bool CAddrDB::WriteAddrman(const Net::CAddrMan& addrman)
    {
        return Write(string("addrman"), addrman);
    }

    bool CAddrDB::LoadAddresses()
    {
        if (Read(string("addrman"), Net::addrman))
        {
            debug::log(0, "Loaded %i addresses\n", Net::addrman.size());
            return true;
        }

        // Read pre-0.6 addr records

        vector<Net::CAddress> vAddr;
        vector<vector<uint8_t> > vDelete;

        // Get cursor
        Dbc* pcursor = GetCursor();
        if (!pcursor)
            return false;

        loop() {
            // Read next record
            CDataStream ssKey(SER_DISK, DATABASE_VERSION);
            CDataStream ssValue(SER_DISK, DATABASE_VERSION);
            int ret = ReadAtCursor(pcursor, ssKey, ssValue);
            if (ret == DB_NOTFOUND)
                break;
            else if (ret != 0)
                return false;

            // Unserialize
            string strType;
            ssKey >> strType;
            if (strType == "addr")
            {
                Net::CAddress addr;
                ssValue >> addr;
                vAddr.push_back(addr);
            }
        }
        pcursor->close();

        Net::addrman.Add(vAddr, Net::CNetAddr("0.0.0.0"));
        debug::log(0, "Loaded %i addresses\n", Net::addrman.size());

        // Note: old records left; we ran into hangs-on-startup
        // bugs for some users who (we think) were running after
        // an unclean shutdown.

        return true;
    }

    bool LoadAddresses()
    {
        return CAddrDB("cr+").LoadAddresses();
    }
}
