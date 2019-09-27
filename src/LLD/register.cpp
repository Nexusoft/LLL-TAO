/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/types/register.h>

#include <TAO/Register/include/enum.h>

namespace LLD
{

    /** The Database Constructor. To determine file location and the Bytes per Record. **/
    RegisterDB::RegisterDB(const uint8_t nFlagsIn, const uint32_t nBucketsIn, const uint32_t nCacheIn)
    : SectorDatabase("registers"
    , nFlagsIn
    , nBucketsIn
    , nCacheIn)

    , MEMORY_MUTEX()
    , pMemory(nullptr)
    , pMiner(nullptr)
    , pCommit(new RegisterTransaction())
    {
    }


    /* Default Destructor */
    RegisterDB::~RegisterDB()
    {
        /* Cleanup memory transactions. */
        if(pMemory)
            delete pMemory;

        /* Cleanup memory transactions. */
        if(pMiner)
            delete pMiner;

        /* Cleanup commited states. */
        if(pCommit)
            delete pCommit;
    }


    /*  Writes a state register to the register database.
     *  If MEMPOOL flag is set, this will write state into a temporary
     *  memory to handle register state sequencing before blocks commit. */
    bool RegisterDB::WriteState(const uint256_t& hashRegister, const TAO::Register::State& state, const uint8_t nFlags)
    {
        /* Memory mode for pre-database commits. */
        if(nFlags == TAO::Ledger::FLAGS::MEMPOOL)
        {
            LOCK(MEMORY_MUTEX);

            /* Check for memory mode. */
            if(pMemory)
            {
                /* Commit to transactional memory. */
                pMemory->mapStates[hashRegister] = state;

                return true;
            }

            /* Otherwise commit like normal. */
            pCommit->mapStates[hashRegister] = state;

            return true;
        }
        else if(nFlags == TAO::Ledger::FLAGS::MINER)
        {
            LOCK(MEMORY_MUTEX);

            /* Check for memory mode. */
            if(pMiner)
                pMiner->mapStates[hashRegister] = state;

            return true;
        }
        else if(nFlags == TAO::Ledger::FLAGS::BLOCK)
        {
            LOCK(MEMORY_MUTEX);

            /* Remove the memory state if writing the disk state. */
            if(pCommit->mapStates.count(hashRegister))
            {
                /* Check for most recent memory state, and remove if writing it. */
                const TAO::Register::State& stateCheck = pCommit->mapStates[hashRegister];
                if(stateCheck == state)
                    pCommit->mapStates.erase(hashRegister);
            }
        }

        /* Add sequential read keys for known address types. */
        std::string strType = "NONE";
        switch(hashRegister.GetType())
        {
            case TAO::Register::Address::ACCOUNT :
                strType = "account";
                break;

            case TAO::Register::Address::APPEND :
                strType = "append";
                break;

            case TAO::Register::Address::CRYPTO :
                strType = "crypto";
                break;

            case TAO::Register::Address::NAME :
                strType = "name";
                break;

            case TAO::Register::Address::NAMESPACE :
                strType = "namespace";
                break;

            case TAO::Register::Address::OBJECT :
                strType = "object";
                break;

            case TAO::Register::Address::RAW :
                strType = "raw";
                break;

            case TAO::Register::Address::READONLY :
                strType = "readonly";
                break;

            case TAO::Register::Address::TOKEN :
                strType = "token";
                break;

            case TAO::Register::Address::TRUST :
                strType = "trust";
                break;

            default :
                strType = "NONE";
        }

        /* Write the state to the register database */
        return Write(std::make_pair(std::string("state"), hashRegister), state, strType);
    }


    /* Read a state register from the register database. */
    bool RegisterDB::ReadState(const uint256_t& hashRegister, TAO::Register::State& state, const uint8_t nFlags)
    {
        /* Memory mode for pre-database commits. */
        if(nFlags == TAO::Ledger::FLAGS::MEMPOOL)
        {
            LOCK(MEMORY_MUTEX);

            /* Check for a memory transaction first */
            if(pMemory && pMemory->mapStates.count(hashRegister))
            {
                /* Get the state from temporary transaction. */
                state = pMemory->mapStates[hashRegister];

                return true;
            }

            /* Check for state in memory map. */
            if(pCommit->mapStates.count(hashRegister))
            {
                /* Get the state from commited memory. */
                state = pCommit->mapStates[hashRegister];

                return true;
            }
        }
        else if(nFlags == TAO::Ledger::FLAGS::MINER)
        {
            LOCK(MEMORY_MUTEX);

            /* Check for a memory transaction first */
            if(pMiner && pMiner->mapStates.count(hashRegister))
            {
                /* Get the state from temporary transaction. */
                state = pMiner->mapStates[hashRegister];

                return true;
            }
        }

        return Read(std::make_pair(std::string("state"), hashRegister), state);
    }


