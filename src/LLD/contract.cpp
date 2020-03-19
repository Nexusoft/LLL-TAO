/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/


#include <LLC/types/uint1024.h>

#include <LLD/types/contract.h>


namespace LLD
{

    /* Register transaction to track current open transaction. */
    thread_local std::unique_ptr<ContractTransaction> ContractDB::pMemory;


    /* Miner transaction to track current states for miner verification. */
    thread_local std::unique_ptr<ContractTransaction> ContractDB::pMiner;


    /** The Database Constructor.  **/
    ContractDB::ContractDB(const uint8_t nFlagsIn, const uint32_t nBucketsIn, const uint32_t nCacheIn)
    : SectorDatabase(std::string("_CONTRACT")
    , nFlagsIn
    , nBucketsIn
    , nCacheIn)

    , MEMORY_MUTEX()
    , pCommit(new ContractTransaction())
    {
    }


    /* Default Destructor */
    ContractDB::~ContractDB()
    {
        /* Free commited memory. */
        if(pCommit)
            delete pCommit;
    }


    /* Writes a caller that fulfilled a conditional agreement.*/
    bool ContractDB::WriteContract(const std::pair<uint512_t, uint32_t>& pair,
                                   const uint256_t& hashCaller, const uint8_t nFlags)
    {
        /* Memory mode for pre-database commits. */
        if(nFlags == TAO::Ledger::FLAGS::MEMPOOL)
        {
            /* Check for pending transactions. */
            if(pMemory)
            {
                /* Write proof to memory. */
                pMemory->setErase.erase(pair);
                pMemory->mapContracts[pair] = hashCaller;

                return true;
            }

            /* Write proof to commited memory. */
            {
                LOCK(MEMORY_MUTEX);
                pCommit->mapContracts[pair] = hashCaller;
            }

            return true;
        }
        else if(nFlags == TAO::Ledger::FLAGS::MINER)
        {
            /* Check for pending transactions. */
            if(pMiner)
                pMiner->mapContracts[pair] = hashCaller;

            return true;
        }
        else if(nFlags == TAO::Ledger::FLAGS::BLOCK || nFlags == TAO::Ledger::FLAGS::ERASE)
        {
            /* Erase memory proof if they exist. */
            if(pMemory)
            {
                pMemory->setErase.insert(pair);
                pMemory->mapContracts.erase(pair);
            }
            else
            {
                LOCK(MEMORY_MUTEX);
                pCommit->mapContracts.erase(pair);
            }

            /* Check for erase to short circuit out. */
            if(nFlags == TAO::Ledger::FLAGS::ERASE)
                return true;
        }

        return Write(pair, hashCaller);
    }


    /* Writes a caller that fulfilled a conditional agreement.*/
    bool ContractDB::EraseContract(const std::pair<uint512_t, uint32_t>& pair, const uint8_t nFlags)
    {
        /* Check for memory transaction. */
        if(nFlags == TAO::Ledger::FLAGS::MEMPOOL)
        {
            /* Check for available states. */
            if(pMemory)
            {
                /* Erase state out of transaction. */
                pMemory->mapContracts.erase(pair);
                pMemory->setErase.insert(pair);

                return true;
            }

            /* Erase proof from mempool. */
            {
                LOCK(MEMORY_MUTEX);
                pCommit->mapContracts.erase(pair);
            }

            return true;
        }
        else if(nFlags == TAO::Ledger::FLAGS::BLOCK || nFlags == TAO::Ledger::FLAGS::ERASE)
        {
            LOCK(MEMORY_MUTEX);

            /* Erase memory proof if they exist. */
            if(pCommit->mapContracts.count(pair))
            {
                /* Erase the proof. */
                if(pMemory)
                {
                    pMemory->mapContracts.erase(pair);
                    pMemory->setErase.insert(pair);
                }
                else
                    pCommit->mapContracts.erase(pair);
            }

            /* Break on erase.  */
            if(nFlags == TAO::Ledger::FLAGS::ERASE)
                return true;
        }

        return Erase(pair);
    }


    /* Reads a caller that fulfilled a conditional agreement.*/
    bool ContractDB::ReadContract(const std::pair<uint512_t, uint32_t>& pair, uint256_t& hashCaller, const uint8_t nFlags)
    {
        /* Memory mode for pre-database commits. */
        if(nFlags == TAO::Ledger::FLAGS::MEMPOOL)
        {
            /* Check for a memory transaction first */
            if(pMemory && pMemory->mapContracts.count(pair))
            {
                /* Get the state from temporary transaction. */
                hashCaller = pMemory->mapContracts[pair];

                return true;
            }

            {
                LOCK(MEMORY_MUTEX);

                /* Check for state in memory map. */
                if(pCommit->mapContracts.count(pair))
                {
                    /* Get the state from commited memory. */
                    hashCaller = pCommit->mapContracts[pair];

                    return true;
                }
            }
        }
        else if(nFlags == TAO::Ledger::FLAGS::MINER)
        {
            /* Check for a memory transaction first */
            if(pMiner && pMiner->mapContracts.count(pair))
            {
                /* Get the state from temporary transaction. */
                hashCaller = pMiner->mapContracts[pair];

                return true;
            }
        }

        return Read(pair, hashCaller);
    }


    /* Checks if a contract is already fulfilled. */
    bool ContractDB::HasContract(const std::pair<uint512_t, uint32_t>& pairContract, const uint8_t nFlags)
    {
        /* Memory mode for pre-database commits. */
        if(nFlags == TAO::Ledger::FLAGS::MEMPOOL)
        {
            /* Set the state in the memory map. */
            if(pMemory && pMemory->mapContracts.count(pairContract))
                return true;

            {
                LOCK(MEMORY_MUTEX);

                /* Check commited memory. */
                if(pCommit->mapContracts.count(pairContract))
                    return true;
            }
        }
        else if(nFlags == TAO::Ledger::FLAGS::MINER)
        {
            /* Check pending transaction memory. */
            if(pMiner && pMiner->mapContracts.count(pairContract))
                return true;
        }

        return Exists(pairContract);
    }

    /* Begin a memory transaction following ACID properties. */
    void ContractDB::MemoryBegin(const uint8_t nFlags)
    {
        /* Check for miner. */
        if(nFlags == TAO::Ledger::FLAGS::MINER)
        {
            /* Set the pre-commit memory mode. */
            pMiner = std::unique_ptr<ContractTransaction>(new ContractTransaction());

            return;
        }

        /* Set the pre-commit memory mode. */
        pMemory = std::unique_ptr<ContractTransaction>(new ContractTransaction());
    }


    /* Abort a memory transaction following ACID properties. */
    void ContractDB::MemoryRelease(const uint8_t nFlags)
    {
        /* Check for miner. */
        if(nFlags == TAO::Ledger::FLAGS::MINER)
        {
            /* Set the pre-commit memory mode. */
            pMiner = nullptr;

            return;
        }

        /* Set the pre-commit memory mode. */
        pMemory = nullptr;
    }


    /* Commit a memory transaction following ACID properties. */
    void ContractDB::MemoryCommit()
    {
        LOCK(MEMORY_MUTEX);

        /* Abort the current memory mode. */
        if(pMemory)
        {
            /* Loop through all new states and apply to commit data. */
            for(const auto& contract : pMemory->mapContracts)
                pCommit->mapContracts[contract.first] = contract.second;

            /* Loop through values to erase. */
            for(const auto& erase : pMemory->setErase)
                pCommit->mapContracts.erase(erase);

            /* Free the memory. */
            pMemory = nullptr;
        }
    }
}
