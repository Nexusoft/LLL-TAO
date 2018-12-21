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

        bool WriteState(uint256_t hashRegister, TAO::Register::State state)
        {
            return Write(std::make_pair(std::string("state"), hashRegister), state);
        }

        bool ReadState(uint256_t hashRegister, TAO::Register::State& state)
        {
            return Read(std::make_pair(std::string("state"), hashRegister), state);
        }

        bool HasState(uint256_t hashRegister)
        {
            return Exists(std::make_pair(std::string("state"), hashRegister));
        }

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
