/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/include/build.h>
#include <TAO/API/include/global.h>
#include <TAO/API/include/get.h>

#include <TAO/Operation/include/enum.h>

#include <TAO/Ledger/include/create.h>
#include <TAO/Ledger/types/mempool.h>

#include <Util/include/math.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* This debits the account to send the coins back to the token address, but does so with a condition that never is true. */
    json::json Tokens::Burn(const json::json& params, bool fHelp)
    {
        /* The sending account or token. */
        const TAO::Register::Address hashBurn = ExtractAddress(params);

        /* Get the token / account object. */
        TAO::Register::Object object;
        if(!LLD::Register->ReadObject(hashBurn, object, TAO::Ledger::FLAGS::MEMPOOL))
            throw APIException(-122, "Token/account not found");

        /* Check that we are operating on an account. */
        if(object.Standard() != TAO::Register::OBJECTS::ACCOUNT)
            throw APIException(-65, "Object is not an account");

        /* Check for amount parameter. */
        if(params.find("amount") == params.end())
            throw APIException(-46, "Missing amount");

        /* Check the amount is not too small once converted by the token Decimals */
        const uint64_t nAmount = std::stod(params["amount"].get<std::string>()) * GetFigures(object);
        if(nAmount == 0)
            throw APIException(-68, "Amount too small");

        /* Check they have the required funds */
        if(nAmount > object.get<uint64_t>("balance"))
            throw APIException(-69, "Insufficient funds");

        /* The optional payment reference */
        uint64_t nReference = 0;
        if(params.find("reference") != params.end())
            nReference = stoull(params["reference"].get<std::string>());

        /* Submit the payload object. */
        std::vector<TAO::Operation::Contract> vContracts(1);
        vContracts[0] << (uint8_t)TAO::Operation::OP::DEBIT << hashBurn << object.get<uint256_t>("token") << nAmount << nReference;

        /* Add the burn conditions.  This is a simple condition that will never evaluate to true */
        vContracts[0] <= uint8_t(TAO::Operation::OP::TYPES::UINT16_T) <= uint16_t(108+105+102+101);
        vContracts[0] <= uint8_t(TAO::Operation::OP::EQUALS);
        vContracts[0] <= uint8_t(TAO::Operation::OP::TYPES::UINT16_T) <= uint16_t(42);

        /* Build a JSON response object. */
        json::json ret;
        ret["txid"] = BuildAndAccept(params, vContracts).ToString();

        return ret;
    }
}
