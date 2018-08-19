/*******************************************************************************************

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

[Learn and Create] Viz. http://www.opensource.org/licenses/mit-license.php

*******************************************************************************************/

#ifndef NEXUS_DB_H
#define NEXUS_DB_H

#include "../core/core.h"

#include <map>
#include <string>
#include <vector>

#include <db_cxx.h>

namespace Net { class CAddrMan; }
namespace Core
{
    class COutPoint;
    class CTxIndex;
    class CBlockLocator;
    class CDiskBlockIndex;
    class CDiskTxPos;
}

namespace Wallet
{

    class CWallet;
    class CWalletTx;
    class CMasterKey;

    extern uint32_t nWalletDBUpdated;
    extern bool fDetachDB;
    extern DbEnv dbenv;

    extern void DBFlush(bool fShutdown);
    void ThreadFlushWalletDB(void* parg);
    bool BackupWallet(const CWallet& wallet, const std::string& strDest);


    /** RAII class that provides access to a Berkeley database */
    class CDB
    {
    protected:
        Db* pdb;
        std::string strFile;
        std::vector<DbTxn*> vTxn;
        bool fReadOnly;

        explicit CDB(const char* pszFile, const char* pszMode="r+");
        ~CDB() { Close(); }
    public:
        void Close();
    private:
        CDB(const CDB&);
        void operator=(const CDB&);

    protected:
        template<typename K, typename T>
        bool Read(const K& key, T& value)
        {
            if (!pdb)
                return false;

            // Key
            CDataStream ssKey(SER_DISK, DATABASE_VERSION);
            ssKey.reserve(1000);
            ssKey << key;
            Dbt datKey(&ssKey[0], ssKey.size());

            // Read
            Dbt datValue;
            datValue.set_flags(DB_DBT_MALLOC);
            int ret = pdb->get(GetTxn(), &datKey, &datValue, 0);
            memset(datKey.get_data(), 0, datKey.get_size());
            if (datValue.get_data() == NULL)
                return false;

            // Unserialize value
            try {
                CDataStream ssValue((char*)datValue.get_data(), (char*)datValue.get_data() + datValue.get_size(), SER_DISK, DATABASE_VERSION);
                ssValue >> value;
            }
            catch (std::exception &e) {
                return false;
            }

            // Clear and free memory
            memset(datValue.get_data(), 0, datValue.get_size());
            free(datValue.get_data());
            return (ret == 0);
        }

        template<typename K, typename T>
        bool Write(const K& key, const T& value, bool fOverwrite=true)
        {
            if (!pdb)
                return false;
            if (fReadOnly)
                assert(!"Write called on database in read-only mode");

            // Key
            CDataStream ssKey(SER_DISK, DATABASE_VERSION);
            ssKey.reserve(1000);
            ssKey << key;
            Dbt datKey(&ssKey[0], ssKey.size());

            // Value
            CDataStream ssValue(SER_DISK, DATABASE_VERSION);
            ssValue.reserve(10000);
            ssValue << value;
            Dbt datValue(&ssValue[0], ssValue.size());

            // Write
            int ret = pdb->put(GetTxn(), &datKey, &datValue, (fOverwrite ? 0 : DB_NOOVERWRITE));

            // Clear memory in case it was a private key
            memset(datKey.get_data(), 0, datKey.get_size());
            memset(datValue.get_data(), 0, datValue.get_size());
            return (ret == 0);
        }

        template<typename K>
        bool Erase(const K& key)
        {
            if (!pdb)
                return false;
            if (fReadOnly)
                assert(!"Erase called on database in read-only mode");

            // Key
            CDataStream ssKey(SER_DISK, DATABASE_VERSION);
            ssKey.reserve(1000);
            ssKey << key;
            Dbt datKey(&ssKey[0], ssKey.size());

            // Erase
            int ret = pdb->del(GetTxn(), &datKey, 0);

            // Clear memory
            memset(datKey.get_data(), 0, datKey.get_size());
            return (ret == 0 || ret == DB_NOTFOUND);
        }

        template<typename K>
        bool Exists(const K& key)
        {
            if (!pdb)
                return false;

            // Key
            CDataStream ssKey(SER_DISK, DATABASE_VERSION);
            ssKey.reserve(1000);
            ssKey << key;
            Dbt datKey(&ssKey[0], ssKey.size());

            // Exists
            int ret = pdb->exists(GetTxn(), &datKey, 0);

            // Clear memory
            memset(datKey.get_data(), 0, datKey.get_size());
            return (ret == 0);
        }

        Dbc* GetCursor()
        {
            if (!pdb)
                return NULL;
            Dbc* pcursor = NULL;
            int ret = pdb->cursor(NULL, &pcursor, 0);
            if (ret != 0)
                return NULL;
            return pcursor;
        }

