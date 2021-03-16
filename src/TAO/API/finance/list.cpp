/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

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
            {
                std::string strName = params["token_name"].get<std::string>();
                if(strName != "NXS")
                    /* If name is provided then use this to deduce the register address */
                    hashToken = Names::ResolveAddress(params, strName);
            }
            /* Otherwise try to find the raw hex encoded address. */
            else if(params.find("token") != params.end())
            {
                std::string strToken = params["token"];
                if(strToken != "0" && IsRegisterAddress(strToken))
                    hashToken.SetBase58(params["token"]);
            }

            /* Get the list of registers owned by this sig chain */
            std::vector<std::pair<TAO::Register::Address, TAO::Register::Object>> vAccounts;
            if(!ListAccounts(user->Genesis(), false, true, vAccounts))
                throw APIException(-74, "No registers found");

            /* Add the register data to the response */
            uint32_t nTotal = 0;
            for(const auto& account : vAccounts)
            {
                /* Get the object register from the list */
                TAO::Register::Object object = account.second;

                /* Check that this is an account */
                uint8_t nStandard = object.Standard();
                if(nStandard != TAO::Register::OBJECTS::ACCOUNT && nStandard != TAO::Register::OBJECTS::TRUST)
                    continue;

                /* Check the account matches the filter */
                if(object.get<uint256_t>("token") != hashToken)
                    continue;

                json::json obj = TAO::API::ObjectToJSON(params, object, account.first);

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


        /* Optimized version to list all NXS accounts but without supporting the JSON filtering . */
        json::json Finance::ListFast(const json::json& params, bool fHelp)
        {
            /* JSON return value. */
            json::json ret;;

            /* Get the session to be used for this API call */
            Session& session = users->GetSession(params);

            /* Get the account. */
            const memory::encrypted_ptr<TAO::Ledger::SignatureChain>& user = session.GetAccount();
            if(!user)
                throw APIException(-10, "Invalid session ID"); 

            /* The token to filter on.  Default to 0 (NXS) */
            TAO::Register::Address hashToken;

            /* Check for data parameter. */
            if(params.find("token_name") != params.end() && !params["token_name"].get<std::string>().empty())
            {
                std::string strName = params["token_name"].get<std::string>();
                if(strName != "NXS")
                    /* If name is provided then use this to deduce the register address */
                    hashToken = Names::ResolveAddress(params, strName);
            }
            /* Otherwise try to find the raw hex encoded address. */
            else if(params.find("token") != params.end())
            {
                std::string strToken = params["token"];
                if(strToken != "0" && IsRegisterAddress(strToken))
                    hashToken.SetBase58(params["token"]);
            }

            /* Get the list of registers owned by this sig chain */
            std::vector<std::pair<TAO::Register::Address, TAO::Register::Object>> vAccounts;
            if(!ListAccounts(user->Genesis(), false, true, vAccounts))
                throw APIException(-74, "No registers found");

            /* Add the register data to the response */
            for(const auto& account : vAccounts)
            {
                /* Get the object register from the list */
                TAO::Register::Object object = account.second;

                /* Check that this is an account */
                uint8_t nStandard = object.Standard();
                if(nStandard != TAO::Register::OBJECTS::ACCOUNT && nStandard != TAO::Register::OBJECTS::TRUST)
                    continue;

                /* Check the account matches the filter */
                if(object.get<uint256_t>("token") != hashToken)
                    continue;

                /* JSON for this account */
                json::json obj;

                obj["address"] = account.first.ToString();

                /* Get the token names. */
                std::string strTokenName = Names::ResolveAccountTokenName(params, object);
                if(!strTokenName.empty())
                    obj["token_name"] = strTokenName;

                /* Set the value to the token contract address. */
                TAO::Register::Address hashToken = object.get<uint256_t>("token");
                obj["token"] = hashToken.ToString();

                /* Handle digit conversion. */
                uint8_t nDecimals = GetDecimals(object);

                /* In order to get the balance for this account we need to ensure that we use the state from disk, which will 
                   have the confirmed balance.  If this is a new account then it won't be on disk so the confirmed balance is 0 */
                uint64_t nConfirmedBalance = 0;
                TAO::Register::Object accountConfirmed;
                if(LLD::Register->ReadState(account.first, accountConfirmed))
                {
                    /* Parse the object register. */
                    if(!accountConfirmed.Parse())
                        throw APIException(-14, "Object failed to parse");

                    nConfirmedBalance = accountConfirmed.get<uint64_t>("balance");
                }

                /* Get unconfirmed debits coming in and credits we have made */
                uint64_t nUnconfirmed = GetUnconfirmed(object.hashOwner, hashToken, false, account.first);

                /* Get all new debits that we have made */
                uint64_t nUnconfirmedOutgoing = GetUnconfirmed(object.hashOwner, hashToken, true, account.first);

                /* Calculate the available balance which is the last confirmed balance minus and mempool debits */
                uint64_t nAvailable = nConfirmedBalance - nUnconfirmedOutgoing;

                obj["balance"] = (double)nAvailable / pow(10, nDecimals);
                obj["unconfirmed"] = (double)nUnconfirmed / pow(10, nDecimals);

                /* Add stake amount if this is a trust account. */
                if(nStandard == TAO::Register::OBJECTS::TRUST)
                    ret["stake"] = (double)object.get<uint64_t>("stake") / pow(10, nDecimals);

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
