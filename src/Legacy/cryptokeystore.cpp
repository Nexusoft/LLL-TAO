/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2018

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/hash/SK.h>
#include <LLC/include/key.h>

#include <Legacy/types/address.h>

#include <Legacy/wallet/cryptokeystore.h>

namespace Legacy
{

    /* Activate encryption for the key store. */
    bool CCryptoKeyStore::SetCrypted()
    {
        LOCK(cs_CryptoKeyStore);
        if (fUseCrypto)
            return true;

        /* Cannot activate encryption if key store contains any unencrypted keys */
        if (!mapKeys.empty())
            return false;

        fUseCrypto = true;

        return true;
    }


    /*  Convert the key store from unencrypted to encrypted. */
    bool CCryptoKeyStore::EncryptKeys(const CKeyingMaterial& vMasterKeyIn)
    {
        LOCK(cs_CryptoKeyStore);

        /* Check whether key store already encrypted */
        if (!mapCryptedKeys.empty() || IsCrypted())
            return false;

        /* Set key store as encrypted */
        fUseCrypto = true;

        /* Convert unencrypted keys from mapKeys to encrypted keys in mapCryptedKeys
         * mKey will have pair for each map entry
         * mKey.first = base 58 address (map key)
         * mKey.second = std::pair<LLC::CSecret, bool> where bool indicates key compressed true/false
         */
        for(const auto mKey : mapKeys)
        {
            LLC::ECKey key;
            LLC::CSecret vchSecret = mKey.second.first;

            if (!key.SetSecret(vchSecret, mKey.second.second))
                return false;

            const std::vector<uint8_t> vchPubKey = key.GetPubKey();
            std::vector<uint8_t> vchCryptedSecret;
            bool fCompressed;

            /* Successful encryption will place encrypted private key into vchCryptedSecret */
            if (!EncryptSecret(vMasterKeyIn, key.GetSecret(fCompressed), LLC::SK576(vchPubKey.begin(), vchPubKey.end()), vchCryptedSecret))
                return false;

            if (!AddCryptedKey(vchPubKey, vchCryptedSecret))
                return false;
        }

        mapKeys.clear();

        return true;
    }


    /*  Attempt to unlock an encrypted key store using the key provided. */
    bool CCryptoKeyStore::Unlock(const CKeyingMaterial& vMasterKeyIn)
    {
        LOCK(cs_CryptoKeyStore);

        /* Cannot unlock unencrypted key store (it is unlocked by default) */
        if (!SetCrypted())
            return false;

        auto mi = mapCryptedKeys.cbegin();

        /* Only need to check first entry in map to determine successful/unsuccessful unlock
         * Will also unlock successfully if map has no entries
         */
        if (mi != mapCryptedKeys.cend())
        {
            const std::vector<uint8_t> &vchPubKey = (*mi).second.first;
            const std::vector<uint8_t> &vchCryptedSecret = (*mi).second.second;
            LLC::CSecret vchSecret;

            /* Successful decryption will place decrypted private key into vchSecret */
            if(!DecryptSecret(vMasterKeyIn, vchCryptedSecret, LLC::SK576(vchPubKey.begin(), vchPubKey.end()), vchSecret))
                return false;

            if (vchSecret.size() != 72)
                return false;

            LLC::ECKey key;
            key.SetPubKey(vchPubKey);
            key.SetSecret(vchSecret);

            /* When these are equal, the decryption is successful and vMasterKeyIn is a match */
            if (key.GetPubKey() != vchPubKey)
                return false;
        }

        /* Key store unlocked, store encryption key for further use */
        vMasterKey = vMasterKeyIn;

        return true;
    }


    /*  Check whether or not the key store is currently locked. */
    bool CCryptoKeyStore::IsLocked() const
    {
        /* Unencrypted key store is not locked */
        if (!IsCrypted())
            return false;

        bool result;

        LOCK(cs_CryptoKeyStore);

        /* If encryption key is not stored, cannot decrypt keys and key store is locked */
        result = vMasterKey.empty();

        return result;
    }


