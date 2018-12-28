/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_LLD_INCLUDE_REGISTER_H
#define NEXUS_LLD_INCLUDE_REGISTER_H

#include <LLC/types/uint1024.h>

#include <LLD/include/version.h>
#include <LLD/templates/sector.h>

#include <LLD/cache/binary_lru.h>
#include <LLD/keychain/hashmap.h>

#include <TAO/Register/include/state.h>

namespace LLD
{

    class RegisterDB : public SectorDatabase<BinaryHashMap, BinaryLRU>
    {
    public:
        /** The Database Constructor. To determine file location and the Bytes per Record. **/
        RegisterDB(const char* pszMode="r+")
        : SectorDatabase("registers", pszMode) {}


        /** Write state
         *
         *  Writes a state register to the register database.
         *
         *  @param[in] hashRegister The register address.
         *  @param[in] state The state register to write.
         *
         *  @return true if write was successul.
         *
         **/
        bool WriteState(uint256_t hashRegister, TAO::Register::State state)
        {
            return Write(std::make_pair(std::string("state"), hashRegister), state);
        }


        /** Read state
         *
         *  Read a state register from the register database.
         *
         *  @param[in] hashRegister The register address.
         *  @param[out] state The state register to read.
         *
         *  @return true if read was successul.
         *
         **/
        bool ReadState(uint256_t hashRegister, TAO::Register::State& state)
        {
            return Read(std::make_pair(std::string("state"), hashRegister), state);
        }


        /** Write Identifier
         *
         *  Writes a token identifier to the register database
         *
         *  @param[in] nIdentifier The token identifier.
         *  @param[in] hashRegister The register address of token.
         *
         *  @return true if write was successul.
         *
         **/
        bool WriteIdentifier(uint32_t nIdentifier, uint256_t hashRegister)
        {
            return Write(std::make_pair(std::string("token"), nIdentifier), hashRegister);
        }


        /** Read Identifier
         *
         *  Read a token identifier from the register database
         *
         *  @param[in] nIdentifier The token identifier.
         *  @param[out] hashRegister The register address of token.
         *
         *  @return true if write was successul.
         *
         **/
        bool ReadIdentifier(uint32_t nIdentifier, uint256_t& hashRegister)
        {
            return Read(std::make_pair(std::string("token"), nIdentifier), hashRegister);
        }


        /** Has Identifier
         *
         *  Determine if an identifier exists in the database
         *
         *  @param[in] nIdentifier The token identifier.
         *
         *  @return true if it exists
         *
         **/
        bool HasIdentifier(uint32_t nIdentifier)
        {
            return Exists(std::make_pair(std::string("token"), nIdentifier));
        }


        /** Has state
         *
         *  Check if a state exists in the register database
         *
         *  @param[in] hashRegister The register address.
         *
         *  @return true if it exists.
         *
         **/
        bool HasState(uint256_t hashRegister)
        {
            return Exists(std::make_pair(std::string("state"), hashRegister));
        }


        /** Get States
         *
         *  Get the previous states of a register
         *
         *  @param[in] hashRegister The register address.
         *  @param[out] states The states vector to return.
         *
         *  @return true if any states were found.
         *
         **/
        bool GetStates(uint256_t hashRegister, std::vector<TAO::Register::State>& states)
        {
            /* Serialize the key to search for. */
            DataStream ssKey(SER_LLD, DATABASE_VERSION);
            ssKey << std::make_pair(std::string("state"), hashRegister);

            /* Get the list of sector keys. */
            std::vector<SectorKey> vKeys;
            if(!pSectorKeys->Get(static_cast<std::vector<uint8_t>>(ssKey), vKeys))
                return false;

            /* Iterate the list of keys. */
            for(auto & key : vKeys)
            {
                DataStream ssData(SER_LLD, DATABASE_VERSION);
                if(!Get(key, ssData))
                    continue;

                TAO::Register::State state;
                ssData >> state;

                states.push_back(state);
            }

            return (states.size() > 0);
        }
    };
}

#endif