        int ReadAtCursor(Dbc* pcursor, CDataStream& ssKey, CDataStream& ssValue, uint32_t fFlags=DB_NEXT)
        {
            // Read at cursor
            Dbt datKey;
            if (fFlags == DB_SET || fFlags == DB_SET_RANGE || fFlags == DB_GET_BOTH || fFlags == DB_GET_BOTH_RANGE)
            {
                datKey.set_data(&ssKey[0]);
                datKey.set_size(ssKey.size());
            }
            Dbt datValue;
            if (fFlags == DB_GET_BOTH || fFlags == DB_GET_BOTH_RANGE)
            {
                datValue.set_data(&ssValue[0]);
                datValue.set_size(ssValue.size());
            }
            datKey.set_flags(DB_DBT_MALLOC);
            datValue.set_flags(DB_DBT_MALLOC);
            int ret = pcursor->get(&datKey, &datValue, fFlags);
            if (ret != 0)
                return ret;
            else if (datKey.get_data() == NULL || datValue.get_data() == NULL)
                return 99999;

            // Convert to streams
            ssKey.SetType(SER_DISK);
            ssKey.clear();
            ssKey.write((char*)datKey.get_data(), datKey.get_size());
            ssValue.SetType(SER_DISK);
            ssValue.clear();
            ssValue.write((char*)datValue.get_data(), datValue.get_size());

            // Clear and free memory
            memset(datKey.get_data(), 0, datKey.get_size());
            memset(datValue.get_data(), 0, datValue.get_size());
            free(datKey.get_data());
            free(datValue.get_data());
            return 0;
        }

        DbTxn* GetTxn()
        {
            if (!vTxn.empty())
                return vTxn.back();
            else
                return NULL;
        }

    public:
        bool TxnBegin()
        {
            if (!pdb)
                return false;
            DbTxn* ptxn = NULL;
            int ret = dbenv.txn_begin(GetTxn(), &ptxn, DB_TXN_WRITE_NOSYNC);
            if (!ptxn || ret != 0)
                return false;
            vTxn.push_back(ptxn);
            return true;
        }

        bool TxnCommit()
        {
            if (!pdb)
                return false;
            if (vTxn.empty())
                return false;
            int ret = vTxn.back()->commit(0);
            vTxn.pop_back();
            return (ret == 0);
        }

        bool TxnAbort()
        {
            if (!pdb)
                return false;
            if (vTxn.empty())
                return false;
            int ret = vTxn.back()->abort();
            vTxn.pop_back();
            return (ret == 0);
        }

        bool ReadVersion(int& nVersion)
        {
            nVersion = 0;
            return Read(std::string("version"), nVersion);
        }

        bool WriteVersion(int nVersion)
        {
            return Write(std::string("version"), nVersion);
        }

        bool static Rewrite(const std::string& strFile, const char* pszSkip = NULL);
    };





    /** Access to the transaction database (blkindex.dat) */
    class CTxDB : public CDB
    {
    public:
        CTxDB(const char* pszMode="r+") : CDB("blkindex.dat", pszMode) { }
    private:
        CTxDB(const CTxDB&);
        void operator=(const CTxDB&);
    public:
        bool ReadTxIndex(LLC::uint512 hash, Core::CTxIndex& txindex);
        bool UpdateTxIndex(LLC::uint512 hash, const Core::CTxIndex& txindex);
        bool AddTxIndex(const Core::CTransaction& tx, const Core::CDiskTxPos& pos, int nHeight);
        bool EraseTxIndex(const Core::CTransaction& tx);
        bool ContainsTx(LLC::uint512 hash);
        bool ReadOwnerTxes(LLC::uint512 hash, int nHeight, std::vector<Core::CTransaction>& vtx);
        bool ReadDiskTx(LLC::uint512 hash, Core::CTransaction& tx, Core::CTxIndex& txindex);
        bool ReadDiskTx(LLC::uint512 hash, Core::CTransaction& tx);
        bool ReadDiskTx(Core::COutPoint outpoint, Core::CTransaction& tx, Core::CTxIndex& txindex);
        bool ReadDiskTx(Core::COutPoint outpoint, Core::CTransaction& tx);
        bool WriteBlockIndex(const Core::CDiskBlockIndex& blockindex);
        bool EraseBlockIndex(LLC::uint1024 hash);
        bool ReadHashBestChain(LLC::uint1024& hashBestChain);
        bool WriteHashBestChain(LLC::uint1024 hashBestChain);
        bool ReadCheckpointPubKey(std::string& strPubKey);
        bool WriteCheckpointPubKey(const std::string& strPubKey);
    };


    /** Access to the Nexus Time Database (time.dat).
        This contains the Unified Time Data collected over time. */
    class CTimeDB : public CDB
    {
    public:
        CTimeDB(const char* pszMode="r+") : CDB("time.dat", pszMode) { }
    private:
        CTimeDB(const CTimeDB&);
        void operator=(const CTimeDB&);
    public:
        bool WriteTimeData(int nOffset);
        bool ReadTimeData(int& nOffset);
    };


    /** Access to the (IP) address database (addr.dat) */
    class CAddrDB : public CDB
    {
    public:
        CAddrDB(const char* pszMode="r+") : CDB("addr.dat", pszMode) { }
    private:
        CAddrDB(const CAddrDB&);
        void operator=(const CAddrDB&);
    public:
        bool WriteAddrman(const Net::CAddrMan& addr);
        bool LoadAddresses();
    };

    bool LoadAddresses();

}
#endif // NEXUS_DB_H
