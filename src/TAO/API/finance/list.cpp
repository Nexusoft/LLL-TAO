/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2021

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/finance/types/finance.h>
#include <TAO/API/objects/types/objects.h>
#include <TAO/API/include/global.h>

#include <TAO/API/include/utils.h>
#include <TAO/API/include/json.h>

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
        json::json Finance::List(const json::json& params, bool fHelp)
        {
            /* JSON return value. */
            json::json ret;// = json::json::array();

            /* Get the session to be used for this API call */
            Session& session = users->GetSession(params);

            /* Get the account. */
            const memory::encrypted_ptr<TAO::Ledger::SignatureChain>& user = session.GetAccount();
            if(!user)
                throw APIException(-10, "Invalid session ID");

            /* Number of results to return. */
            uint32_t nLimit = 100;

            /* Offset into the result set to return results from */
            uint32_t nOffset = 0;

            /* Sort order to apply */
            std::string strOrder = "desc";

            /* Vector of where clauses to apply to filter the results */
            std::map<std::string, std::vector<Clause>> vWhere;

            /* Get the params to apply to the response. */
            GetListParams(params, strOrder, nLimit, nOffset, vWhere);

            /* Flag indicating there are top level filters  */
            bool fHasFilter = vWhere.count("") > 0;

            /* Fields to ignore in the where clause.  This is necessary so that the count param is not treated as 
               standard where clauses to filter the json */
            std::vector<std::string> vIgnore = {"count"};

            /* The token to filter on.  Default to 0 (NXS) */
            TAO::Register::Address hashToken;

            /* Check for data parameter. */
            if(params.find("token_name") != params.end() && !params["token_name"].get<std::string>().empty())
                /* If name is provided then use this to deduce the register address */
                hashToken = Names::ResolveAddress(params, params["token_name"].get<std::string>());
            /* Otherwise try to find the raw hex encoded address. */
            else if(params.find("token") != params.end() && IsRegisterAddress(params["token"]))
                hashToken.SetBase58(params["token"]);

            /* Get the list of registers owned by this sig chain */
            std::vector<TAO::Register::Address> vAccounts;
            if(!ListAccounts(user->Genesis(), vAccounts, false, true))
                throw APIException(-74, "No registers found");

            /* Read all the registers to that they are sorted by creation time */
            std::vector<std::pair<TAO::Register::Address, TAO::Register::State>> vRegisters;
            GetRegisters(vAccounts, vRegisters);

            /* Add the register data to the response */
            uint32_t nTotal = 0;
            for(const auto& state : vRegisters)
            {
                /* Double check that it is an object before we cast it */
                if(state.second.nType != TAO::Register::REGISTER::OBJECT)
                    continue;

                /* Cast the state to an Object register */
                TAO::Register::Object object(state.second);

                /* Check that this is a non-standard object type so that we can parse it and check the type*/
                if(object.nType != TAO::Register::REGISTER::OBJECT)
                    continue;

                /* parse object so that the data fields can be accessed */
                if(!object.Parse())
                    throw APIException(-36, "Failed to parse object register");

                /* Check that this is an account */
                uint8_t nStandard = object.Standard();
                if(nStandard != TAO::Register::OBJECTS::ACCOUNT && nStandard != TAO::Register::OBJECTS::TRUST)
                    continue;

                /* Check the account matches the filter */
                if(object.get<uint256_t>("token") != hashToken)
                    continue;

                json::json obj = TAO::API::ObjectToJSON(params, object, state.first);

                /* Check to see whether the transaction has had all children filtered out */
                if(obj.empty())
                    continue;

                /* Check to see that it matches the where clauses */
                if(fHasFilter)
                {
                    /* Skip this top level record if not all of the filters were matched */
                    if(!MatchesWhere(obj, vWhere[""], vIgnore))
                        continue;
                }

                ++nTotal;

                /* Check the offset. */
                if(nTotal <= nOffset)
                    continue;
                
                /* Check the limit */
                if(nTotal - nOffset > nLimit)
                    break;

                ret.push_back(obj);
            }

            return ret;
        }

        /* Lists all transactions for a given account. */
        json::json Finance::ListTransactions(const json::json& params, bool fHelp)
        {
            /* The account to list transactions for. */
            TAO::Register::Address hashAccount;

            /* If name is provided then use this to deduce the register address,
             * otherwise try to find the raw hex encoded address. */
            if(params.find("name") != params.end())
                hashAccount = Names::ResolveAddress(params, params["name"].get<std::string>());
            else if(params.find("address") != params.end())
                hashAccount.SetBase58(params["address"].get<std::string>());
            else
                throw APIException(-33, "Missing name or address");

            /* Get the account object. */
            TAO::Register::Object object;
            if(!LLD::Register->ReadState(hashAccount, object))
                throw APIException(-13, "Account not found");

            /* Parse the object register. */
            if(!object.Parse())
                throw APIException(-14, "Object failed to parse");

            /* Get the object standard. */
            uint8_t nStandard = object.Standard();

            /* Check the object standard. */
            if(nStandard != TAO::Register::OBJECTS::ACCOUNT && nStandard != TAO::Register::OBJECTS::TRUST)
                throw APIException(-65, "Object is not an account");

            return Objects::ListTransactions(params, fHelp);
        }
    }
}
