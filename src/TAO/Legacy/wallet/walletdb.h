/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++

			(c) Copyright The Nexus Developers 2014 - 2018

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_LEGACY_WALLET_WALLETDB_H
#define NEXUS_LEGACY_WALLET_WALLETDB_H

#include <string>
#include <utility>
#include <vector>

#include <LLC/types/uint1024.h>

#include <TAO/Legacy/wallet/db.h>

/* forward declaration */    
namespace LLC 
{
    class CPrivKey;
}

namespace Legacy
{
    
    /* forward declaration */    
    namespace Types
    {
        class CScript;
    }

    namespace Wallet
    {

        /* forward declarations */
        class CAccount;
        class CAccountingEntry;
        class CKeyPool;
        class CMasterKey;
        class CWallet;
        class CWalletTx;

        /** Error statuses for the wallet database */
        enum DBErrors
        {
            DB_LOAD_OK,
            DB_CORRUPT,
            DB_TOO_NEW,
            DB_LOAD_FAIL,
            DB_NEED_REWRITE
        };

        /** Access to the wallet database (wallet.dat) */
        class CWalletDB : public CDB
        {
        public:
            CWalletDB(std::string strFilename, const char* pszMode="r+") : CDB(strFilename.c_str(), pszMode)
            {
            }

        private:
            CWalletDB(const CWalletDB&);
            void operator=(const CWalletDB&);

        public:
            bool ReadName(const std::string& strAddress, std::string& strName)
            {
                strName = "";
                return Read(std::make_pair(std::string("name"), strAddress), strName);
            }

            bool WriteName(const std::string& strAddress, const std::string& strName);

            bool EraseName(const std::string& strAddress);

            bool ReadTx(uint512_t hash, CWalletTx& wtx)
            {
                return Read(std::make_pair(std::string("tx"), hash), wtx);
            }

            bool WriteTx(uint512_t hash, const CWalletTx& wtx)
            {
                nWalletDBUpdated++;
                return Write(std::make_pair(std::string("tx"), hash), wtx);
            }

            bool EraseTx(uint512_t hash)
            {
                nWalletDBUpdated++;
                return Erase(std::make_pair(std::string("tx"), hash));
            }

            bool ReadKey(const std::vector<uint8_t>& vchPubKey, LLC::CPrivKey& vchPrivKey)
            {
                vchPrivKey.clear();
                return Read(std::make_pair(std::string("key"), vchPubKey), vchPrivKey);
            }

            bool WriteKey(const std::vector<uint8_t>& vchPubKey, const LLC::CPrivKey& vchPrivKey)
            {
                nWalletDBUpdated++;
                return Write(std::make_pair(std::string("key"), vchPubKey), vchPrivKey, false);
            }

            bool WriteCryptedKey(const std::vector<uint8_t>& vchPubKey, const std::vector<uint8_t>& vchCryptedSecret, bool fEraseUnencryptedKey = true)
            {
                nWalletDBUpdated++;
                if (!Write(std::make_pair(std::string("ckey"), vchPubKey), vchCryptedSecret, false))
                    return false;
                if (fEraseUnencryptedKey)
                {
                    Erase(std::make_pair(std::string("key"), vchPubKey));
                    Erase(std::make_pair(std::string("wkey"), vchPubKey));
                }
                return true;
            }

            bool WriteMasterKey(uint32_t nID, const CMasterKey& kMasterKey)
            {
                nWalletDBUpdated++;
                return Write(std::make_pair(std::string("mkey"), nID), kMasterKey, true);
            }

            bool ReadCScript(const uint256_t &hash, Legacy::Types::CScript& redeemScript)
            {
                redeemScript.clear();
                return Read(std::make_pair(std::string("cscript"), hash), redeemScript);
            }

            bool WriteCScript(const uint256_t& hash, const Legacy::Types::CScript& redeemScript)
            {
                nWalletDBUpdated++;
                return Write(std::make_pair(std::string("cscript"), hash), redeemScript, false);
            }

            bool WriteBestBlock(const Core::CBlockLocator& locator)
            {
                nWalletDBUpdated++;
                return Write(std::string("bestblock"), locator);
            }

            bool ReadBestBlock(Core::CBlockLocator& locator)
            {
                return Read(std::string("bestblock"), locator);
            }

            bool ReadDefaultKey(std::vector<uint8_t>& vchPubKey)
            {
                vchPubKey.clear();
                return Read(std::string("defaultkey"), vchPubKey);
            }

            bool WriteDefaultKey(const std::vector<uint8_t>& vchPubKey)
            {
                nWalletDBUpdated++;
                return Write(std::string("defaultkey"), vchPubKey);
            }

            bool ReadPool(int64_t nPool, CKeyPool& keypool)
            {
                return Read(std::make_pair(std::string("pool"), nPool), keypool);
            }

            bool WritePool(int64_t nPool, const CKeyPool& keypool)
            {
                nWalletDBUpdated++;
                return Write(std::make_pair(std::string("pool"), nPool), keypool);
            }

            bool ErasePool(int64_t nPool)
            {
                nWalletDBUpdated++;
                return Erase(std::make_pair(std::string("pool"), nPool));
            }

            // Settings are no longer stored in wallet.dat; these are
            // used only for backwards compatibility:
            template<typename T>
            bool ReadSetting(const std::string& strKey, T& value)
            {
                return Read(std::make_pair(std::string("setting"), strKey), value);
            }
            template<typename T>
            bool WriteSetting(const std::string& strKey, const T& value)
            {
                nWalletDBUpdated++;
                return Write(std::make_pair(std::string("setting"), strKey), value);
            }
            bool EraseSetting(const std::string& strKey)
            {
                nWalletDBUpdated++;
                return Erase(std::make_pair(std::string("setting"), strKey));
            }

            bool WriteMinVersion(int nVersion)
            {
                return Write(std::string("minversion"), nVersion);
            }

            bool ReadAccount(const std::string& strAccount, CAccount& account);
            bool WriteAccount(const std::string& strAccount, const CAccount& account);
            bool WriteAccountingEntry(const CAccountingEntry& acentry);
            int64_t GetAccountCreditDebit(const std::string& strAccount);
            void ListAccountCreditDebit(const std::string& strAccount, std::list<CAccountingEntry>& acentries);

            int LoadWallet(CWallet* pwallet);
        };

    }
}

#endif
