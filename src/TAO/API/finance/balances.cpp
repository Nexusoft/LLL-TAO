
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

#include <TAO/Ledger/include/constants.h>
#include <TAO/Ledger/types/sigchain.h>

#include <TAO/Register/types/object.h>

#include <Util/include/debug.h>


/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {
        typedef struct
        {
            /* The confirmed balance from the state at the last block*/
            uint64_t nBalance = 0;

            /* The available balance including mempool transactions (outgoing debits) */
            uint64_t nAvailable = 0;

            /* The sum of all debits that are confirmed but not credited */
            uint64_t nPending = 0;

            /* The sum of all incoming debits that are not yet confirmed or credits we have made that are not yet confirmed*/
            uint64_t nUnconfirmed = 0;

            /* The sum of all unconfirmed outcoing debits */
            uint64_t nUnconfirmedOutgoing = 0;

            /* The amount currently being staked */
            uint64_t nStake = 0;

            /* The sum of all immature coinbase transactions */
            uint64_t nImmature = 0;

            /* The decimals used for this token for display purposes */
            uint8_t nDecimals = 0;

        } TokenBalance;
        

        /* Get a summary of balance information across all accounts belonging to the currently logged in signature chain
           for a particular token type */
        json::json Finance::GetBalances(const json::json& params, bool fHelp)
        {
            json::json ret;

            /* Get the session to be used for this API call */
            Session& session = users->GetSession(params);

            /* Get the account. */
            const memory::encrypted_ptr<TAO::Ledger::SignatureChain>& user = session.GetAccount();
            if(!user)
                throw APIException(-10, "Invalid session ID");

            /* The user genesis hash */
            uint256_t hashGenesis = user->Genesis();

            /* The token to return balances for. Default to 0 (NXS) */
            TAO::Register::Address hashToken;

            /* See if the caller has requested a particular token type */
            if(params.find("token_name") != params.end() && !params["token_name"].get<std::string>().empty() && params["token_name"].get<std::string>() != "NXS")
                /* If name is provided then use this to deduce the register address */
                hashToken = Names::ResolveAddress(params, params["token_name"].get<std::string>());
            /* Otherwise try to find the raw hex encoded address. */
            else if(params.find("token") != params.end() && IsRegisterAddress(params["token"]))
                hashToken.SetBase58(params["token"]);

            /* The balances for this token type*/
            TokenBalance balance;

            /* The trust account */
            TAO::Register::Object trust;

            /* First get the list of registers owned by this sig chain so we can work out which ones are NXS accounts */
            std::vector<TAO::Register::Address> vRegisters;
            if(!ListRegisters(hashGenesis, vRegisters))
                throw APIException(-74, "No registers found");

            /* Iterate through each register we own */
            for(const auto& hashRegister : vRegisters)
            {
                /* Initial check that it is an account/trust/token, before we hit the DB to get the balance */
                if(!hashRegister.IsAccount() && !hashRegister.IsTrust() && !hashRegister.IsToken())
                    continue;

                /* Get the register from the register DB */
                TAO::Register::Object object;
                if(!LLD::Register->ReadState(hashRegister, object)) // note we don't include mempool state here as we want the confirmed
                    continue;

                /* Check that this is a non-standard object type so that we can parse it and check the type*/
                if(object.nType != TAO::Register::REGISTER::OBJECT)
                    continue;

                /* parse object so that the data fields can be accessed */
                if(!object.Parse())
                    continue;

                /* Check that this is an account */
                if(object.Base() != TAO::Register::OBJECTS::ACCOUNT )
                    continue;

                /* Check that it is for the correct token */
                if(object.get<uint256_t>("token") != hashToken)
                    continue;

                /* Cache this if it is the trust account */
                if(object.Standard() == TAO::Register::OBJECTS::TRUST)
                    trust = object;

                /* Increment the balance */
                balance.nBalance += object.get<uint64_t>("balance");

                /* Cache the decimals for this token to use for display */
                balance.nDecimals = GetDecimals(object);
            }

            /* Find all pending debits to the token accounts */
            balance.nPending = GetPending(hashGenesis, hashToken);

            /* Get unconfirmed debits coming in and credits we have made */
            balance.nUnconfirmed = GetUnconfirmed(hashGenesis, hashToken, false);

            /* Get all new debits that we have made */
            balance.nUnconfirmedOutgoing = GetUnconfirmed(hashGenesis, hashToken, true);

            /* Calculate the available balance which is the last confirmed balance minus and mempool debits */
            balance.nAvailable = balance.nBalance - balance.nUnconfirmedOutgoing;

            /* Get immature mined / staked */
            balance.nImmature = GetImmature(hashGenesis);

            /* Get the stake amount */
            if(!trust.IsNull())
                balance.nStake = trust.get<uint64_t>("stake");

            /* Populate the response object */

            /* Resolve the name of the token name */
            std::string strToken = hashToken != 0 ? Names::ResolveName(hashGenesis, hashToken) : "NXS";
            if(!strToken.empty())
                ret["token_name"] = strToken;

            /* Add the token identifier */
            ret["token"] = hashToken.ToString();

            ret["available"] = (double)balance.nAvailable / pow(10, balance.nDecimals);
            ret["pending"] = (double)balance.nPending / pow(10, balance.nDecimals);
            ret["unconfirmed"] = (double)balance.nUnconfirmed / pow(10, balance.nDecimals);

            /* Add stake/immature for NXS only */
            if(hashToken == 0)
            {
                ret["stake"] = (double)balance.nStake / pow(10, balance.nDecimals);
                ret["immature"] = (double)balance.nImmature / pow(10, balance.nDecimals);
            }

            return ret;
        }


        /* Get a summary of balance information across all accounts belonging to the currently logged in signature chain */
        json::json Finance::ListBalances(const json::json& params, bool fHelp)
        {
            json::json ret = json::json::array();

            /* Get the session to be used for this API call */
            Session& session = users->GetSession(params);

            /* Get the account. */
            const memory::encrypted_ptr<TAO::Ledger::SignatureChain>& user = session.GetAccount();
            if(!user)
                throw APIException(-10, "Invalid session ID");

            /* The user genesis hash */
            uint256_t hashGenesis = user->Genesis();

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

            /* token register hash */
            uint256_t hashToken;

            /* map of token types to balances */
            std::map<uint256_t, TokenBalance> vTokenBalances;

            /* The trust account */
            TAO::Register::Object trust;

            /* First get the list of registers owned by this sig chain so we can work out which ones are NXS accounts */
            std::vector<TAO::Register::Address> vRegisters;
            if(!ListRegisters(hashGenesis, vRegisters))
                throw APIException(-74, "No registers found");

            /* Iterate through each register we own */
            for(const auto& hashRegister : vRegisters)
            {
                /* Initial check that it is an account/trust/token, before we hit the DB to get the balance */
                if(!hashRegister.IsAccount() && !hashRegister.IsTrust() && !hashRegister.IsToken())
                    continue;

                /* Get the register from the register DB */
                TAO::Register::Object object;
                if(!LLD::Register->ReadState(hashRegister, object)) // note we don't include mempool state here as we want the confirmed
                    continue;

                /* Check that this is a non-standard object type so that we can parse it and check the type*/
                if(object.nType != TAO::Register::REGISTER::OBJECT)
                    continue;

                /* parse object so that the data fields can be accessed */
                if(!object.Parse())
                    continue;

                /* Check that this is an account */
                if(object.Base() != TAO::Register::OBJECTS::ACCOUNT )
                    continue;

                /* Get the token */
                hashToken = object.get<uint256_t>("token");

                /* Cache this if it is the trust account */
                if(object.Standard() == TAO::Register::OBJECTS::TRUST)
                    trust = object;

                /* Increment the balance */
                vTokenBalances[hashToken].nBalance += object.get<uint64_t>("balance");

                /* Cache the decimals for this token to use for display */
                vTokenBalances[hashToken].nDecimals = GetDecimals(object);
            }

            /* Iterate through each token and get the pending/unconfirmed etc  */
            uint32_t nTotal = 0;
            for(const auto& token : vTokenBalances)
            {
                /* Get the token hash for this token */
                hashToken = token.first;

                /* Find all pending debits to NXS accounts */
                vTokenBalances[hashToken].nPending = GetPending(hashGenesis, hashToken);

                /* Get unconfirmed debits coming in and credits we have made */
                vTokenBalances[hashToken].nUnconfirmed = GetUnconfirmed(hashGenesis, hashToken, false);

                /* Get all new debits that we have made */
                vTokenBalances[hashToken].nUnconfirmedOutgoing = GetUnconfirmed(hashGenesis, hashToken, true);

                /* Calculate the available balance which is the last confirmed balance minus and mempool debits */
                vTokenBalances[hashToken].nAvailable = vTokenBalances[hashToken].nBalance - vTokenBalances[hashToken].nUnconfirmedOutgoing;

                /* Get immature mined / staked */
                vTokenBalances[hashToken].nImmature = GetImmature(hashGenesis);

                /* Get the stake amount */
                if(!trust.IsNull())
                    vTokenBalances[hashToken].nStake = trust.get<uint64_t>("stake");


                /* Populate the response object */
                json::json jsonBalances;

                /* Resolve the name of the token name */
                std::string strToken = hashToken != 0 ? Names::ResolveName(hashGenesis, hashToken) : "NXS";
                if(!strToken.empty())
                    jsonBalances["token_name"] = strToken;

                /* Add the token identifier */
                jsonBalances["token"] = hashToken.ToString();

                jsonBalances["available"] = (double)(vTokenBalances[hashToken].nAvailable / pow(10, vTokenBalances[hashToken].nDecimals));
                jsonBalances["pending"] = (double)(vTokenBalances[hashToken].nPending / pow(10, vTokenBalances[hashToken].nDecimals));
                jsonBalances["unconfirmed"] = (double)(vTokenBalances[hashToken].nUnconfirmed / pow(10, vTokenBalances[hashToken].nDecimals));

                /* Add stake/immature for NXS only */
                if(hashToken == 0)
                {
                    jsonBalances["stake"] = (double)(vTokenBalances[hashToken].nStake / pow(10, vTokenBalances[hashToken].nDecimals));
                    jsonBalances["immature"] = (double)(vTokenBalances[hashToken].nImmature / pow(10, vTokenBalances[hashToken].nDecimals));
                }

                /* Check to see that it matches the where clauses */
                if(fHasFilter)
                {
                    /* Skip this top level record if not all of the filters were matched */
                    if(!MatchesWhere(jsonBalances, vWhere[""]))
                        continue;
                }

                ++nTotal;

                /* Check the offset. */
                if(nTotal <= nOffset)
                    continue;

                /* Check the limit */
                if(nTotal - nOffset > nLimit)
                    break;

                ret.push_back(jsonBalances);
            }

            return ret;
        }
    }
}
