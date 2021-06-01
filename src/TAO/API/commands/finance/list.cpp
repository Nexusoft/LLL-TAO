/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/types/commands/finance.h>
#include <TAO/API/objects/types/objects.h>

#include <TAO/API/include/build.h>
#include <TAO/API/include/check.h>
#include <TAO/API/include/extract.h>
#include <TAO/API/include/list.h>
#include <TAO/API/include/get.h>
#include <TAO/API/include/json.h>
#include <TAO/API/types/commands.h>

#include <TAO/API/users/types/users.h>
#include <TAO/API/types/commands/finance.h>

#include <TAO/Ledger/types/sigchain.h>

#include <TAO/Register/types/object.h>

#include <Util/include/debug.h>


/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        /* Get a list of accounts owned by a signature chain. */
        json::json Finance::List(const json::json& jParams, const bool fHelp)
        {
            /* Get our genesis-id for this call. */
            const uint256_t hashGenesis =
                Commands::Get<Users>()->GetSession(jParams).GetAccount()->Genesis();

            /* Number of results to return. */
            uint32_t nLimit = 100, nOffset = 0;

            /* Sort order to apply */
            std::string strOrder = "desc";

            /* Get the jParams to apply to the response. */
            ExtractList(jParams, strOrder, nLimit, nOffset);

            /* The token to filter on.  Default to 0 (NXS) */
            const uint256_t hashToken = ExtractToken(jParams);

            /* Get the list of registers owned by this sig chain */
            std::vector<TAO::Register::Address> vAccounts;
            if(!ListAccounts(hashGenesis, vAccounts, false, true))
                throw APIException(-74, "No registers found");

            /* Build our response json object. */
            json::json jRet;

            /* Add the register data to the response */
            uint32_t nTotal = 0;
            for(const auto& hashRegister : vAccounts)
            {
                /* Check the offset. */
                if(++nTotal <= nOffset)
                    continue;

                /* Get the token / account object. */
                TAO::Register::Object objThis;
                if(!LLD::Register->ReadObject(hashRegister, objThis, TAO::Ledger::FLAGS::LOOKUP))
                    continue;

                /* Check that our type matches our noun. */
                if(!CheckType(jParams, objThis))
                    continue;

                /* Check the account matches the filter */
                if(objThis.get<uint256_t>("token") != hashToken)
                    continue;

                /* Check to see whether the transaction has had all children filtered out */
                json::json jObj = TAO::API::ObjectToJSON(jParams, objThis, hashRegister);
                if(jObj.empty())
                    continue;

                /* Add to our return json. */
                jRet.push_back(jObj);

                /* Check the limit */
                if(nTotal - nOffset > nLimit)
                    break;
            }

            return jRet;
        }


        /* Lists all transactions for a given account. */
        json::json Finance::ListTransactions(const json::json& jParams, const bool fHelp)
        {
            /* The account to list transactions for. */
            const TAO::Register::Address hashAccount = ExtractAddress(jParams);

            /* Get the account object. */
            TAO::Register::Object object;
            if(!LLD::Register->ReadObject(hashAccount, object))
                throw APIException(-13, "Object not found");

            /* Check the object standard. */
            if(object.Base() != TAO::Register::OBJECTS::ACCOUNT)
                throw APIException(-65, "Object is not an account");

            return Objects::ListTransactions(jParams, fHelp);
        }
    }
}
