/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLD_INCLUDE_LOCAL_H
#define NEXUS_LLD_INCLUDE_LOCAL_H

#include <LLC/types/uint1024.h>

#include <LLD/templates/sector.h>
#include <LLD/cache/binary_lru.h>
#include <LLD/keychain/hashmap.h>

#include <TAO/Ledger/include/stake_change.h>


/** Forward declarations **/
namespace TAO
{
    namespace Ledger
    {
        class Transaction;
    }
}


namespace LLD
{

  /** LocalDB
   *
   *  Database class for storing local wallet transactions.
   *
   **/
    class LocalDB : public SectorDatabase<BinaryHashMap, BinaryLRU>
    {

    public:

        /** The Database Constructor. To determine file location and the Bytes per Record. **/
        LocalDB(const uint8_t nFlagsIn = FLAGS::CREATE | FLAGS::WRITE,
            const uint32_t nBucketsIn = 77773, const uint32_t nCacheIn = 1024 * 1024);


        /** Default Destructor **/
        virtual ~LocalDB();


        /** WriteIndex
         *
         *  Writes the txid of the transaction that modified the given register.
         *
         *  @param[in] hashAddress The register address.
         *  @param[in] pairIndex The index containing expiration and txid.
         *
         *  @return True if the last was successfully written, false otherwise.
         *
         **/
        bool WriteIndex(const uint256_t& hashAddress, const std::pair<uint512_t, uint64_t>& pairIndex);


        /** ReadIndex
         *
         *  Reads the txid of the transaction that modified the given register.
         *
         *  @param[in] hashAddress The register address.
         *  @param[out] pairIndex The index containing expiration and txid.
         *
         *  @return True if the last was successfully read, false otherwise.
         *
         **/
        bool ReadIndex(const uint256_t& hashAddress, std::pair<uint512_t, uint64_t> &pairIndex);


        /** WriteExpiration
         *
         *  Writes the timestamp of a register's local disk cache expiration.
         *
         *  @param[in] hashAddress The register address.
         *  @param[in] nTimestamp The timestamp the register cache expires.
         *
         *  @return True if the last was successfully written, false otherwise.
         *
         **/
        bool WriteExpiration(const uint256_t& hashAddress, const uint64_t nTimestamp);


        /** ReadExpiration
         *
         *  Reads the timestamp of a register's local disk cache expiration.
         *
         *  @param[in] hashAddress The register address.
         *  @param[out] nTimestamp The timestamp register cache expires.
         *
         *  @return True if the last was successfully read, false otherwise.
         *
         **/
        bool ReadExpiration(const uint256_t& hashAddress, uint64_t &nTimestamp);


        /** WriteStakeChange
         *
         *  Writes a stake change request indexed by genesis.
         *
         *  @param[in] hashGenesis The user genesis requesting the stake change.
         *  @param[in] stakeChange Stake change request data to record.
         *
         *  @return True if the change was successfully written, false otherwise.
         *
         **/
        bool WriteStakeChange(const uint256_t& hashGenesis, const TAO::Ledger::StakeChange& stakeChange);


        /** ReadStakeChange
         *
         *  Reads a stake change request for a sig chain genesis.
         *
         *  @param[in] hashGenesis The user genesis to read.
         *
         *  @return True if the change was successfully read, false otherwise.
         *
         **/
        bool ReadStakeChange(const uint256_t& hashGenesis, TAO::Ledger::StakeChange& stakeChange);


        /** EraseStakeChange
         *
         *  Removes a recorded stake change request.
         *
         *  @param[in] hashGenesis The user genesis of the request to remove.
         *
         *  @return True if the change was successfully erased, false otherwise.
         *
         **/
        bool EraseStakeChange(const uint256_t& hashGenesis);

    };
}

#endif
