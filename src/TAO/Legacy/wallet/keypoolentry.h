/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++

			(c) Copyright The Nexus Developers 2014 - 2018

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_LEGACY_WALLET_KEYPOOLENTRY_H
#define NEXUS_LEGACY_WALLET_KEYPOOLENTRY_H

#include <vector>

#include <Util/include/runtime.h>
#include <Util/templates/serialize.h>

namespace Legacy 
{
    
    namespace Wallet
    {
        /** @class CKeyPoolEntry
         *
         *  Defines the public key of a key pool entry.
         *  The CKeyPoolEntry is written to the wallet database to store a pre-generated pool of 
         *  available public keys for use by the wallet. The private keys corresponding to
         *  public keys in the key pool are not included. 
         *
         *  These key pool entries can then be read from the database as new keys are needed
         *  for use by the wallet.
         *
         *  Database key is pool<nPool> where nPool is the pool entry ID number
         **/
        class CKeyPoolEntry
        {
        public:
            /** Timestamp when key pool entry created **/
            uint64_t nTime;

            /** Public key for this key pool entry **/
            std::vector<uint8_t> vchPubKey;


            /** Constructor
             *
             *  Initializes a key pool entry with an empty public key. 
             *
             **/
            CKeyPoolEntry()
            {
                nTime = UnifiedTimestamp();
            }


            /** Constructor
             *
             *  Initializes a key pool entry with the provided public key. 
             *
             *  @param[in] vchPubKeyIn The public key to use for initialization
             *
             **/
            CKeyPoolEntry(const std::vector<uint8_t>& vchPubKeyIn)
            {
                nTime = UnifiedTimestamp();
                vchPubKey = vchPubKeyIn;
            }


            IMPLEMENT_SERIALIZE
            (
                if (!(nSerType & SER_GETHASH))
                    READWRITE(nVersion);
                READWRITE(nTime);
                READWRITE(vchPubKey);
            )
        };

    }
}

#endif
