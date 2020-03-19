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

    /** RegisterTransaction
     *
     *  Helper class for managing memory states in register database.
     *
     **/
    class ContractTransaction
    {
    public:

        /** Map of states that are stored in memory mode until commited. **/
        std::map<std::pair<uint512_t, uint32_t>, uint256_t> mapContracts;


        /** Set of indexes to remove during commit. **/
        std::set<std::pair<uint512_t, uint32_t>> setErase;

    };

    /** ContractDB
     *
     *  Database class for storing local wallet transactions.
     *
     **/
    class ContractDB : public SectorDatabase<BinaryHashMap, BinaryLRU>
    {
        /** Internal mutex for MEMPOOL mode. **/
        std::mutex MEMORY_MUTEX;


        /** Register transaction to track current open transaction. **/
        static thread_local std::unique_ptr<ContractTransaction> pMemory;


        /** Miner transaction to track current states for miner verification. **/
        static thread_local std::unique_ptr<ContractTransaction> pMiner;


        /** Register transaction to keep open all commited data. **/
        ContractTransaction* pCommit;


    public:

        /** The Database Constructor. To determine file location and the Bytes per Record. **/
        ContractDB(const uint8_t nFlagsIn = FLAGS::CREATE | FLAGS::WRITE,
            const uint32_t nBucketsIn = 77773, const uint32_t nCacheIn = 1024 * 1024);


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
        bool WriteContract(const std::pair<uint512_t, uint32_t>& pair, const uint256_t& hashCaller,
                           const uint8_t nFlags = TAO::Ledger::FLAGS::BLOCK);


        /** EraseContract
         *
         *  Erase a caller that fulfilled a conditional agreement.
         *
         *  @param[in] pairContract The origin contract
         *  @param[in] nFlags The flags for memory or not.
         *
         *  @return True if erased, false otherwise.
         *
         **/
        bool EraseContract(const std::pair<uint512_t, uint32_t>& pair, const uint8_t nFlags = TAO::Ledger::FLAGS::BLOCK);


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
        bool ReadContract(const std::pair<uint512_t, uint32_t>& pair, uint256_t& hashCaller,
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
        bool HasContract(const std::pair<uint512_t, uint32_t>& pair, const uint8_t nFlags = TAO::Ledger::FLAGS::BLOCK);


        /** MemoryBegin
         *
         *  Begin a memory transaction following ACID properties.
         *
         **/
        void MemoryBegin(const uint8_t nFlags = TAO::Ledger::FLAGS::MEMPOOL);


        /** MemoryRelease
         *
         *  Release a memory transaction following ACID properties.
         *
         **/
        void MemoryRelease(const uint8_t nFlags = TAO::Ledger::FLAGS::MEMPOOL);


        /** MemoryCommit
         *
         *  Commit a memory transaction following ACID properties.
         *
         **/
        void MemoryCommit();

    };
}

#endif
