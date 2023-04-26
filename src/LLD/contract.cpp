/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2021

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/


#include <LLC/types/uint1024.h>

#include <LLD/types/contract.h>

#include <LLP/include/global.h>


namespace LLD
{
    /** The Database Constructor.  **/
    ContractDB::ContractDB(const Config::Static& sector, const Config::Hashmap& keychain)
    : StaticDatabase(sector, keychain)
    , MEMORY_MUTEX()
    , pMemory(nullptr)
    , pMiner(nullptr)
    , pCommit(new ContractTransaction())
    {
    }


    /* Default Destructor */
    ContractDB::~ContractDB()
    {
        /* Free transaction memory. */
        if(pMemory)
            delete pMemory;

        /* Free miner memory. */
        if(pMiner)
            delete pMiner;

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
            LOCK(MEMORY_MUTEX);

            /* Check for pending transactions. */
            if(pMemory)
            {
                /* Write proof to memory. */
                pMemory->setErase.erase(pair);
                pMemory->mapContracts[pair] = hashCaller;

                return true;
            }

            /* Write proof to commited memory. */
            pCommit->mapContracts[pair] = hashCaller;

            return true;
        }
        else if(nFlags == TAO::Ledger::FLAGS::MINER)
        {
            LOCK(MEMORY_MUTEX);

            /* Check for pending transactions. */
            if(pMiner)
                pMiner->mapContracts[pair] = hashCaller;

            return true;
        }
        else if(nFlags == TAO::Ledger::FLAGS::BLOCK || nFlags == TAO::Ledger::FLAGS::ERASE)
        {
            LOCK(MEMORY_MUTEX);

            /* Erase memory proof if they exist. */
            if(pMemory)
            {
                pMemory->setErase.insert(pair);
                pMemory->mapContracts.erase(pair);
            }
            else
               pCommit->mapContracts.erase(pair);

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
            LOCK(MEMORY_MUTEX);

            /* Check for available states. */
            if(pMemory)
            {
                /* Erase state out of transaction. */
                pMemory->mapContracts.erase(pair);
                pMemory->setErase.insert(pair);

                return true;
            }

            /* Erase proof from mempool. */
            pCommit->mapContracts.erase(pair);

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
    bool ContractDB::ReadContract(const std::pair<uint512_t, uint32_t>& pairContract, uint256_t& hashCaller, const uint8_t nFlags)
    {
        /* Memory mode for pre-database commits. */
        if(nFlags == TAO::Ledger::FLAGS::MEMPOOL)
        {
            LOCK(MEMORY_MUTEX);

            /* Check for a memory transaction first */
            if(pMemory && pMemory->mapContracts.count(pairContract))
            {
                /* Get the state from temporary transaction. */
                hashCaller = pMemory->mapContracts[pairContract];

                return true;
            }

            /* Check for state in memory map. */
            if(pCommit->mapContracts.count(pairContract))
            {
                /* Get the state from commited memory. */
                hashCaller = pCommit->mapContracts[pairContract];

                return true;
            }
        }
        else if(nFlags == TAO::Ledger::FLAGS::MINER)
        {
            LOCK(MEMORY_MUTEX);

            /* Check for a memory transaction first */
            if(pMiner && pMiner->mapContracts.count(pairContract))
            {
                /* Get the state from temporary transaction. */
                hashCaller = pMiner->mapContracts[pairContract];

                return true;
            }
        }

        /* Check our disk state first. */
        return Read(pairContract, hashCaller);
    }


    /* Checks if a contract is already fulfilled. */
    bool ContractDB::HasContract(const std::pair<uint512_t, uint32_t>& pairContract, const uint8_t nFlags)
    {
        /* Memory mode for pre-database commits. */
        if(nFlags == TAO::Ledger::FLAGS::MEMPOOL)
        {
            LOCK(MEMORY_MUTEX);

            /* Set the state in the memory map. */
            if(pMemory && pMemory->mapContracts.count(pairContract))
                return true;

            /* Check commited memory. */
            if(pCommit->mapContracts.count(pairContract))
                return true;
        }
        else if(nFlags == TAO::Ledger::FLAGS::MINER)
        {
            LOCK(MEMORY_MUTEX);

            /* Check pending transaction memory. */
            if(pMiner && pMiner->mapContracts.count(pairContract))
                return true;
        }

        /* Check our disk index first. */
        if(Exists(pairContract))
            return true;

        /* Additional routine if the proof doesn't exist. */
        if(config::fClient.load() && nFlags == TAO::Ledger::FLAGS::LOOKUP)
        {
            /* Check for -client mode or active server object. */
            if(!LLP::TRITIUM_SERVER || !LLP::LOOKUP_SERVER)
                throw debug::exception(FUNCTION, "tritium or lookup servers inactive");

            /* Try to find a connection first. */
            std::shared_ptr<LLP::LookupNode> pConnection = LLP::LOOKUP_SERVER->GetConnection();
            if(pConnection == nullptr)
            {
                /* Attempt to get an active tritium connection for lookup. */
                std::shared_ptr<LLP::TritiumNode> pNode = LLP::TRITIUM_SERVER->GetConnection();
                if(pNode != nullptr)
                {
                    /* Get our lookup address now. */
                    const std::string strAddress =
                        pNode->GetAddress().ToStringIP();

                    /* Make our new connection now. */
                    if(!LLP::LOOKUP_SERVER->ConnectNode(strAddress, pConnection))
                        throw debug::exception(FUNCTION, "failed to connect to node");
                }
            }

            /* Check that we were able to make a connection. */
            if(!pConnection)
                throw debug::exception(FUNCTION, "no connections found");

            /* Debug output to console. */
            debug::log(2, FUNCTION, "CLIENT MODE: Requesting ACTION::GET::CONTRACT for ", pairContract.first.SubString());
            pConnection->BlockingLookup
            (
                5000,
                LLP::LookupNode::REQUEST::PROOF,
                uint8_t(LLP::LookupNode::SPECIFIER::CONTRACT),
                pairContract.first, pairContract.second
            );
            debug::log(2, FUNCTION, "CLIENT MODE: TYPES::CONTRACT received for ", pairContract.first.SubString());

            /* Return if the proof exists or not now. */
            return Exists(pairContract);
        }

        return false;
    }

    /* Begin a memory transaction following ACID properties. */
    void ContractDB::MemoryBegin(const uint8_t nFlags)
    {
        LOCK(MEMORY_MUTEX);

        /* Check for miner. */
        if(nFlags == TAO::Ledger::FLAGS::MINER)
        {
            /* Set the pre-commit memory mode. */
            if(pMiner)
                delete pMiner;

            pMiner = new ContractTransaction();

            return;
        }

        /* Set the pre-commit memory mode. */
        if(pMemory)
            delete pMemory;

        pMemory = new ContractTransaction();
    }


    /* Abort a memory transaction following ACID properties. */
    void ContractDB::MemoryRelease(const uint8_t nFlags)
    {
        LOCK(MEMORY_MUTEX);

        /* Check for miner. */
        if(nFlags == TAO::Ledger::FLAGS::MINER)
        {
            /* Set the pre-commit memory mode. */
            if(pMiner)
                delete pMiner;

            pMiner = nullptr;

            return;
        }

        /* Set the pre-commit memory mode. */
        if(pMemory)
            delete pMemory;

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
            delete pMemory;
            pMemory = nullptr;
        }
    }
}
