/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++

			(c) Copyright The Nexus Developers 2014 - 2018

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_KEYSTORE_H
#define NEXUS_KEYSTORE_H

#include "crypter.h"
#include "../util/util.h"
#include "base58.h"

class CScript;

namespace Wallet
{
    /** A virtual base class for key stores */
    class CKeyStore
    {
    protected:
        mutable CCriticalSection cs_KeyStore;

    public:
        virtual ~CKeyStore() {}

        // Add a key to the store.
        virtual bool AddKey(const ECKey& key) =0;

        // Check whether a key corresponding to a given address is present in the store.
        virtual bool HaveKey(const NexusAddress &address) const =0;
        virtual bool GetKey(const NexusAddress &address, ECKey& keyOut) const =0;
        virtual void GetKeys(std::set<NexusAddress> &setAddress) const =0;
        virtual bool GetPubKey(const NexusAddress &address, std::vector<uint8_t>& vchPubKeyOut) const;
        virtual bool AddCScript(const CScript& redeemScript) =0;
        virtual bool HaveCScript(const uint256_t &hash) const =0;
        virtual bool GetCScript(const uint256_t &hash, CScript& redeemScriptOut) const =0;

        virtual bool GetSecret(const NexusAddress &address, CSecret& vchSecret, bool &fCompressed) const
        {
            ECKey key;
            if (!GetKey(address, key))
                return false;
            vchSecret = key.GetSecret(fCompressed);
            return true;
        }
    };

    typedef std::map<NexusAddress, std::pair<CSecret, bool> > KeyMap;
    typedef std::map<uint256_t, CScript > ScriptMap;

    /** Basic key store, that keeps keys in an address->secret map */
    class CBasicKeyStore : public CKeyStore
    {
    protected:
        KeyMap mapKeys;
        ScriptMap mapScripts;

    public:
        bool AddKey(const ECKey& key);
        bool HaveKey(const NexusAddress &address) const
        {
            bool result;
            {
                LOCK(cs_KeyStore);
                result = (mapKeys.count(address) > 0);
            }
            return result;
        }
        void GetKeys(std::set<NexusAddress> &setAddress) const
        {
            setAddress.clear();
            {
                LOCK(cs_KeyStore);
                KeyMap::const_iterator mi = mapKeys.begin();
                while (mi != mapKeys.end())
                {
                    setAddress.insert((*mi).first);
                    mi++;
                }
            }
        }
        bool GetKey(const NexusAddress &address, ECKey &keyOut) const
        {
            {
                LOCK(cs_KeyStore);
                KeyMap::const_iterator mi = mapKeys.find(address);
                if (mi != mapKeys.end())
                {
                    keyOut.Reset();
                    keyOut.SetSecret((*mi).second.first, (*mi).second.second);
                    return true;
                }
            }
            return false;
        }
        virtual bool AddCScript(const CScript& redeemScript);
        virtual bool HaveCScript(const uint256_t &hash) const;
        virtual bool GetCScript(const uint256_t &hash, CScript& redeemScriptOut) const;
    };

    typedef std::map<NexusAddress, std::pair<std::vector<uint8_t>, std::vector<uint8_t> > > CryptedKeyMap;

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
        bool AddKey(const ECKey& key);
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
        bool GetKey(const NexusAddress &address, ECKey& keyOut) const;
        bool GetPubKey(const NexusAddress &address, std::vector<uint8_t>& vchPubKeyOut) const;
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
