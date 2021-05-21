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

#include <TAO/API/finance/types/finance.h>

#include <TAO/Operation/include/enum.h>

#include <TAO/Ledger/include/create.h>
#include <TAO/Ledger/types/mempool.h>

#include <Util/include/math.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* This debits the account to send the coins back to the token address, but does so with a condition that never is true. */
    json::json Finance::Burn(const json::json& jParams, bool fHelp)
    {
        /* The sending account or token. */
        const TAO::Register::Address hashRegister = ExtractAddress(jParams);

        /* First let's check for our type noun. */
        if(jParams.find("type") == jParams.end())
            throw APIException(-118, "Missing type");

        /* Now let's do some checks here to prevent burning something you don't want */
        if(jParams["type"] != "token")
            throw APIException(-36, "Invalid type for command");

        /* Get the token / account object. */
        TAO::Register::Object object;
        if(!LLD::Register->ReadObject(hashRegister, object, TAO::Ledger::FLAGS::MEMPOOL))
            throw APIException(-122, "Token/account not found");

        /* Check that we are operating on an account. */
        if(object.Standard() != TAO::Register::OBJECTS::ACCOUNT)
            throw APIException(-65, "Object is not an account");

        /* Make sure we aren't burning any NXS, no need to burn it, ever! */
        const uint256_t hashToken = object.get<uint256_t>("token");
        if(hashToken == 0)
            throw APIException(-212, "Invalid token");

        /* Check for amount parameter. */
        if(jParams.find("amount") == jParams.end())
            throw APIException(-46, "Missing amount");

        /* Check the amount is not too small once converted by the token Decimals */
        const uint64_t nAmount = std::stod(jParams["amount"].get<std::string>()) * GetFigures(object);
        if(nAmount == 0)
            throw APIException(-68, "Amount too small");

        /* Check they have the required funds */
        if(nAmount > object.get<uint64_t>("balance"))
            throw APIException(-69, "Insufficient funds");

        /* The optional payment reference */
        uint64_t nReference = 0;
        if(jParams.find("reference") != jParams.end())
            nReference = stoull(jParams["reference"].get<std::string>());

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
