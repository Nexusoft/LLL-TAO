/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2019

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

        {
            /* Need crypto keystore lock to check and update fUseCrypto. Need to hold it between check and update */
            LOCK(cs_cryptoKeyStore);

            if (fUseCrypto)
                return true;

            {
                /* Need basic keystore lock to check mapKeys.
                 * This is a nested lock.
                 * Have to ensure any others that might nest them also get them in the same order to avoid deadlock potential.
                 * Good part is we only actually need this when first activating encryption. After that, above returns true.
                 */
                LOCK(cs_basicKeyStore);

                /* Cannot activate encryption if key store contains any unencrypted keys */
                if (!mapKeys.empty())
                    return false;
            }

            /* Now can activate encryption */
            fUseCrypto = true;
        }

        return true;
    }


    /*  Convert the key store from unencrypted to encrypted. */
    bool CCryptoKeyStore::EncryptKeys(const CKeyingMaterial& vMasterKeyIn)
    {

        /* Check whether key store already encrypted */
        if (!mapCryptedKeys.empty() || IsCrypted())
            return false;

        /* Set key store as encrypted */
        fUseCrypto = true;

        /* Need basic keystore lock for iterating mapKeys, but will also need it within AddCryptedKey.
         * Thus, can't keep it. To ensure a good mapKeys, make a copy while have hold of lock
         */
        KeyMap mapKeysToEncrypt;
        {
            LOCK(cs_basicKeyStore);

            for(const auto mKey : mapKeys)
            {
                NexusAddress keyAddress = mKey.first;

                mapKeysToEncrypt[keyAddress] = mKey.second;
            }
        } //Now can let go of basic keystore lock

        /* Convert unencrypted keys from mapKeys to encrypted keys in mapCryptedKeys
         * mKey will have pair for each map entry
         * mKey.first = base 58 address (map key)
         * mKey.second = std::pair<LLC::CSecret, bool> where bool indicates key compressed true/false
         */
        for(const auto mKey : mapKeysToEncrypt)
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

            /* Also need crypto keystore lock to add key. AddCryptedKey() obtains this */
            /* During wallet encryption, this will call this->AddCryptedKey() which is actually Wallet::AddCryptedKey() */
            if (!AddCryptedKey(vchPubKey, vchCryptedSecret))
                return false;
        }

        {
            /* Need to obtain lock again to clear mapKeys */
            LOCK(cs_basicKeyStore);

            mapKeys.clear();
        }

        return true;
    }


    /*  Attempt to unlock an encrypted key store using the key provided. */
    bool CCryptoKeyStore::Unlock(const CKeyingMaterial& vMasterKeyIn)
    {
        /* Cannot unlock unencrypted key store (it is unlocked by default) */
        if (!SetCrypted())
            return false;

        LOCK(cs_cryptoKeyStore);

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
        bool result;

        {
            LOCK(cs_cryptoKeyStore);

            /* Unencrypted key store is not locked */
            if (!IsCrypted())
                return false;

            /* If encryption key is not stored, cannot decrypt keys and key store is locked */
            result = vMasterKey.empty();
        }

        return result;
    }


    /*  Attempt to lock the key store. */
    bool CCryptoKeyStore::Lock()
    {
        /* Cannot lock unencrypted key store. This will enable encryption if not enabled already. */
        if (!SetCrypted())
            return false;

        {
            LOCK(cs_cryptoKeyStore);

            /* Remove any stored encryption key so key store values cannot be decrypted, locking the key store */
            vMasterKey.clear();
        }

        return true;
    }


    /*  Add a public/encrypted private key pair to the key store. */
    bool CCryptoKeyStore::AddCryptedKey(const std::vector<uint8_t>& vchPubKey, const std::vector<uint8_t>& vchCryptedSecret)
    {
        /* Key store must be encrypted */
        if (!SetCrypted())
            return false;

        {
            LOCK(cs_cryptoKeyStore);

            mapCryptedKeys[NexusAddress(vchPubKey)] = make_pair(vchPubKey, vchCryptedSecret);
        }

        return true;
    }


    /*  Add a key to the key store. */
    bool CCryptoKeyStore::AddKey(const LLC::ECKey& key)
    {
        /* Only use LOCK to check IsCrypted() -- use internal flag so we can release before potential call to CBasicKeyStore::AddKey */
        bool fCrypted = false;

        {
            LOCK(cs_cryptoKeyStore);

            if (IsCrypted())
                fCrypted = true;
        }

        /* Add key to basic key store if encryption not active */
        if (!fCrypted)
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

        /* AddCryptedKey contains LOCK for updating the crypto keystore. No lock here */
        if (!AddCryptedKey(key.GetPubKey(), vchCryptedSecret))
            return false;

        return true;
    }


    /*  Retrieve a key from the key store. */
    bool CCryptoKeyStore::GetKey(const NexusAddress& address, LLC::ECKey& keyOut) const
    {
        if (!IsCrypted())
            return CBasicKeyStore::GetKey(address, keyOut);


        {
            LOCK(cs_cryptoKeyStore);

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
    void CCryptoKeyStore::GetKeys(std::set<NexusAddress>& setAddress) const
    {
        /* Only use LOCK to check IsCrypted() -- use internal flag so we can release before potential call to CBasicKeyStore::GetKeys */
        bool fCrypted = false;

        {
            LOCK(cs_cryptoKeyStore);

            if (IsCrypted())
                fCrypted = true;
        }

        /* Get keys from basic key store if encryption not active */
        if (!fCrypted)
        {
            CBasicKeyStore::GetKeys(setAddress);
            return;
        }

        {
            /* Get the lock back if we need to load set from mapCryptedKeys */
            LOCK(cs_cryptoKeyStore);

            setAddress.clear();

            for (auto &mapEntry : mapCryptedKeys)
                setAddress.insert(mapEntry.first);
        }
    }


    /*  Check whether a key corresponding to a given address is present in the store. */
    bool CCryptoKeyStore::HaveKey(const NexusAddress& address) const
    {
        /* Only use LOCK to check IsCrypted() -- use internal flag so we can release before potential call to CBasicKeyStore::HaveKey */
        bool fCrypted = false;

        {
            LOCK(cs_cryptoKeyStore);

            if (IsCrypted())
                fCrypted = true;
        }

        if (!fCrypted)
            return CBasicKeyStore::HaveKey(address);

        {
            /* Get the lock back if we need to check mapCryptedKeys */
            LOCK(cs_cryptoKeyStore);

            return mapCryptedKeys.count(address) > 0;
        }

        return false;
    }


    /*  Retrieve the public key for a key in the key store. */
    bool CCryptoKeyStore::GetPubKey(const NexusAddress& address, std::vector<uint8_t>& vchPubKeyOut) const
    {
        /* Only use LOCK to check IsCrypted() -- use internal flag so we can release before potential call to CKeyStore::GetPubKey */
        bool fCrypted = false;

        {
            LOCK(cs_cryptoKeyStore);

            if (IsCrypted())
                fCrypted = true;
        }

        if (!fCrypted)
            return CKeyStore::GetPubKey(address, vchPubKeyOut);

        {
            /* Get the lock back if we need to check mapCryptedKeys */
            LOCK(cs_cryptoKeyStore);

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
