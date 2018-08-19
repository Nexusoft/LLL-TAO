/*******************************************************************************************

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

[Learn and Create] Viz. http://www.opensource.org/licenses/mit-license.php

*******************************************************************************************/

#include "keystore.h"
#include "script.h"

namespace Wallet
{
    bool CKeyStore::GetPubKey(const NexusAddress &address, std::vector<unsigned char> &vchPubKeyOut) const
    {
        CKey key;
        if (!GetKey(address, key))
            return false;
        vchPubKeyOut = key.GetPubKey();
        return true;
    }

    bool CBasicKeyStore::AddKey(const CKey& key)
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

    bool CBasicKeyStore::HaveCScript(const LLC::uint256& hash) const
    {
        bool result;
        {
            LOCK(cs_KeyStore);
            result = (mapScripts.count(hash) > 0);
        }
        return result;
    }


    bool CBasicKeyStore::GetCScript(const LLC::uint256 &hash, CScript& redeemScriptOut) const
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
                const std::vector<unsigned char> &vchPubKey = (*mi).second.first;
                const std::vector<unsigned char> &vchCryptedSecret = (*mi).second.second;
                CSecret vchSecret;
                if(!DecryptSecret(vMasterKeyIn, vchCryptedSecret, SK576(vchPubKey.begin(), vchPubKey.end()), vchSecret))
                    return false;
                if (vchSecret.size() != 72)
                    return false;
                CKey key;
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

    bool CCryptoKeyStore::AddKey(const CKey& key)
    {
        {
            LOCK(cs_KeyStore);
            if (!IsCrypted())
                return CBasicKeyStore::AddKey(key);

            if (IsLocked())
                return false;

            std::vector<unsigned char> vchCryptedSecret;
            std::vector<unsigned char> vchPubKey = key.GetPubKey();
            bool fCompressed;
            if (!EncryptSecret(vMasterKey, key.GetSecret(fCompressed), SK576(vchPubKey.begin(), vchPubKey.end()), vchCryptedSecret))
                return false;

            if (!AddCryptedKey(key.GetPubKey(), vchCryptedSecret))
                return false;
        }
        return true;
    }


    bool CCryptoKeyStore::AddCryptedKey(const std::vector<unsigned char> &vchPubKey, const std::vector<unsigned char> &vchCryptedSecret)
    {
        {
            LOCK(cs_KeyStore);
            if (!SetCrypted())
                return false;

            mapCryptedKeys[NexusAddress(vchPubKey)] = make_pair(vchPubKey, vchCryptedSecret);
        }
        return true;
    }

    bool CCryptoKeyStore::GetKey(const NexusAddress &address, CKey& keyOut) const
    {
        {
            LOCK(cs_KeyStore);
            if (!IsCrypted())
                return CBasicKeyStore::GetKey(address, keyOut);

            CryptedKeyMap::const_iterator mi = mapCryptedKeys.find(address);
            if (mi != mapCryptedKeys.end())
            {
                const std::vector<unsigned char> &vchPubKey = (*mi).second.first;
                const std::vector<unsigned char> &vchCryptedSecret = (*mi).second.second;
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

    bool CCryptoKeyStore::GetPubKey(const NexusAddress &address, std::vector<unsigned char>& vchPubKeyOut) const
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
            BOOST_FOREACH(KeyMap::value_type& mKey, mapKeys)
            {
                CKey key;
                if (!key.SetSecret(mKey.second.first, mKey.second.second))
                    return false;
                const std::vector<unsigned char> vchPubKey = key.GetPubKey();
                std::vector<unsigned char> vchCryptedSecret;
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
