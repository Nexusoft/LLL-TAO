/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++

			(c) Copyright The Nexus Developers 2014 - 2018

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_LEGACY_WALLET_CRYPTEDKEYSTORE_H
#define NEXUS_LEGACY_WALLET_CRYPTEDKEYSTORE_H

#include <map>
#include <set>
#include <utility>
#include <vector>

#include <LLC/types/uint1024.h>

#include <TAO/Legacy/wallet/basickeystore.h>
#include <TAO/Legacy/wallet/crypter.h> /* for CKeyingMaterial typedef */

/* forward declaration */    
namespace LLC 
{
    class ECKey;
}

namespace Legacy 
{
    
    /* forward declaration */    
    namespace Types
    {
        class NexusAddress;
    }

    namespace Wallet
    {
        typedef std::map<Legacy::Types::NexusAddress, std::pair<std::vector<uint8_t>, std::vector<uint8_t> > > CryptedKeyMap;

        /** Keystore which keeps the private keys encrypted.
        * It derives from the basic key store, which is used if no encryption is active.
        */
        class CCryptoKeyStore : public CBasicKeyStore
        {
        private:
            CryptedKeyMap mapCryptedKeys;

            CKeyingMaterial vMasterKey;

            // if fUseCrypto is true, mapKeys must be empty
            // if fUseCrypto is false, vMasterKey must be empty
            bool fUseCrypto;

        protected:
            bool SetCrypted();

            // will encrypt previously unencrypted keys
            bool EncryptKeys(CKeyingMaterial& vMasterKeyIn);

            bool Unlock(const CKeyingMaterial& vMasterKeyIn);

        public:
            CCryptoKeyStore() : fUseCrypto(false)
            {
            }

            bool IsCrypted() const
            {
                return fUseCrypto;
            }

            bool IsLocked() const
            {
                if (!IsCrypted())
                    return false;
                bool result;
                {
                    LOCK(cs_KeyStore);
                    result = vMasterKey.empty();
                }
                return result;
            }

            bool Lock()
            {
                if (!SetCrypted())
                    return false;

                {
                    LOCK(cs_KeyStore);
                    vMasterKey.clear();
                }

                return true;
            }

            virtual bool AddCryptedKey(const std::vector<uint8_t> &vchPubKey, const std::vector<uint8_t> &vchCryptedSecret);
            bool AddKey(const LLC::ECKey& key);
            bool HaveKey(const Legacy::Types::NexusAddress &address) const
            {
                {
                    LOCK(cs_KeyStore);
                    if (!IsCrypted())
                        return CBasicKeyStore::HaveKey(address);
                    return mapCryptedKeys.count(address) > 0;
                }
                return false;
            }
            bool GetKey(const Legacy::Types::NexusAddress &address, LLC::ECKey& keyOut) const;
            bool GetPubKey(const Legacy::Types::NexusAddress &address, std::vector<uint8_t>& vchPubKeyOut) const;
            void GetKeys(std::set<Legacy::Types::NexusAddress> &setAddress) const
            {
                if (!IsCrypted())
                {
                    CBasicKeyStore::GetKeys(setAddress);
                    return;
                }
                setAddress.clear();
                CryptedKeyMap::const_iterator mi = mapCryptedKeys.begin();
                while (mi != mapCryptedKeys.end())
                {
                    setAddress.insert((*mi).first);
                    mi++;
                }
            }
        };

    }
}

#endif
