/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/hash/SK.h>

#include <LLD/include/global.h>

#include <TAO/API/include/build.h>
#include <TAO/API/include/check.h>
#include <TAO/API/include/global.h>

#include <TAO/Operation/include/enum.h>
#include <TAO/Operation/include/execute.h>

#include <TAO/Register/include/enum.h>
#include <TAO/Register/include/create.h>
#include <TAO/Register/types/object.h>
#include <TAO/Register/types/address.h>

#include <TAO/Ledger/include/create.h>
#include <TAO/Ledger/types/mempool.h>
#include <TAO/Ledger/types/sigchain.h>

#include <Util/templates/datastream.h>
#include <Util/include/math.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Create a NXS account. */
    json::json Finance::Create(const json::json& jParams, bool fHelp)
    {
        /* Check that we have designated a type to create. */
        TAO::Register::Address hashRegister;
        if(jParams.find("type") == jParams.end())
            throw APIException(-118, "Missing type");

        /* Check for account or token type. */
        TAO::Register::Object object;
        if(jParams["type"] == "token")
        {
            /* Generate register address for token. */
            hashRegister = TAO::Register::Address(TAO::Register::Address::TOKEN);

            /* Check for supply parameter. */
            if(jParams.find("supply") == jParams.end())
                throw APIException(-119, "Missing supply");

            /* Extract the supply parameter */
            uint64_t nSupply = 0;
            if(jParams.find("supply") != jParams.end())
            {
                /* Attempt to convert the supplied value to a 64-bit unsigned integer, catching argument/range exceptions */
                try  { nSupply = std::stoull(jParams["supply"].get<std::string>()); }
                catch(const std::invalid_argument& e)
                {
                    throw APIException(-175, "Invalid supply amount. Supply must be whole number value");
                }
                catch(const std::out_of_range& e)
                {
                    throw APIException(-176, "Invalid supply amount. The maximum token supply is 18446744073709551615");
                }
            }

            /* Check for nDecimals parameter. */
            uint8_t nDecimals = 0;
            if(jParams.find("decimals") != jParams.end())
            {
                /* Attempt to convert the supplied value catching argument / range exceptions */
                bool fValid = false;
                try
                {
                    nDecimals = std::stoul(jParams["decimals"].get<std::string>());
                    fValid = (nDecimals <= 8);
                }
                catch(const std::exception& e) { fValid = false; }

                /* Check for errors. */
                if(!fValid)
                    throw APIException(-177, "Invalid decimals amount.  Decimals must be whole number value between 0 and 8");
            }

            /* Sanitize the supply/decimals combination for uint64 overflow */
            const uint64_t nCoinFigures = math::pow(10, nDecimals);
            if(nDecimals > 0 && nSupply > (std::numeric_limits<uint64_t>::max() / nCoinFigures))
                throw APIException(-178, "Invalid supply/decimals. The maximum supply/decimals cannot exceed 18446744073709551615");

            /* Create a token object register. */
            object = TAO::Register::CreateToken(hashRegister, nSupply * nCoinFigures, nDecimals);
        }
        else if(jParams["type"] == "account")
        {
            /* Generate register address for account. */
            hashRegister = TAO::Register::Address(TAO::Register::Address::ACCOUNT);

            /* If this is not a NXS token account, verify that the token identifier is for a valid token */
            const TAO::Register::Address hashToken = ExtractToken(jParams);
            if(hashToken != 0)
            {
                /* Check our address before hitting the database. */
                if(hashToken.GetType() != TAO::Register::Address::TOKEN)
                    throw APIException(-212, "Invalid token");

                /* Get the register off the disk. */
                TAO::Register::Object object;
                if(!LLD::Register->ReadObject(hashToken, object, TAO::Ledger::FLAGS::MEMPOOL))
                    throw APIException(-125, "Token not found");

                /* Check the standard */
                if(object.Standard() != TAO::Register::OBJECTS::TOKEN)
                    throw APIException(-212, "Invalid token");
            }

            /* Create an account object register. */
            object = TAO::Register::CreateAccount(hashToken);
        }
        else
            throw APIException(-36, "Invalid type for command");

        /* If the user has supplied the data parameter than add this to the account register */
        if(jParams.find("data") != jParams.end())
            object << std::string("data") << uint8_t(TAO::Register::TYPES::STRING) << jParams["data"].get<std::string>();

        /* Submit the payload object. */
        std::vector<TAO::Operation::Contract> vContracts(1);
        vContracts[0] << uint8_t(TAO::Operation::OP::CREATE)      << hashRegister;
        vContracts[0] << uint8_t(TAO::Register::REGISTER::OBJECT) << object.GetState();

        /* Check for name parameter. If one is supplied then we need to create a Name Object register for it. */
        if(jParams.find("name") != jParams.end() && !jParams["name"].get<std::string>().empty())
        {
            /* Add an optional name if supplied. */
            vContracts.push_back
            (
                Names::CreateName(users->GetSession(jParams).GetAccount()->Genesis(),
                jParams["name"].get<std::string>(), "", hashRegister)
            );
        }

        /* Build response JSON boilerplate. */
        return BuildResponse(jParams, hashRegister, vContracts);
    }
}
