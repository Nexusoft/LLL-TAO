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

#include <LLD/include/version.h>
#include <LLD/templates/sector.h>

#include <TAO/Ledger/types/trustkey.h>


namespace LLD
{

    /** TrustDB
     *
     *  The database class for trust keys for both Legacy and Tritium.
     *
     **/
    class TrustDB : public SectorDatabase<BinaryFileMap, BinaryLRU>
    {

    public:


        /** The Database Constructor. To determine file location and the Bytes per Record. **/
        TrustDB(uint8_t nFlagsIn = FLAGS::CREATE | FLAGS::WRITE)
        : SectorDatabase(std::string("trust"), nFlagsIn) { }


        /** Default Destructor **/
        virtual ~TrustDB() {}


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
        bool WriteTrustKey(const uint576_t& hashKey, const TAO::Ledger::TrustKey& key)
        {
            return Write(hashKey, key);
        }


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
        bool ReadTrustKey(const uint576_t& hashKey, TAO::Ledger::TrustKey& key)
        {
            return Read(hashKey, key);
        }
    };
}

#endif
