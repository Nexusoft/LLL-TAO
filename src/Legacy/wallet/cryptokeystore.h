/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2019

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LEGACY_WALLET_CRYPTEDKEYSTORE_H
#define NEXUS_LEGACY_WALLET_CRYPTEDKEYSTORE_H

#include <Legacy/wallet/basickeystore.h>
#include <Legacy/wallet/crypter.h>

#include <Util/include/mutex.h>

#include <map>
#include <set>
#include <utility>
#include <vector>


/* forward declaration */
namespace LLC
{
    class ECKey;
}

namespace Legacy
{

    /* forward declaration */
    class NexusAddress;

    /** Map to store public key/encrypted private key pairs, mapped by Base 58-encoded address **/
    typedef std::map<NexusAddress, std::pair<std::vector<uint8_t>, std::vector<uint8_t> > > CryptedKeyMap;


    /** @class CryptoKeyStore
     *
     * Key store that keeps the private keys encrypted.
     *
     * Supports having encryption active or inactive. Must be one or the other for all keys.
     * It derives from the basic key store, which is used if encryption is inactive.
     *
     **/
    class CryptoKeyStore : public BasicKeyStore
    {
    private:
        /** Map containing public key/encrypted private key pairs, keyed by Nexus address **/
        CryptedKeyMap mapCryptedKeys;


        /** Key used for mapCryptedKeys encryption and decryption.
         *  When present, key store is unlocked and keys can be decrypted and retrieved
         *  All keys in a single encrypted key store must be encrypted using the same master key.
         **/
        CKeyingMaterial vMasterKey;


        /** Indicates whether key store is storing private keys in encrypted or unencrypted format.
         *
         * If fUseCrypto is true, mapCryptedKeys is used and mapKeys must be empty
         * if fUseCrypto is false, mapKeys (from BasicKeyStore) is used and vMasterKey/mapCryptedKeys must be empty
         */
        bool fUseCrypto;


    protected:
        /** Mutex for thread concurrency. **/
        mutable std::mutex cs_cryptoKeyStore;


        /** SetCrypted
         *
         * Activate encryption for an empty key store.
         * To activate successfully, the key store cannot contain any unencrypted keys.
         *
         * @return true if encryption successfully activated or previously active
         *
         **/
        virtual bool SetCrypted();


        /** EncryptKeys
         *
         *  Convert the key store keys from unencrypted to encrypted and returns the encrypted keys.
         *
         *  This method converts all the key store keys to encrypted format and stores them in the output map.
         *  It only updates the key store, removing the unencrypted key and storing the encrypted key 
         *  in its place. It does not update any backing file or database.
         *
         *  If the keystore is already encrypted, this method does nothing and returns false.
         *
         *  @param[in] vMasterKeyIn Encryption key used to perform encryption. Value is not stored in vMasterKey
         *                          so key store remains locked after conversion until explictly unlocked
         *
         *  @param[out] mapNewEncryptedKeys Method populates this map with the newly encrypted keys
         *
         *  @return true if keys successfully encrypted, false otherwise
         **/
        virtual bool EncryptKeys(const CKeyingMaterial& vMasterKeyIn, CryptedKeyMap& mapNewEncryptedKeys);


        /** Unlock
         *
         *  Attempt to unlock an encrypted key store using the key provided.
         *  Encrypted key store cannot be accessed until unlocked by providing the key used to encrypt it.
         *
         *  @param[in] vMasterKeyIn Encryption key originally used to perform encryption.
         *
         *  @return true if master key matches key used to encrypt the store and unlock is successful
         *
         **/
        virtual bool Unlock(const CKeyingMaterial& vMasterKeyIn);

    public:
        /** Default constructor
         *
         *  Initializes key store as unencrypted
         *
         **/
        CryptoKeyStore()
        : BasicKeyStore()
        , mapCryptedKeys()
        , vMasterKey()
        , fUseCrypto(false)
        , cs_cryptoKeyStore()
        {
        }


        /** Default Destructor **/
        virtual ~CryptoKeyStore()
        {
        }


        /** IsCrypted
         *
         *  Check if key store is encrypted
         *
         *  @return true if key store is using encryption
         **/
        inline bool IsCrypted() const
        {
            return fUseCrypto;
        }


        /** IsLocked
         *
         *  Check whether or not the key store is currently locked.
         *
         *  @return true if the key store is locked
         **/
        virtual bool IsLocked() const;


        /** Lock
         *
         *  Attempt to lock the key store.
         *  Can only lock the key store if it is encrypted.
         *
         *  @return true if the key store was successfully locked
         **/
        virtual bool Lock();


        /** AddCryptedKey
         *
         *  Add a public/encrypted private key pair to the key store.
         *  Key pair must be created from the same master key used to create any other key pairs in the store.
         *  Key store must have encryption active.
         *
         *  @param[in] vchPubKey The public key to add
         *
         *  @param[in] vchCryptedSecret The encrypted private key
         *
         *  @return true if key successfully added
         *
         **/
        virtual bool AddCryptedKey(const std::vector<uint8_t>& vchPubKey, const std::vector<uint8_t>& vchCryptedSecret);


        /** AddKey
         *
         *  Add a key to the key store.
         *  Encrypts the key if encryption is active and key store unlocked.
         *  Adds to basic keystore if encryption not active.
         *
         *  @param[in] key The key to add
         *
         *  @return true if key successfully added
         *
         **/
        virtual bool AddKey(const LLC::ECKey& key) override;


        /** GetKey
         *
         *  Retrieve a key from the key store.
         *  Encrypted key store must be unlocked.
         *  Decrypts the key if encryption is active.
         *
         *  @param[in] address The Base 58-encoded address of the key to retrieve
         *
         *  @param[out] keyOut The retrieved key
         *
         *  @return true if key successfully retrieved
         *
         **/
        virtual bool GetKey(const NexusAddress& address, LLC::ECKey& keyOut) const override;


        /** GetKeys
         *
         *  Retrieve the set of public addresses for all keys currently present in the key store.
         *  Encrypted key store does not require unlock to retrieve public addresses.
         *
         *  @param[out] setAddress A Set containing the Base 58-encoded addresses of the all keys currently in the key store
         *
         **/
        virtual void GetKeys(std::set<NexusAddress>& setAddress) const override;


        /** HaveKey
         *
         *  Check whether a key corresponding to a given address is present in the store.
         *
         *  @param[in] address The Base 58-encoded address of the key to check
         *
         *  @return true if key is present in the key store
         *
         **/
        virtual bool HaveKey(const NexusAddress& address) const override;


        /** GetPubKey
         *
         *  Retrieve the public key for a key in the key store.
         *
         *  @param[in] address The Base 58-encoded address of the key to retrieve
         *
         *  @param[out] vchPubKeyOut A byte vector containing the retrieved public key
         *
         *  @return true if public key was successfully retrieved
         *
         **/
        virtual bool GetPubKey(const NexusAddress& address, std::vector<uint8_t>& vchPubKeyOut) const override;
    };

}

#endif
