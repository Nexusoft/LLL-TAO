/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/ledger/types/ledger.h>
#include <TAO/API/include/global.h>

#include <LLD/include/global.h>

#include <TAO/API/include/utils.h>
#include <TAO/API/include/json.h>

#include <TAO/Ledger/include/constants.h>
#include <TAO/Ledger/include/stake.h>

#include <TAO/Register/types/address.h>
#include <TAO/Register/types/object.h>


/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {
        /* Retrieves the transaction for the given hash. */
        json::json Finance::TrustAccounts(const json::json& params, bool fHelp)
        {
            /* The return json array */
            json::json jsonRet = json::json::array();

            /* Number of results to return. */
            uint32_t nLimit = 100;

            /* Offset into the result set to return results from */
            uint32_t nOffset = 0;

            /* Order to apply */
            std::string strOrder = "desc";

            /* Vector of where clauses to apply to filter the results */
            std::map<std::string, std::vector<Clause>> vWhere;

            /* Get the params to apply to the response. */
            GetListParams(params, strOrder, nLimit, nOffset, vWhere);

            /* Sort order to apply */
            std::string strSort = "trust";
            if(params.find("sort") != params.end())
                strSort = params["sort"].get<std::string>();

            /* The vector of trust accounts */
            std::vector<TAO::Register::Object> vAccounts;

            /* Timestamp of 30 days ago, to use to include active accounts */
            uint64_t nActive = runtime::unifiedtimestamp() - (60 * 60 * 24 * 30);

            /* The vector of active accounts */
            std::vector<TAO::Register::Object> vActive;

            /* Batch read up to 100,000 at a time*/
            if(LLD::Register->BatchRead("trust", vAccounts, 100000))
            {
                /* Make sure they are all parsed so that we can sort them */
                for(auto& account : vAccounts)
                {
                    /* Only include active within last 30 days */
                    if(account.nModified < nActive)
                        continue;

                    /* Parse so we can access the data */
                    account.Parse();

                    /* Only include accounts with stake or trust (genesis has stake w/o trust; can unstake to trust w/o stake) */
                    if(account.get<uint64_t>("stake") == 0 && account.get<uint64_t>("trust") == 0)
                        continue;

                    /* Add the account to our active list */
                    vActive.push_back(account);
                }
            }

            /* Sort the list */
            bool fDesc = strOrder == "desc";
            if(strSort == "stake" || strSort == "balance" || strSort == "trust")
                std::sort(vActive.begin(), vActive.end(), [strSort, fDesc]
                        (const TAO::Register::Object &a, const TAO::Register::Object &b)
                {
                    /* Sort in decending/ascending order based on order param */
                    if(fDesc)
                        return ( a.get<uint64_t>(strSort) > b.get<uint64_t>(strSort) );
                    else
                        return ( a.get<uint64_t>(strSort) < b.get<uint64_t>(strSort) );
                });

            /* Flag indicating there are top level filters  */
            bool fHasFilter = vWhere.count("") > 0;

            /* Iterate the list and build the response */
            uint32_t nTotal = 0;
            for(auto& account : vActive)
            {
                /* The JSON for this account */
                json::json jsonAccount;

                /* The register address */
                TAO::Register::Address address("trust", account.hashOwner, TAO::Register::Address::TRUST);

                /* Populate the response */
                jsonAccount["address"] = address.ToString();
                jsonAccount["owner"] = account.hashOwner.ToString();
                jsonAccount["created"] = account.nCreated;
                jsonAccount["modified"] = account.nModified;
                jsonAccount["balance"] = (double) account.get<uint64_t>("balance") / pow(10, TAO::Ledger::NXS_DIGITS);
                jsonAccount["stake"] = (double) account.get<uint64_t>("stake") / pow(10, TAO::Ledger::NXS_DIGITS);

                /* Calculate and add the stake rate */
                uint64_t nTrust = account.get<uint64_t>("trust");
                jsonAccount["trust"] = nTrust;
                jsonAccount["stakerate"] = TAO::Ledger::StakeRate(nTrust, (nTrust == 0)) * 100.0;

                /* Check to see that it matches the where clauses */
                if(fHasFilter)
                {
                    /* Skip this top level record if not all of the filters were matched */
                    if(!MatchesWhere(jsonAccount, vWhere[""]))
                        continue;
                }

                ++nTotal;

                /* Check the offset. */
                if(nTotal <= nOffset)
                    continue;
                
                /* Check the limit */
                if(nTotal - nOffset > nLimit)
                    break;

                jsonRet.push_back(jsonAccount);

            }


            return jsonRet;

        }

    }
}