    /*  Attempt to lock the key store. */
    bool CCryptoKeyStore::Lock()
    {
        /* Cannot lock unencrypted key store */
        if (!SetCrypted())
            return false;

        LOCK(cs_CryptoKeyStore);

        /* Remove any stored encryption key so key store values cannot be decrypted, locking the key store */
        vMasterKey.clear();

        return true;
    }


    /*  Add a public/encrypted private key pair to the key store. */
    bool CCryptoKeyStore::AddCryptedKey(const std::vector<uint8_t> &vchPubKey, const std::vector<uint8_t> &vchCryptedSecret)
    {
        {
            LOCK(cs_CryptoKeyStore);

            /* Key store must be encrypted */
            if (!SetCrypted())
                return false;

            mapCryptedKeys[NexusAddress(vchPubKey)] = make_pair(vchPubKey, vchCryptedSecret);
        }

        return true;
    }


    /*  Add a key to the key store. */
    bool CCryptoKeyStore::AddKey(const LLC::ECKey& key)
    {
        {
            LOCK(cs_CryptoKeyStore);

            /* Add key to basic key store if encryption not active */
            if (!IsCrypted())
                return CBasicKeyStore::AddKey(key);

            /* Cannot add key if key store is encrypted and locked */
            if (IsLocked())
                return false;

            /* When encryption active, first encrypt the key using vMaster key, then add to key store */
            std::vector<uint8_t> vchCryptedSecret;
            std::vector<uint8_t> vchPubKey = key.GetPubKey();
            bool fCompressed;

            /* Successful encryption will place encrypted private key into vchCryptedSecret */
            if (!EncryptSecret(vMasterKey, key.GetSecret(fCompressed), LLC::SK576(vchPubKey.begin(), vchPubKey.end()), vchCryptedSecret))
                return false;

            if (!AddCryptedKey(key.GetPubKey(), vchCryptedSecret))
                return false;
        }
        return true;
    }


    /*  Retrieve a key from the key store. */
    bool CCryptoKeyStore::GetKey(const NexusAddress &address, LLC::ECKey& keyOut) const
    {
        {
            LOCK(cs_CryptoKeyStore);

            /* Cannot decrypt key if key store is encrypted and locked */
            if (IsLocked())
                return false;

            auto mi = mapCryptedKeys.find(address);
            if (mi != mapCryptedKeys.end())
            {
                const std::vector<uint8_t> &vchPubKey = (*mi).second.first;
                const std::vector<uint8_t> &vchCryptedSecret = (*mi).second.second;
                LLC::CSecret vchSecret;

                if (!DecryptSecret(vMasterKey, vchCryptedSecret, LLC::SK576(vchPubKey.begin(), vchPubKey.end()), vchSecret))
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


    /*  Retrieve the set of public addresses for all keys currently present in the key store. */
    void CCryptoKeyStore::GetKeys(std::set<NexusAddress> &setAddress) const
    {
        {
            LOCK(cs_CryptoKeyStore);

            if (!IsCrypted())
            {
                CBasicKeyStore::GetKeys(setAddress);
                return;
            }

            setAddress.clear();

            for (auto &mapEntry : mapCryptedKeys)
                setAddress.insert(mapEntry.first);
        }
    }


    /*  Check whether a key corresponding to a given address is present in the store. */
    bool CCryptoKeyStore::HaveKey(const NexusAddress &address) const
    {
        {
            LOCK(cs_CryptoKeyStore);

            if (!IsCrypted())
                return CBasicKeyStore::HaveKey(address);

            return mapCryptedKeys.count(address) > 0;
        }

        return false;
    }


    /*  Retrieve the public key for a key in the key store. */
    bool CCryptoKeyStore::GetPubKey(const NexusAddress &address, std::vector<uint8_t>& vchPubKeyOut) const
    {
        {
            LOCK(cs_CryptoKeyStore);

            if (!IsCrypted())
                return CKeyStore::GetPubKey(address, vchPubKeyOut);

            auto mi = mapCryptedKeys.find(address);
            if (mi != mapCryptedKeys.end())
            {
                vchPubKeyOut = (*mi).second.first;
                return true;
            }
        }

        return false;
    }

}
