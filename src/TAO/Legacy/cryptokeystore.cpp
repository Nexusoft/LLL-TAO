/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++

			(c) Copyright The Nexus Developers 2014 - 2018

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/hash/sk.h>
#include <LLC/include/key.h>

#include <TAO/Legacy/types/address.h>

#include <TAO/Legacy/wallet/cryptokeystore.h>

namespace Legacy
{

    namespace Wallet
    {

        /* Activate encryption for the key store. */
        bool CCryptoKeyStore::SetCrypted()
        {
            {
                std::lock_guard<std::mutex> ksLock(cs_KeyStore);
                if (fUseCrypto)
                    return true;

                // Cannot activate encryption if key store contains any unencrypted keys
                if (!mapKeys.empty())
                    return false;

                fUseCrypto = true;
            }

            return true;
        }


        /*  Convert the key store from unencrypted to encrypted. */
        bool CCryptoKeyStore::EncryptKeys(const CKeyingMaterial& vMasterKeyIn)
        {
            {
                std::lock_guard<std::mutex> ksLock(cs_KeyStore);

                //Check whether key store already encrypted
                if (!mapCryptedKeys.empty() || IsCrypted())
                    return false;

                //Set key store as encrypted
                fUseCrypto = true;

                //Convert unencrypted keys from mapKeys to encrypted keys in mapCryptedKeys
                //mKey will have pair for each map entry
                //mKey.first = base 58 address (map key)
                //mKey.second = std::pair<LLC::CSecret, bool> where bool indicates key compressed true/false
                for(const auto mKey : mapKeys)
                {
                    LLC::ECKey key;
                    LLC::CSecret vchSecret = mKey.second.first;

                    if (!key.SetSecret(vchSecret, mKey.second.second))
                        return false;

                    const std::vector<uint8_t> vchPubKey = key.GetPubKey();
                    std::vector<uint8_t> vchCryptedSecret;
                    bool fCompressed;

                    //Successful encryption will place encrypted private key into vchCryptedSecret
                    if (!EncryptSecret(vMasterKeyIn, key.GetSecret(fCompressed), LLC::SK576(vchPubKey.begin(), vchPubKey.end()), vchCryptedSecret))
                        return false;

                    if (!AddCryptedKey(vchPubKey, vchCryptedSecret))
                        return false;
                }

                mapKeys.clear();

            }
            return true;
        }


        /*  Attempt to unlock an encrypted key store using the key provided. */
        bool CCryptoKeyStore::Unlock(const CKeyingMaterial& vMasterKeyIn)
        {
            {
                std::lock_guard<std::mutex> ksLock(cs_KeyStore);

                //Cannot unlock unencrypted key store (it is unlocked by default)
                if (!SetCrypted())
                    return false;

                auto mi = mapCryptedKeys.cbegin();

                //Only need to check first entry in map to determine successful/unsuccessful unlock
                //Will also unlock successfully if map has no entries
                if (mi != mapCryptedKeys.cend())
                {
                    const std::vector<uint8_t> &vchPubKey = (*mi).second.first;
                    const std::vector<uint8_t> &vchCryptedSecret = (*mi).second.second;
                    LLC::CSecret vchSecret;

                    //Successful decryption will place decrypted private key into vchSecret
                    if(!DecryptSecret(vMasterKeyIn, vchCryptedSecret, SK576(vchPubKey.begin(), vchPubKey.end()), vchSecret))
                        return false;

                    if (vchSecret.size() != 72)
                        return false;

                    LLC::ECKey key;
                    key.SetPubKey(vchPubKey);
                    key.SetSecret(vchSecret);

                    //When these are equal, the decryption is successful and vMasterKeyIn is a match
                    if (key.GetPubKey() != vchPubKey)
                        return false;
                }

                //Key store unlocked, store encryption key for further use
                vMasterKey = vMasterKeyIn;
            }

            return true;
        }


        /*  Check whether or not the key store is currently locked. */
        bool CCryptoKeyStore::IsLocked() const
        {
            //Unencrypted key store is not locked
            if (!IsCrypted())
                return false;

            bool result;

            {
                std::lock_guard<std::mutex> ksLock(cs_KeyStore);

                //If encryption key is not stored, cannot decrypt keys and key store is locked
                result = vMasterKey.empty();
            }

            return result;
        }


        /*  Attempt to lock the key store. */
        bool CCryptoKeyStore::Lock()
        {
            //Cannot lock unencrypted key store
            if (!SetCrypted())
                return false;

            {
                std::lock_guard<std::mutex> ksLock(cs_KeyStore);

                //Remove any stored encryption key so key store values cannot be decrypted, locking the key store
                vMasterKey.clear();
            }

            return true;
        }


        /*  Add a public/encrypted private key pair to the key store. */
        bool CCryptoKeyStore::AddCryptedKey(const std::vector<uint8_t> &vchPubKey, const std::vector<uint8_t> &vchCryptedSecret)
        {
            {
                std::lock_guard<std::mutex> ksLock(cs_KeyStore);

                //Key store must be encrypted
                if (!SetCrypted())
                    return false;

                mapCryptedKeys[Legacy::Types::NexusAddress(vchPubKey)] = make_pair(vchPubKey, vchCryptedSecret);
            }
 
            return true;
        }


        /*  Add a key to the key store. */
        bool CCryptoKeyStore::AddKey(const LLC::ECKey& key)
        {
            {
                std::lock_guard<std::mutex> ksLock(cs_KeyStore);

                //Add key to basic key store if encryption not active
                if (!IsCrypted())
                    return CBasicKeyStore::AddKey(key);

                //Cannot add key if key store is encrypted and locked
                if (IsLocked())
                    return false;

                //When encryption active, first encrypt the key using vMaster key, then add to key store
                std::vector<uint8_t> vchCryptedSecret;
                std::vector<uint8_t> vchPubKey = key.GetPubKey();
                bool fCompressed;

                //Successful encryption will place encrypted private key into vchCryptedSecret
                if (!EncryptSecret(vMasterKey, key.GetSecret(fCompressed), LLC::SK576(vchPubKey.begin(), vchPubKey.end()), vchCryptedSecret))
                    return false;

                if (!AddCryptedKey(key.GetPubKey(), vchCryptedSecret))
                    return false;
            }
            return true;
        }


        /*  Retrieve a key from the key store. */
        bool CCryptoKeyStore::GetKey(const Legacy::Types::NexusAddress &address, LLC::ECKey& keyOut) const
        {
            {
                std::lock_guard<std::mutex> ksLock(cs_KeyStore);
 
                //Cannot decrypt key if key store is encrypted and locked
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
        void CCryptoKeyStore::GetKeys(std::set<Legacy::Types::NexusAddress> &setAddress) const 
        {
            {
                std::lock_guard<std::mutex> ksLock(cs_KeyStore);

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
        bool CCryptoKeyStore::HaveKey(const Legacy::Types::NexusAddress &address) const 
        {
            {
                std::lock_guard<std::mutex> ksLock(cs_KeyStore);
 
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
                std::lock_guard<std::mutex> ksLock(cs_KeyStore);

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
}
