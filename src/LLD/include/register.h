/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++

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
#include <LLD/templates/filemap.h>

#include <TAO/Register/include/state.h>

namespace LLD
{

    class RegisterDB : public SectorDatabase<BinaryFileMap>
    {
    public:
        /** The Database Constructor. To determine file location and the Bytes per Record. **/
        RegisterDB(const char* pszMode="r+") : SectorDatabase("registers", pszMode) {}

        bool WriteState(uint256_t hashRegister, TAO::Register::State state)
        {
            return Write(std::make_pair(std::string("state"), hashRegister), state);
        }

        bool ReadState(uint256_t hashRegister, TAO::Register::State& state)
        {
            return Read(std::make_pair(std::string("state"), hashRegister), state);
        }
    };
}

#endif
