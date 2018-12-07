/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_TAO_LEGACY_KEYSTORE_CRYPTED_H
#define NEXUS_TAO_LEGACY_KEYSTORE_CRYPTED_H

#include <LLC/types/bignum.h>
#include <Util/include/base58.h>

namespace Legacy
{

    typedef std::map<NexusAddress, std::pair<std::vector<unsigned char>, std::vector<unsigned char> > > CryptedKeyMap;

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

        virtual bool AddCryptedKey(const std::vector<unsigned char> &vchPubKey, const std::vector<unsigned char> &vchCryptedSecret);
        bool AddKey(const CKey& key);
        bool HaveKey(const NexusAddress &address) const
        {
            {
                LOCK(cs_KeyStore);
                if (!IsCrypted())
                    return CBasicKeyStore::HaveKey(address);
                return mapCryptedKeys.count(address) > 0;
            }
            return false;
        }
        bool GetKey(const NexusAddress &address, CKey& keyOut) const;
        bool GetPubKey(const NexusAddress &address, std::vector<unsigned char>& vchPubKeyOut) const;
        void GetKeys(std::set<NexusAddress> &setAddress) const
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

#endif
