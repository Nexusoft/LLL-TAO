/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2019

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LEGACY_TYPES_KEYPOOLENTRY_H
#define NEXUS_LEGACY_TYPES_KEYPOOLENTRY_H

#include <Util/templates/serialize.h>

#include <vector>

namespace Legacy
{
    /** @class KeyPoolEntry
     *
     *  Defines the public key of a key pool entry.
     *  The KeyPoolEntry is written to the wallet database to store a pre-generated pool of
     *  available public keys for use by the wallet. The private keys corresponding to
     *  public keys in the key pool are not included.
     *
     *  These key pool entries can then be read from the database as new keys are needed
     *  for use by the wallet.
     *
     *  Database key is pool<nPool> where nPool is the pool entry ID number
     **/
    class KeyPoolEntry
    {
    public:

        /** Timestamp when key pool entry created **/
        uint64_t nTime;


        /** Public key for this key pool entry **/
        std::vector<uint8_t> vchPubKey;


        //serialization methods
        IMPLEMENT_SERIALIZE
        (
            if(!(nSerType & SER_GETHASH))
                READWRITE(nSerVersion);

            READWRITE(nTime);
            READWRITE(vchPubKey);
        )


        /** The default constructor. **/
        KeyPoolEntry();


        /** Copy Constructor. **/
        KeyPoolEntry(const KeyPoolEntry& entry);


        /** Move Constructor. **/
        KeyPoolEntry(KeyPoolEntry&& entry) noexcept;


        /** Copy Assignment. **/
        KeyPoolEntry& operator=(const KeyPoolEntry& entry);


        /** Move Assignment. **/
        KeyPoolEntry& operator=(KeyPoolEntry&& entry) noexcept;


        /** Default Destructor **/
        ~KeyPoolEntry();


        /** Constructor
         *
         *  Initializes a key pool entry with the provided public key.
         *
         *  @param[in] vchPubKeyIn The public key to use for initialization
         *
         **/
        KeyPoolEntry(const std::vector<uint8_t>& vchPubKeyIn);

    };

}

#endif
