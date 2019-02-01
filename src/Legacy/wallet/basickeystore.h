/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2019

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_LEGACY_WALLET_BASICKEYSTORE_H
#define NEXUS_LEGACY_WALLET_BASICKEYSTORE_H

#include <map>
#include <set>
#include <utility>

#include <LLC/include/key.h>
#include <LLC/types/uint1024.h>

#include <Legacy/types/address.h>
#include <Legacy/types/script.h>
#include <Legacy/wallet/keystore.h>

#include <Util/include/mutex.h>

namespace Legacy
{

    /** Map to store private keys, keyed by Base 58-encoded address. bool=true indicates compressed key **/
    typedef std::map<NexusAddress, std::pair<LLC::CSecret, bool> > KeyMap;

    /** Map to store scripts, keyed by 256 bit script hash **/
    typedef std::map<uint256_t, Script > ScriptMap;

    /** @class BasicKeyStore
     *
     *  Basic implementation of a key store that keeps keys in an unencrypted address->secret map
     *
     **/
    class BasicKeyStore : public KeyStore
    {
    protected:
        /** Mutex for thread concurrency. **/
        mutable std::mutex cs_basicKeyStore;


        /** Map containing KeyMap of Nexus address/unencrypted private key **/
        KeyMap mapKeys;


        /** Map containing ScriptMap of script hash/Script **/
        ScriptMap mapScripts;


    public:
        /** AddKey
         *
         *  Add a key to the key store.
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
         *
         *  @param[in] address The Base 58-encoded address of the key to retrieve
         *
         *  @param[out] keyOut The retrieved key
         *
         *  @return true if key successfully retrieved
         *
         **/
        virtual bool GetKey(const NexusAddress& address, LLC::ECKey &keyOut) const override;


        /** GetKeys
         *
         *  Retrieve the set of public addresses for all keys currently present in the key store.
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


        /** AddScript
         *
         *  Add a script to the key store.
         *
         *  @param[in] redeemScript The script to add
         *
         *  @return true if script was successfully added
         *
         **/
        virtual bool AddScript(const Script& redeemScript) override;


        /** GetScript
         *
         *  Retrieve a script from the key store.
         *
         *  @param[in] hash The 256 bit hash of the script to retrieve
         *
         *  @param[out] redeemScriptOut The retrieved script
         *
         *  @return true if script successfully retrieved
         *
         **/
        virtual bool GetScript(const uint256_t& hash, Script& redeemScriptOut) const override;


        /** HaveScript
         *
         *  Check whether a script is present in the store.
         *
         *  @param[in] hash The 256 bit hash of the script to check
         *
         *  @return true if script is present in the key store
         *
         **/
        virtual bool HaveScript(const uint256_t& hash) const override;

    };

}

#endif
