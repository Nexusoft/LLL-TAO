/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/


#include <LLC/types/uint1024.h>

#include <LLD/include/contract.h>


namespace LLD
{
    /** The Database Constructor.  **/
    ContractDB::ContractDB(uint8_t nFlags)
    : SectorDatabase("contracts", nFlags)
    , MEMORY_MUTEX()
    , mapContracts()
    {
    }


    /** Default Destructor **/
    ContractDB::~ContractDB()
    {
    }


    /* Writes a caller that fulfilled a conditional agreement.*/
    bool ContractDB::WriteContract(const std::pair<uint512_t, uint32_t>& pairContract,
                                   const uint256_t& hashCaller, const uint8_t nFlags)
    {
        /* Memory mode for pre-database commits. */
        if(nFlags == TAO::Ledger::FLAGS::MEMPOOL)
        {
            LOCK(MEMORY_MUTEX);

            /* Set the state in the memory map. */
            mapContracts[pairContract] = hashCaller;

            return true;
        }
        else if(nFlags == TAO::Ledger::FLAGS::BLOCK)
        {
            LOCK(MEMORY_MUTEX);

            /* Remove the memory state if writing the disk state. */
            if (mapContracts.count(pairContract))
                mapContracts.erase(pairContract);
        }

        return Write(pairContract, hashCaller);
    }


    /* Writes a caller that fulfilled a conditional agreement.*/
    bool ContractDB::EraseContract(const std::pair<uint512_t, uint32_t>& pairContract)
    {
        LOCK(MEMORY_MUTEX);

        /* Remove the memory state if writing the disk state. */
        if (mapContracts.count(pairContract))
            mapContracts.erase(pairContract);

        return Erase(pairContract);
    }


    /* Reads a caller that fulfilled a conditional agreement.*/
    bool ContractDB::ReadContract(const std::pair<uint512_t, uint32_t>& pairContract, uint256_t& hashCaller, const uint8_t nFlags)
    {
        /* Memory mode for pre-database commits. */
        if(nFlags == TAO::Ledger::FLAGS::MEMPOOL)
        {
            LOCK(MEMORY_MUTEX);

            /* Set the state in the memory map. */
            if(mapContracts.count(pairContract))
            {
                /* Get the state from memory map. */
                hashCaller = mapContracts[pairContract];

                return true;
            }
        }

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
            if(mapContracts.count(pairContract))

                return true;
        }

        return Exists(pairContract);
    }
}