    /* Erase a state register from the register database. */
    bool RegisterDB::EraseState(const uint256_t& hashRegister, const uint8_t nFlags)
    {
        /* Memory mode for pre-database commits. */
        LOCK(MEMORY_MUTEX);

        /* Check for memory transaction. */
        if(pMemory && nFlags == TAO::Ledger::FLAGS::MEMPOOL)
        {
            /* Check for available states. */
            bool fExists = false;
            if(pMemory->mapStates.count(hashRegister))
            {
                /* Erase state out of transaction. */
                pMemory->mapStates.erase(hashRegister);

                fExists = true;
            }

            /* Check that value exists to erase. */
            if(pCommit->mapStates.count(hashRegister))
            {
                /* Set data to be in erase queue. */
                pMemory->setErase.insert(hashRegister);

                fExists = true;
            }

            return fExists;
        }

        /* Check for state in memory map. */
        if(pCommit->mapStates.count(hashRegister))
        {
            /* Erase the states. */
            pCommit->mapStates.erase(hashRegister);

            /* If in memory only mode, break early. */
            if(nFlags == TAO::Ledger::FLAGS::MEMPOOL)
                return true;
        }

        return Erase(std::make_pair(std::string("state"), hashRegister));
    }


    /* Index a genesis to a register address. */
    bool RegisterDB::IndexTrust(const uint256_t& hashGenesis, const uint256_t& hashRegister)
    {
        return Index(std::make_pair(std::string("genesis"), hashGenesis), std::make_pair(std::string("state"), hashRegister));
    }


    /* Check that a genesis doesn't already exist. */
    bool RegisterDB::HasTrust(const uint256_t& hashGenesis)
    {
        return Exists(std::make_pair(std::string("genesis"), hashGenesis));
    }


    /* Write a genesis to a register address. */
    bool RegisterDB::WriteTrust(const uint256_t& hashGenesis, const TAO::Register::State& state)
    {
        /* Get trust account address for contract caller */
        uint256_t hashRegister =
            TAO::Register::Address(std::string("trust"), hashGenesis, TAO::Register::Address::TRUST);

        /* Check for block to delete memory state. */
        if(nFlags == TAO::Ledger::FLAGS::BLOCK)
        {
            LOCK(MEMORY_MUTEX);

            /* Remove the memory state if writing the disk state. */
            if(pCommit->mapStates.count(hashRegister))
            {
                /* Check for most recent memory state, and remove if writing it. */
                const TAO::Register::State& stateCheck = pCommit->mapStates[hashRegister];
                if(stateCheck == state)
                    pCommit->mapStates.erase(hashRegister);
            }
        }

        return Write(std::make_pair(std::string("genesis"), hashGenesis), state, "trust");
    }


    /* Index a genesis to a register address. */
    bool RegisterDB::ReadTrust(const uint256_t& hashGenesis, TAO::Register::State& state)
    {
        /* Get trust account address for contract caller */
        uint256_t hashRegister =
            TAO::Register::Address(std::string("trust"), hashGenesis, TAO::Register::Address::TRUST);

        /* Memory mode for pre-database commits. */
        if(nFlags == TAO::Ledger::FLAGS::MEMPOOL)
        {
            LOCK(MEMORY_MUTEX);

            /* Check for a memory transaction first */
            if(pMemory && pMemory->mapStates.count(hashRegister))
            {
                /* Get the state from temporary transaction. */
                state = pMemory->mapStates[hashRegister];

                return true;
            }

            /* Check for state in memory map. */
            if(pCommit->mapStates.count(hashRegister))
            {
                /* Get the state from commited memory. */
                state = pCommit->mapStates[hashRegister];

                return true;
            }
        }
        else if(nFlags == TAO::Ledger::FLAGS::MINER)
        {
            LOCK(MEMORY_MUTEX);

            /* Check for a memory transaction first */
            if(pMiner && pMiner->mapStates.count(hashRegister))
            {
                /* Get the state from temporary transaction. */
                state = pMiner->mapStates[hashRegister];

                return true;
            }
        }

        return Read(std::make_pair(std::string("genesis"), hashGenesis), state);
    }


