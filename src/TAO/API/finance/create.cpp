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

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        /* Create a NXS account. */
        json::json Finance::Create(const json::json& params, bool fHelp)
        {
            /* Generate a random hash for this objects register address */
            const TAO::Register::Address hashRegister =
                TAO::Register::Address(TAO::Register::Address::ACCOUNT);

            /* If this is not a NXS token account, verify that the token identifier is for a valid token */
            const TAO::Register::Address hashToken = ExtractToken(params);
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
            TAO::Register::Object account = TAO::Register::CreateAccount(hashToken);

            /* If the user has supplied the data parameter than add this to the account register */
            if(params.find("data") != params.end())
                account << std::string("data") << uint8_t(TAO::Register::TYPES::STRING) << params["data"].get<std::string>();

            /* Submit the payload object. */
            std::vector<TAO::Operation::Contract> vContracts(1);
            vContracts[0] << uint8_t(TAO::Operation::OP::CREATE) << hashRegister << uint8_t(TAO::Register::REGISTER::OBJECT) << account.GetState();

            /* Check for name parameter. If one is supplied then we need to create a Name Object register for it. */
            if(params.find("name") != params.end() && !params["name"].get<std::string>().empty())
            {
                /* Add an optional name if supplied. */
                vContracts.push_back
                (
                    Names::CreateName(users->GetSession(params).GetAccount()->Genesis(),
                    params["name"].get<std::string>(), "", hashRegister)
                );
            }

            /* Build a JSON response object. */
            json::json jRet;
            jRet["txid"]    = BuildAndAccept(params, vContracts).ToString();
            jRet["address"] = hashRegister.ToString();

            return jRet;
        }
    }
}
