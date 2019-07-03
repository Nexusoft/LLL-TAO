/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLD_INCLUDE_CONTRACT_H
#define NEXUS_LLD_INCLUDE_CONTRACT_H

#include <LLC/types/uint1024.h>

#include <LLD/templates/sector.h>
#include <LLD/cache/binary_lru.h>
#include <LLD/keychain/hashmap.h>

#include <TAO/Ledger/include/enum.h>


namespace LLD
{

  /** LocalDB
   *
   *  Database class for storing local wallet transactions.
   *
   **/
    class ContractDB : public SectorDatabase<BinaryHashMap, BinaryLRU>
    {
        /** Internal mutex for MEMPOOL mode. **/
        std::mutex MEMORY_MUTEX;


        /** Internal map for MEMPOOL mode. **/
        std::map<std::pair<uint512_t, uint32_t>, uint256_t> mapContracts;

    public:

        /** The Database Constructor. To determine file location and the Bytes per Record. **/
        ContractDB(uint8_t nFlags = FLAGS::CREATE | FLAGS::WRITE);


        /** Default Destructor **/
        virtual ~ContractDB();


        /** WriteContract
         *
         *  Writes a caller that fulfilled a conditional agreement.
         *
         *  @param[in] pairContract The origin contract
         *  @param[in] hashCaller The caller that fulfilled agreement.
         *  @param[in] nFlags The flags for memory or not.
         *
         *  @return True if written, false otherwise.
         *
         **/
        bool WriteContract(const std::pair<uint512_t, uint32_t>& pairContract, const uint256_t& hashCaller,
                           const uint8_t nFlags = TAO::Ledger::FLAGS::BLOCK);


        /** EraseContract
         *
         *  Erase a caller that fulfilled a conditional agreement.
         *
         *  @param[in] pairContract The origin contract
         *
         *  @return True if erased, false otherwise.
         *
         **/
        bool EraseContract(const std::pair<uint512_t, uint32_t>& pairContract);


        /** ReadContract
         *
         *  Reads a caller that fulfilled a conditional agreement.
         *
         *  @param[in] pairContract The origin contract.
         *  @param[out] hashCaller The caller that fulfilled agreement.
         *  @param[in] nFlags The flags for memory or not.
         *
         *  @return True if the caller was read, false otherwise.
         *
         **/
        bool ReadContract(const std::pair<uint512_t, uint32_t>& pairContract, uint256_t& hashCaller,
                          const uint8_t nFlags = TAO::Ledger::FLAGS::BLOCK);


        /** HasCaontract
         *
         *  Checks if a contract is already fulfilled.
         *
         *  @param[in] pairContract The origin contract.
         *  @param[in] nFlags The flags for memory or not.
         *
         *  @return True if a contract exists
         *
         **/
        bool HasContract(const std::pair<uint512_t, uint32_t>& pairContract, const uint8_t nFlags = TAO::Ledger::FLAGS::BLOCK);

    };
}

#endif
