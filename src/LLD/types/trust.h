/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLD_INCLUDE_TRUST_H
#define NEXUS_LLD_INCLUDE_TRUST_H

#include <LLC/types/uint1024.h>

#include <LLD/templates/sector.h>
#include <LLD/cache/binary_lru.h>
#include <LLD/keychain/hashmap.h>

#include <Legacy/types/trustkey.h>


namespace LLD
{

    /** TrustDB
     *
     *  The database class for trust keys for both Legacy and Tritium.
     *
     **/
    class TrustDB : public SectorDatabase<BinaryHashMap, BinaryLRU>
    {

    public:

        /** The Database Constructor. To determine file location and the Bytes per Record. **/
        TrustDB(const uint8_t nFlagsIn = FLAGS::CREATE | FLAGS::WRITE,
            const uint32_t nBucketsIn = 77773, const uint32_t nCacheIn = 1024 * 1024);


        /** Default Destructor **/
        virtual ~TrustDB();


        /** WriteTrustKey
         *
         *  Writes a trust key to the ledger DB.
         *
         *  @param[in] hashKey The key of trust key to write.
         *  @param[in] key The trust key object.
         *
         *  @return True if the trust key was successfully written, false otherwise.
         *
         **/
        bool WriteTrustKey(const uint576_t& hashKey, const Legacy::TrustKey& key);


        /** ReadTrustKey
         *
         *  Reads a trust key from the ledger DB.
         *
         *  @param[in] hashKey The key of trust key to write.
         *  @param[in] key The trust key object.
         *
         *  @return True if the trust key was successfully written, false otherwise.
         *
         **/
        bool ReadTrustKey(const uint576_t& hashKey, Legacy::TrustKey& key);


        /** EraseTrustKey
         *
         *  Erases a trust key from the trust DB.
         *
         *  @param[in] hashKey The key of the trust key to erase.
         *
         *  @return True if the trust key was successfully erased, false otherwise.
         *
         **/
        bool EraseTrustKey(const uint576_t& hashKey)
        {
            return Erase(hashKey);
        }
    };
}

#endif
