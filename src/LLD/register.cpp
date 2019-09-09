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
        if(nFlags >= TAO::Ledger::FLAGS::MEMPOOL)
        {
            LOCK(MEMORY_MUTEX);

            /* Set the state in the memory map. */
            uint32_t nConflict = nFlags - TAO::Ledger::FLAGS::MEMPOOL;
            if(!mapStates.count(hashRegister))
                mapStates[hashRegister] = { state };

            /* Check that the diff index is correct. */
            std::vector<TAO::Register::State>& vStates = mapStates[hashRegister];
            if(nConflict > vStates.size())
                throw debug::exception(FUNCTION, "conflict ", nConflict, " out of sequence ", vStates.size());

            /* Check if there is a new conflict. */
            if(nConflict == vStates.size())
            {
                debug::error(FUNCTION, "REGISTER CONFLICT: ", nConflict, " hash ", hashRegister.SubString());
                vStates.push_back(state);
            }

            /* Set state conflict by id. */
            vStates[nConflict] = state;

            return true;
        }
        else if(nFlags == TAO::Ledger::FLAGS::BLOCK)
        {
            LOCK(MEMORY_MUTEX);

            /* Remove the memory state if writing the disk state. */
            if(mapStates.count(hashRegister))
                mapStates.erase(hashRegister);
        }

        return Write(std::make_pair(std::string("state"), hashRegister), state);
    }


    /* Read a state register from the register database. */
    bool RegisterDB::ReadState(const uint256_t& hashRegister, TAO::Register::State& state, const uint8_t nFlags)
    {
        /* Memory mode for pre-database commits. */
        if(nFlags >= TAO::Ledger::FLAGS::MEMPOOL)
        {
            LOCK(MEMORY_MUTEX);

            /* Check for state in memory map. */
            if(mapStates.count(hashRegister))
            {
                /* Get the memory map access iterator. */
                uint32_t nConflict = nFlags - TAO::Ledger::FLAGS::MEMPOOL;

                /* Check that the diff index is correct. */
                std::vector<TAO::Register::State>& vStates = mapStates[hashRegister];
                if(nConflict >= vStates.size())
                    throw debug::exception(FUNCTION, "conflict ", nConflict, " out of range ", vStates.size());

                /* Return correct conflict by id. */
                state = vStates[nConflict];

                return true;
            }
        }

        return Read(std::make_pair(std::string("state"), hashRegister), state);
    }


    /* Erase a state register from the register database. */
    bool RegisterDB::EraseState(const uint256_t& hashRegister, const uint8_t nFlags)
    {
        /* Memory mode for pre-database commits. */
        if(nFlags >= TAO::Ledger::FLAGS::MEMPOOL)
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


    /* Writes a token identifier to the register database. */
    bool RegisterDB::WriteIdentifier(const uint256_t nIdentifier, const uint256_t& hashRegister, const uint8_t nFlags)
    {
        /* Memory mode for pre-database commits. */
        if(nFlags == TAO::Ledger::FLAGS::MEMPOOL)
        {
            LOCK(MEMORY_MUTEX);

            mapIdentifiers[nIdentifier] = hashRegister;

            return true;
        }
        else if(nFlags == TAO::Ledger::FLAGS::BLOCK)
        {
            LOCK(MEMORY_MUTEX);

            /* Remove the memory state if writing the disk state. */
            if(mapIdentifiers.count(nIdentifier))
                mapIdentifiers.erase(nIdentifier);
        }

        return Write(std::make_pair(std::string("token"), nIdentifier), hashRegister);
    }


    /* Erase a token identifier to the register database. */
    bool RegisterDB::EraseIdentifier(const uint256_t nIdentifier)
    {
        return Erase(std::make_pair(std::string("token"), nIdentifier));
    }


    /* Read a token identifier from the register database. */
    bool RegisterDB::ReadIdentifier(const uint256_t nIdentifier, uint256_t& hashRegister, const uint8_t nFlags)
    {
        /* Memory mode for pre-database commits. */
        if(nFlags == TAO::Ledger::FLAGS::MEMPOOL)
        {
            LOCK(MEMORY_MUTEX);

            /* Return the state if it is found. */
            if(mapIdentifiers.count(nIdentifier))
            {
                hashRegister = mapIdentifiers[nIdentifier];

                return true;
            }
        }

        return Read(std::make_pair(std::string("token"), nIdentifier), hashRegister);
    }


    /*  Determines if an identifier exists in the database. */
    bool RegisterDB::HasIdentifier(const uint256_t nIdentifier, const uint8_t nFlags)
    {
        /* Memory mode for pre-database commits. */
        if(nFlags == TAO::Ledger::FLAGS::MEMPOOL)
        {
            LOCK(MEMORY_MUTEX);

            /* Return the state if it is found. */
            if(mapIdentifiers.count(nIdentifier))
                return true;
        }

        return Exists(std::make_pair(std::string("token"), nIdentifier));
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
