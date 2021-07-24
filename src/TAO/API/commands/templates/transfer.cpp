/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/include/build.h>
#include <TAO/API/include/check.h>
#include <TAO/API/include/conditions.h>
#include <TAO/API/include/extract.h>

#include <TAO/API/users/types/users.h>

#include <TAO/API/types/commands/templates.h>
#include <TAO/API/types/commands.h>

#include <TAO/Operation/include/enum.h>

#include <TAO/Operation/types/contract.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Claim an incoming transfer from recipient. */
    encoding::json Templates::Transfer(const encoding::json& jParams, const bool fHelp)
    {
        /* Get our genesis-id for this call. */
        const uint256_t hashGenesis =
            Commands::Get<Users>()->GetSession(jParams).GetAccount()->Genesis();

        /* Extract some parameters from input data. */
        const uint256_t hashRegister  = ExtractAddress(jParams);
        const uint256_t hashRecipient = ExtractRecipient(jParams);

        /* Check that the destination exists. */
        if(!LLD::Ledger->HasGenesis(hashRecipient))
            throw Exception(-113, "Destination user doesn't exist");

        /* Check out our object now. */
        TAO::Register::Object tObject;
        if(!LLD::Register->ReadObject(hashRegister, tObject))
            throw Exception(-13, "Object not found");

        /* Now lets check our expected types match. */
        if(!CheckStandard(jParams, tObject))
            throw Exception(-49, "Unsupported type for name/address");

        /* Build our contract now that we've validated. */
        std::vector<TAO::Operation::Contract> vContracts(1);
        vContracts[0] << uint8_t(TAO::Operation::OP::TRANSFER) << hashRegister;
        vContracts[0] << hashRecipient << uint8_t(TAO::Operation::TRANSFER::CLAIM);

        /* Add our conditional contract. */
        AddExpires(jParams, hashGenesis, vContracts[0], false);

        /* Build response JSON boilerplate. */
        return BuildResponse(jParams, hashRegister, vContracts);
    }
}
