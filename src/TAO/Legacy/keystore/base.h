/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2018

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_TAO_LEGACY_TYPES_KEYSTORE_H
#define NEXUS_TAO_LEGACY_TYPES_KEYSTORE_H

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
        virtual bool AddKey(const LLC::ECKey& key) = 0;
        virtual bool HaveKey(const NexusAddress &address) const = 0;
        virtual bool GetKey(const NexusAddress &address, LLC::ECKey& keyOut) const = 0;
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
        virtual bool GetSecret(const NexusAddress &address, LLC::CSecret& vchSecret, bool &fCompressed) const;
    };
}

#endif
