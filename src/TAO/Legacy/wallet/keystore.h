/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++

			(c) Copyright The Nexus Developers 2014 - 2018

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_LEGACY_WALLET_KEYSTORE_H
#define NEXUS_LEGACY_WALLET_KEYSTORE_H

#include <mutex>
#include <set>
#include <vector>

#include <LLC/types/uint1024.h>

/* forward declarations */    
namespace LLC 
{
    class CSecret;
    class ECKey;
}

namespace Legacy 
{
    
    /* forward declarations */    
    namespace Types
    {
        class NexusAddress;
        class CScript;
    }

    namespace Wallet
    {
        /** A virtual base class for key stores */
        class CKeyStore
        {
        protected:
            mutable std::recursive_mutex cs_KeyStore;

        public:
            virtual ~CKeyStore() {}

            // Add a key to the store.
            virtual bool AddKey(const LLC::ECKey& key) =0;

            // Check whether a key corresponding to a given address is present in the store.
            virtual bool HaveKey(const Legacy::Types::NexusAddress &address) const =0;
            virtual bool GetKey(const Legacy::Types::NexusAddress &address, LLC::ECKey& keyOut) const =0;
            virtual void GetKeys(std::set<Legacy::Types::NexusAddress> &setAddress) const =0;
            virtual bool GetPubKey(const Legacy::Types::NexusAddress &address, std::vector<uint8_t>& vchPubKeyOut) const;
            virtual bool AddCScript(const Legacy::Types::CScript& redeemScript) =0;
            virtual bool HaveCScript(const uint256_t &hash) const =0;
            virtual bool GetCScript(const uint256_t &hash, Legacy::Types::CScript& redeemScriptOut) const =0;

            virtual bool GetSecret(const Legacy::Types::NexusAddress &address, LLC::CSecret& vchSecret, bool &fCompressed) const
            {
                LLC::ECKey key;
                if (!GetKey(address, key))
                    return false;
                vchSecret = key.GetSecret(fCompressed);
                return true;
            }
        };
 
    }
}

#endif
