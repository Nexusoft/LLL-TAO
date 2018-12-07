/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2018

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_TAO_LEGACY_KEYSTORE_BASIC_H
#define NEXUS_TAO_LEGACY_KEYSTORE_BASIC_H

#include <LLC/include/key.h>

#include <TAO/Legacy/types/address.h>
#include <TAO/Legacy/types/secret.h>
#include <TAO/Legacy/types/script.h>

#include <Util/include/mutex.h>

namespace Legacy
{

    /** An abstract base class for key stores */
    class CKeyStore
    {
    protected:

        /** Internal recursive mutex for locks. **/
        mutable std::recursive_mutex cs_KeyStore;

    public:
        virtual ~CKeyStore() {}


        // abstract method defintions
        virtual bool AddKey(const ECKey& key) = 0;
        virtual bool HaveKey(const NexusAddress &address) const = 0;
        virtual bool GetKey(const NexusAddress &address, ECKey& keyOut) const = 0;
        virtual void GetKeys(std::set<NexusAddress> &setAddress) const = 0;
        virtual bool AddCScript(const CScript& redeemScript) = 0;
        virtual bool HaveCScript(const uint256_t &hash) const = 0;
        virtual bool GetCScript(const uint256_t &hash, CScript& redeemScriptOut) const = 0;


        /** Get Pub Key
         *
         *  Get the public key from the given nexus address.
         *
         *  @param[in] address The address to get public key for.
         *  @param[out] vchPubKeyOut The public key to return.
         *
         *  @return true if the public key was extracted.
         *
         **/
        virtual bool GetPubKey(const NexusAddress &address, std::vector<uint8_t>& vchPubKeyOut) const;


        /** Get Secret
         *
         *  Get the secret key from a nexus address.
         *
         *  @param[in] address The address to get secret key for.
         *  @param[out] vchSecret The secret key to return.
         *  @param[in] fCompressed Flag to determine if key should be in compressed form.
         *
         *  @return true if the secret key was extracted.
         *
         **/
        virtual bool GetSecret(const NexusAddress &address, CSecret& vchSecret, bool &fCompressed) const;
    };


    /** Basic key store, that keeps keys in an address->secret map */
    class CBasicKeyStore : public CKeyStore
    {
    protected:

        /** Internal map to hold list of keys by address. **/
        std::map<NexusAddress, std::pair<CSecret, bool> > mapKeys;


        /** Internal map to hold scripts by hash. **/
        std::map<uint256_t, CScript > mapScripts;

    public:


        /** Add Key
         *
         *  Add a key to this keystore.
         *
         *  @param[in] key The key object to add.
         *
         *  @return true if the key was added to the store.
         *
         **/
        bool AddKey(const LLC::ECKey& key);



        /** Have Key
         *
         *  Checks if a key exists in the keystore.
         *
         *  @param[in] address The address to check by.
         *
         *  @return true if the key was found.
         *
         **/
        bool HaveKey(const NexusAddress &address) const
        {
            bool result;
            {
                LOCK(cs_KeyStore);
                result = (mapKeys.count(address) > 0);
            }
            return result;
        }


        /** Get Keys
         *
         *  Gets keys from this keystore.
         *
         *  @param[out] setAddresses The addresses of keys in keystore.
         *
         **/
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


        /** Get Key
         *
         *  Get a single key from the keystore by address.
         *
         *  @param[in] address The address to find key by.
         *  @param[out] keyOut The key that was found in keystore.
         *
         *  @return true if the key was found.
         *
         **/
        bool GetKey(const NexusAddress &address, LLC::ECKey &keyOut) const
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


        /** Add CScript
         *
         *  Add a script to this keystore.
         *
         *  @param[in] redeemScript The script object to add
         *
         *  @return true if the script was added to the store.
         *
         **/
        virtual bool AddCScript(const CScript& redeemScript);


        /** Have CScript
         *
         *  Check the keystore if a script exists.
         *
         *  @param[in] hash The hash to search for.
         *
         *  @return true if the script exists in the store.
         *
         **/
        virtual bool HaveCScript(const uint256_t &hash) const;


        /** Get CScript
         *
         *  Get a script from the keystore.
         *
         *  @param[in] hash The hash to search for.
         *  @param[out] redeemScriptOut The script object to return.
         *
         *  @return true if the script was found in the store.
         *
         **/
        virtual bool GetCScript(const uint256_t &hash, CScript& redeemScriptOut) const;

    };

}

#endif