    /* Erase a genesis from a register address. */
    bool RegisterDB::EraseTrust(const uint256_t& hashGenesis)
    {
        /* Get trust account address for contract caller */
        uint256_t hashRegister =
            TAO::Register::Address(std::string("trust"), hashGenesis, TAO::Register::Address::TRUST);

        /* Check for memory transaction. */
        if(pMemory && nFlags == TAO::Ledger::FLAGS::MEMPOOL)
        {
            /* Check for available states. */
            bool fExists = false;
            if(pMemory->mapStates.count(hashRegister))
            {
                /* Erase state out of transaction. */
                pMemory->mapStates.erase(hashRegister);

                fExists = true;
            }

            /* Check that value exists to erase. */
            if(pCommit->mapStates.count(hashRegister))
            {
                /* Set data to be in erase queue. */
                pMemory->setErase.insert(hashRegister);

                fExists = true;
            }

            return fExists;
        }

        /* Check for state in memory map. */
        if(pCommit->mapStates.count(hashRegister))
        {
            /* Erase the states. */
            pCommit->mapStates.erase(hashRegister);

            /* If in memory only mode, break early. */
            if(nFlags == TAO::Ledger::FLAGS::MEMPOOL)
                return true;
        }

        return Erase(std::make_pair(std::string("genesis"), hashGenesis));
    }


    /* Determines if a state exists in the register database. */
    bool RegisterDB::HasState(const uint256_t& hashRegister, const uint8_t nFlags)
    {
        /* Memory mode for pre-database commits. */
        if(nFlags == TAO::Ledger::FLAGS::MEMPOOL)
        {
            LOCK(MEMORY_MUTEX);

            /* Check internal memory state. */
            if(pMemory && pMemory->mapStates.count(hashRegister))
                return true;

            /* Check for state in memory map. */
            if(pCommit->mapStates.count(hashRegister))
                return true;
        }
        else if(nFlags == TAO::Ledger::FLAGS::MINER)
        {
            LOCK(MEMORY_MUTEX);

            /* Check internal memory state. */
            if(pMiner && pMiner->mapStates.count(hashRegister))
                return true;
        }

        return Exists(std::make_pair(std::string("state"), hashRegister));
    }

    /* Begin a memory transaction following ACID properties. */
    void RegisterDB::MemoryBegin(const uint8_t nFlags)
    {
        LOCK(MEMORY_MUTEX);

        /* Check for mempool. */
        if(nFlags == TAO::Ledger::FLAGS::MEMPOOL)
        {
            /* Set the pre-commit memory mode. */
            if(pMemory)
                delete pMemory;

            pMemory = new RegisterTransaction();

            return;
        }

        /* Check for miner. */
        if(nFlags == TAO::Ledger::FLAGS::MINER)
        {
            /* Set the pre-commit memory mode. */
            if(pMiner)
                delete pMiner;

            pMiner = new RegisterTransaction();

            return;
        }
    }


    /* Abort a memory transaction following ACID properties. */
    void RegisterDB::MemoryRelease(const uint8_t nFlags)
    {
        LOCK(MEMORY_MUTEX);

        /* Check for mempool. */
        if(nFlags == TAO::Ledger::FLAGS::MEMPOOL)
        {
            /* Set the pre-commit memory mode. */
            if(pMemory)
                delete pMemory;

            pMemory = nullptr;

            return;
        }

        /* Check for miner. */
        if(nFlags == TAO::Ledger::FLAGS::MINER)
        {
            /* Set the pre-commit memory mode. */
            if(pMiner)
                delete pMiner;

            pMiner = nullptr;

            return;
        }
    }


    /* Commit a memory transaction following ACID properties. */
    void RegisterDB::MemoryCommit()
    {
        LOCK(MEMORY_MUTEX);

        /* Abort the current memory mode. */
        if(pMemory)
        {
            /* Loop through all new states and apply to commit data. */
            for(const auto& state : pMemory->mapStates)
                pCommit->mapStates[state.first] = state.second;

            /* Loop through values to erase. */
            for(const auto& erase : pMemory->setErase)
                pCommit->mapStates.erase(erase);

            /* Free the memory. */
            delete pMemory;
            pMemory = nullptr;
        }
    }
}
