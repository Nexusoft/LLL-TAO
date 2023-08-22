/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2023

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/types/commands.h>
#include <TAO/API/types/commands/assets.h>

#include <TAO/API/include/build.h>
#include <TAO/API/include/check.h>
#include <TAO/API/include/extract.h>

#include <TAO/Operation/include/enum.h>

#include <TAO/Register/include/enum.h>


/* Global TAO namespace. */
namespace TAO::API
{
    /* Tokenize an asset into fungible tokens that represent ownership. */
    encoding::json Assets::Tokenize(const encoding::json& jParams, const bool fHelp)
    {
        /* Get the register addresses. */
        const TAO::Register::Address hashToken    = ExtractToken(jParams);
        const TAO::Register::Address hashRegister = ExtractAddress(jParams);

        /* Check for token. */
        TAO::Register::Object tToken;
        if(!LLD::Register->ReadObject(hashToken, tToken))
            throw Exception(-13, "Object not found");

        /* Check for valid type. */
        if(tToken.Standard() != TAO::Register::OBJECTS::TOKEN)
            throw Exception(-35, "Invalid Parameter [token]");

        /* Validate the asset */
        TAO::Register::Object tObject;
        if(!LLD::Register->ReadState(hashRegister, tObject, TAO::Ledger::FLAGS::MEMPOOL))
            throw Exception(-13, "Object not found");

        /* Now lets check our expected types match. */
        if(!CheckStandard(jParams, tObject))
            throw Exception(-49, "Unsupported type for name/address");

        /* Get our contracts payload.*/
        std::vector<TAO::Operation::Contract> vContracts(1);

        /* Serialize the contract. */
        vContracts[0] << uint8_t(TAO::Operation::OP::TRANSFER) << hashRegister;
        vContracts[0] << hashToken << uint8_t(TAO::Operation::TRANSFER::FORCE);

        return BuildResponse(jParams, hashRegister, vContracts);
    }
}
