/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/types/register.h>

namespace LLD
{

    /** The Database Constructor. To determine file location and the Bytes per Record. **/
    RegisterDB::RegisterDB(const uint8_t nFlagsIn, const uint32_t nBucketsIn, const uint32_t nCacheIn)
    : SectorDatabase("registers"
    , nFlagsIn
    , nBucketsIn
    , nCacheIn)

    , MEMORY_MUTEX()
    , mapStates()
    , mapIdentifiers()
    {
    }


    /* Default Destructor */
    RegisterDB::~RegisterDB()
    {
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

            mapStates[hashRegister] = state;

            return true;
        }
        else if(nFlags == TAO::Ledger::FLAGS::BLOCK)
        {
            LOCK(MEMORY_MUTEX);

            /* Remove the memory state if writing the disk state. */
            if(mapStates.count(hashRegister))
            {
                /* Check for most recent memory state, and remove if writing it. */
                const TAO::Register::State& stateCheck = mapStates[hashRegister];
                if(stateCheck == state)
                {
                    debug::log(0, FUNCTION, "NOTICE: deleting matching state");
                    mapStates.erase(hashRegister);
                }
            }

        }

        return Write(std::make_pair(std::string("state"), hashRegister), state);
    }


    /* Read a state register from the register database. */
    bool RegisterDB::ReadState(const uint256_t& hashRegister, TAO::Register::State& state, const uint8_t nFlags)
    {
        /* Memory mode for pre-database commits. */
        if(nFlags == TAO::Ledger::FLAGS::MEMPOOL)
        {
            LOCK(MEMORY_MUTEX);

            /* Check for state in memory map. */
            if(mapStates.count(hashRegister))
            {
                state = mapStates[hashRegister];

                return true;
            }
        }

        return Read(std::make_pair(std::string("state"), hashRegister), state);
    }


    /* Erase a state register from the register database. */
    bool RegisterDB::EraseState(const uint256_t& hashRegister, const uint8_t nFlags)
    {
        /* Memory mode for pre-database commits. */
        if(nFlags == TAO::Ledger::FLAGS::MEMPOOL)
        {
            LOCK(MEMORY_MUTEX);

            /* Check for state in memory map. */
            if(mapStates.count(hashRegister))
                mapStates.erase(hashRegister);
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
        return Write(std::make_pair(std::string("genesis"), hashGenesis), state);
    }


    /* Index a genesis to a register address. */
    bool RegisterDB::ReadTrust(const uint256_t& hashGenesis, TAO::Register::State& state)
    {
        return Read(std::make_pair(std::string("genesis"), hashGenesis), state);
    }


    /* Erase a genesis from a register address. */
    bool RegisterDB::EraseTrust(const uint256_t& hashGenesis)
    {
        return Erase(std::make_pair(std::string("genesis"), hashGenesis));
    }


    /* Determines if a state exists in the register database. */
    bool RegisterDB::HasState(const uint256_t& hashRegister, const uint8_t nFlags)
    {
        /* Memory mode for pre-database commits. */
        if(nFlags == TAO::Ledger::FLAGS::MEMPOOL)
        {
            LOCK(MEMORY_MUTEX);

            /* Check for state in memory map. */
            if(mapStates.count(hashRegister))
                return true;
        }

        return Exists(std::make_pair(std::string("state"), hashRegister));
    }
}
