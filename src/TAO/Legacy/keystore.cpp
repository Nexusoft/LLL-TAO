/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++

			(c) Copyright The Nexus Developers 2014 - 2018

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include "keystore.h"
#include "script.h"

namespace Legacy
{
    bool CKeyStore::GetPubKey(const NexusAddress &address, std::vector<uint8_t> &vchPubKeyOut) const
    {
        ECKey key;
        if (!GetKey(address, key))
            return false;
        vchPubKeyOut = key.GetPubKey();
        return true;
    }

    bool CBasicKeyStore::AddKey(const ECKey& key)
    {
        bool fCompressed = false;
        CSecret secret = key.GetSecret(fCompressed);
        {
            LOCK(cs_KeyStore);
            mapKeys[NexusAddress(key.GetPubKey())] = make_pair(secret, fCompressed);
        }
        return true;
    }

    bool CBasicKeyStore::AddCScript(const CScript& redeemScript)
    {
        {
            LOCK(cs_KeyStore);
            mapScripts[SK256(redeemScript)] = redeemScript;
        }
        return true;
    }

    bool CBasicKeyStore::HaveCScript(const uint256_t& hash) const
    {
        bool result;
        {
            LOCK(cs_KeyStore);
            result = (mapScripts.count(hash) > 0);
        }
        return result;
    }


    bool CBasicKeyStore::GetCScript(const uint256_t &hash, CScript& redeemScriptOut) const
    {
        {
            LOCK(cs_KeyStore);
            ScriptMap::const_iterator mi = mapScripts.find(hash);
            if (mi != mapScripts.end())
            {
                redeemScriptOut = (*mi).second;
                return true;
            }
        }
        return false;
    }

    bool CCryptoKeyStore::SetCrypted()
    {
        {
            LOCK(cs_KeyStore);
            if (fUseCrypto)
                return true;
            if (!mapKeys.empty())
                return false;
            fUseCrypto = true;
        }
        return true;
    }

    bool CCryptoKeyStore::Unlock(const CKeyingMaterial& vMasterKeyIn)
    {
        {
            LOCK(cs_KeyStore);
            if (!SetCrypted())
                return false;

            CryptedKeyMap::const_iterator mi = mapCryptedKeys.begin();
            for (; mi != mapCryptedKeys.end(); ++mi)
            {
                const std::vector<uint8_t> &vchPubKey = (*mi).second.first;
                const std::vector<uint8_t> &vchCryptedSecret = (*mi).second.second;
                CSecret vchSecret;
                if(!DecryptSecret(vMasterKeyIn, vchCryptedSecret, SK576(vchPubKey.begin(), vchPubKey.end()), vchSecret))
                    return false;
                if (vchSecret.size() != 72)
                    return false;
                ECKey key;
                key.SetPubKey(vchPubKey);
                key.SetSecret(vchSecret);
                if (key.GetPubKey() == vchPubKey)
                    break;
                return false;
            }
            vMasterKey = vMasterKeyIn;
        }
        return true;
    }

    bool CCryptoKeyStore::AddKey(const ECKey& key)
    {
        {
            LOCK(cs_KeyStore);
            if (!IsCrypted())
                return CBasicKeyStore::AddKey(key);

            if (IsLocked())
                return false;

            std::vector<uint8_t> vchCryptedSecret;
            std::vector<uint8_t> vchPubKey = key.GetPubKey();
            bool fCompressed;
            if (!EncryptSecret(vMasterKey, key.GetSecret(fCompressed), SK576(vchPubKey.begin(), vchPubKey.end()), vchCryptedSecret))
                return false;

            if (!AddCryptedKey(key.GetPubKey(), vchCryptedSecret))
                return false;
        }
        return true;
    }


    bool CCryptoKeyStore::AddCryptedKey(const std::vector<uint8_t> &vchPubKey, const std::vector<uint8_t> &vchCryptedSecret)
    {
        {
            LOCK(cs_KeyStore);
            if (!SetCrypted())
                return false;

            mapCryptedKeys[NexusAddress(vchPubKey)] = make_pair(vchPubKey, vchCryptedSecret);
        }
        return true;
    }

    bool CCryptoKeyStore::GetKey(const NexusAddress &address, ECKey& keyOut) const
    {
        {
            LOCK(cs_KeyStore);
            if (!IsCrypted())
                return CBasicKeyStore::GetKey(address, keyOut);

            CryptedKeyMap::const_iterator mi = mapCryptedKeys.find(address);
            if (mi != mapCryptedKeys.end())
            {
                const std::vector<uint8_t> &vchPubKey = (*mi).second.first;
                const std::vector<uint8_t> &vchCryptedSecret = (*mi).second.second;
                CSecret vchSecret;
                if (!DecryptSecret(vMasterKey, vchCryptedSecret, SK576(vchPubKey.begin(), vchPubKey.end()), vchSecret))
                    return false;
                if (vchSecret.size() != 72)
                    return false;
                keyOut.SetPubKey(vchPubKey);
                keyOut.SetSecret(vchSecret);
                return true;
            }
        }
        return false;
    }

    bool CCryptoKeyStore::GetPubKey(const NexusAddress &address, std::vector<uint8_t>& vchPubKeyOut) const
    {
        {
            LOCK(cs_KeyStore);
            if (!IsCrypted())
                return CKeyStore::GetPubKey(address, vchPubKeyOut);

            CryptedKeyMap::const_iterator mi = mapCryptedKeys.find(address);
            if (mi != mapCryptedKeys.end())
            {
                vchPubKeyOut = (*mi).second.first;
                return true;
            }
        }
        return false;
    }

    bool CCryptoKeyStore::EncryptKeys(CKeyingMaterial& vMasterKeyIn)
    {
        {
            LOCK(cs_KeyStore);
            if (!mapCryptedKeys.empty() || IsCrypted())
                return false;

            fUseCrypto = true;
            for(KeyMap::value_type& mKey : mapKeys)
            {
                ECKey key;
                if (!key.SetSecret(mKey.second.first, mKey.second.second))
                    return false;
                const std::vector<uint8_t> vchPubKey = key.GetPubKey();
                std::vector<uint8_t> vchCryptedSecret;
                bool fCompressed;
                if (!EncryptSecret(vMasterKeyIn, key.GetSecret(fCompressed), SK576(vchPubKey.begin(), vchPubKey.end()), vchCryptedSecret))
                    return false;
                if (!AddCryptedKey(vchPubKey, vchCryptedSecret))
                    return false;
            }
            mapKeys.clear();
        }
        return true;
    }

}
