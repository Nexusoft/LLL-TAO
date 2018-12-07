/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2018

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_TAO_LEGACY_KEYSTORE_BASIC_H
#define NEXUS_TAO_LEGACY_KEYSTORE_BASIC_H

#include <TAO/Legacy/keystore/base.h>

namespace Legacy
{

    /** Basic key store, that keeps keys in an address->secret map */
    class CBasicKeyStore : public CKeyStore
    {
    protected:

        /** Internal map to hold list of keys by address. **/
        std::map<NexusAddress, std::pair<LLC::CSecret, bool> > mapKeys;


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
        bool HaveKey(const NexusAddress &address) const;


        /** Get Keys
         *
         *  Gets keys from this keystore.
         *
         *  @param[out] setAddresses The addresses of keys in keystore.
         *
         **/
        void GetKeys(std::set<NexusAddress> &setAddress) const;


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
        bool GetKey(const NexusAddress &address, LLC::ECKey &keyOut) const;


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
