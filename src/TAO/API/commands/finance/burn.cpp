/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/include/build.h>
#include <TAO/API/include/extract.h>
#include <TAO/API/include/get.h>

#include <TAO/API/types/commands/finance.h>

#include <TAO/Operation/include/enum.h>

#include <TAO/Ledger/include/create.h>
#include <TAO/Ledger/types/mempool.h>

#include <Util/include/math.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* This debits the account to send the coins back to the token address, but does so with a condition that never is true. */
    encoding::json Finance::Burn(const encoding::json& jParams, const bool fHelp)
    {
        /* The sending account or token. */
        const TAO::Register::Address hashRegister = ExtractAddress(jParams);

        /* Now let's do some checks here to prevent burning something you don't want */
        const std::string strType = ExtractType(jParams);
        if(strType != "account") //we only want to allow burning from account levels
            throw Exception(-36, "Unsupported type [", strType, "] for command");

        /* Get the token / account object. */
        TAO::Register::Object object;
        if(!LLD::Register->ReadObject(hashRegister, object, TAO::Ledger::FLAGS::MEMPOOL))
            throw Exception(-122, "Token/account not found");

        /* Check that we are operating on an account. */
        if(object.Standard() != TAO::Register::OBJECTS::ACCOUNT)
            throw Exception(-65, "Object is not an account");

        /* Make sure we aren't burning any NXS, no need to burn it, ever! */
        const uint256_t hashToken = object.get<uint256_t>("token");
        if(hashToken == 0)
            throw Exception(-212, "Invalid token");

        /* Check they have the required funds */
        const uint64_t nAmount = ExtractAmount(jParams, GetFigures(object));
        if(nAmount > object.get<uint64_t>("balance"))
            throw Exception(-69, "Insufficient funds");

        /* The optional payment reference */
        const uint64_t nReference = ExtractNumber<uint64_t>(jParams, "reference", 0); //0: as our default parameter

        /* Submit the payload object. */
        std::vector<TAO::Operation::Contract> vContracts(1);
        vContracts[0] << (uint8_t)TAO::Operation::OP::DEBIT << hashRegister << hashToken << nAmount << nReference;

        /* Add the burn conditions.  This is a simple condition that will never evaluate to true */
        vContracts[0] <= uint8_t(TAO::Operation::OP::TYPES::UINT16_T) <= uint16_t(4);
        vContracts[0] <= uint8_t(TAO::Operation::OP::EQUALS);
        vContracts[0] <= uint8_t(TAO::Operation::OP::TYPES::UINT16_T) <= uint16_t(2);

        /* Build response JSON boilerplate. */
        return BuildResponse(jParams, hashRegister, vContracts);
    }
}
