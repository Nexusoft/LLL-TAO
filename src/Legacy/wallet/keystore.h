/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2019

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LEGACY_WALLET_KEYSTORE_H
#define NEXUS_LEGACY_WALLET_KEYSTORE_H

#include <set>
#include <vector>

#include <LLC/include/key.h>
#include <LLC/types/uint1024.h>

namespace Legacy
{

    /* forward declarations */
    class NexusAddress;
    class Script;

    /** @class KeyStore
     *
     *  An abstract base class for key stores.
     *
     *  Can store ECKey or Script (or both).
     *
     **/
    class KeyStore
    {
    public:

        /** Virtual Destructor
         *
         *  Supports dynamic allocation of objects in inheritance hierarchy.
         *
         **/
        virtual ~KeyStore()
        {
        };


        /** AddKey
         *
         *  Add a key to the key store.
         *  Pure virtual method for implementation by derived class.
         *
         *  @param[in] key The key to add
         *
         *  @return true if key successfully added
         *
         **/
        virtual bool AddKey(const LLC::ECKey& key) = 0;


        /** GetKey
         *
         *  Retrieve a key from the key store.
         *  Pure virtual method for implementation by derived class.
         *
         *  @param[in] address The Base 58-encoded address of the key to retrieve
         *
         *  @param[out] keyOut The retrieved key
         *
         *  @return true if key successfully retrieved
         *
         **/
        virtual bool GetKey(const NexusAddress& address, LLC::ECKey& keyOut) const = 0;


        /** GetKeys
         *
         *  Retrieve the set of public addresses for all keys currently present in the key store.
         *  Pure virtual method for implementation by derived class.
         *
         *  @param[out] setAddress A Set containing the Base 58-encoded addresses of the all keys currently in the key store
         *
         **/
        virtual void GetKeys(std::set<NexusAddress>& setAddress) const = 0;


        /** HaveKey
         *
         *  Check whether a key corresponding to a given address is present in the store.
         *  Pure virtual method for implementation by derived class.
         *
         *  @param[in] address The Base 58-encoded address of the key to check
         *
         *  @return true if key is present in the key store
         *
         **/
        virtual bool HaveKey(const NexusAddress& address) const = 0;


        /** AddScript
         *
         *  Add a script to the key store.
         *  Pure virtual method for implementation by derived class.
         *
         *  @param[in] redeemScript The script to add
         *
         *  @return true if script was successfully added
         *
         **/
        virtual bool AddScript(const Script& redeemScript) = 0;


        /** GetScript
         *
         *  Retrieve a script from the key store.
         *  Pure virtual method for implementation by derived class.
         *
         *  @param[in] hash The 256 bit hash of the script to retrieve
         *
         *  @param[out] redeemScriptOut The retrieved script
         *
         *  @return true if script successfully retrieved
         *
         **/
        virtual bool GetScript(const uint256_t& hash, Script& redeemScriptOut) const = 0;


        /** HaveScript
         *
         *  Check whether a script is present in the store.
         *  Pure virtual method for implementation by derived class.
         *
         *  @param[in] hash The 256 bit hash of the script to check
         *
         *  @return true if script is present in the key store
         *
         **/
        virtual bool HaveScript(const uint256_t& hash) const = 0;


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
        virtual bool GetPubKey(const NexusAddress& address, std::vector<uint8_t>& vchPubKeyOut) const;


        /** GetSecret
         *
         *  Retrieve the private key associated with an address.
         *
         *  @param[in] address The Base 58-encoded address of the key to retrieve
         *
         *  @param[out] vchSecret The private key in byte code in secure allocator
         *
         *  @param[out] fCompressed true if private key is in compressed form
         *
         *  @return true if address present in the key store and private key was successfully retrieved
         *
         **/
        virtual bool GetSecret(const NexusAddress& address, LLC::CSecret& vchSecret, bool& fCompressed) const;

    };

}

#endif
