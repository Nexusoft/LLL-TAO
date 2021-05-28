/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/types/commands/register.h>

#include <TAO/API/include/json.h>
#include <TAO/API/include/format.h>
#include <TAO/API/include/get.h>
#include <TAO/API/types/exception.h>

#include <TAO/Ledger/include/stake.h>

#include <TAO/Register/types/address.h>
#include <TAO/Register/types/object.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Retrieves the transaction for the given hash. */
    json::json Register::List(const json::json& jParams, const bool fHelp)
    {
        /* Check that there is a type defined here. */
        if(jParams["request"].find("type") == jParams["request"].end())
            throw APIException(-36, "Invalid type for command.");

        /* Grab our type to run some checks against. */
        const std::string strType =
            jParams["request"]["type"].get<std::string>();

        /* Number of results to return. */
        uint32_t nLimit = 100, nOffset = 0;

        /* Get the params to apply to the response. */
        std::string strOrder = "desc";
        GetListParams(jParams, strOrder, nLimit, nOffset);

        /* Sort order to apply */
        std::string strSort = "trust";
        if(jParams.find("sort") != jParams.end())
            strSort = jParams["sort"].get<std::string>();

        /* Timestamp of 30 days ago, to use to include active accounts */
        //const uint64_t nActive = runtime::unifiedtimestamp() - (60 * 60 * 24 * 30);

        /* The vector of active accounts */
        std::vector<TAO::Register::Object> vActive;

        /* Batch read up to 1000 at a time */
        std::vector<TAO::Register::Object> vAccounts;
        if(LLD::Register->BatchRead(strType, vAccounts, -1))
        {
            /* Make sure they are all parsed so that we can sort them */
            for(auto& rAccount : vAccounts)
            {
                /* Parse so we can access the data */
                if(!rAccount.Parse())
                    continue;

                //NOTE: this is a temporary manual filtering paramter, need to automate filtering language
                if(rAccount.Check("token", TAO::Register::TYPES::UINT256_T, false) && jParams.find("token") != jParams.end())
                {
                    /* Grab our token-id from our input parameters. */
                    const uint256_t hashToken = TAO::Register::Address(jParams["token"].get<std::string>());

                    /* Check for where clause with token. */
                    if(rAccount.get<uint256_t>("token") != hashToken)
                        continue;
                }

                /* Only include active within last 30 days */
                //if(rAccount.nModified < nActive)
                //    continue;

                /* Only include accounts with stake or trust (genesis has stake w/o trust; can unstake to trust w/o stake) */
                if(strType == "trust" && rAccount.get<uint64_t>("stake") == 0 && rAccount.get<uint64_t>("trust") == 0)
                    continue;

                /* Add the account to our active list */
                vActive.push_back(rAccount); //XXX: we really shouldn't copy everything again here
            }
        }

        /* Check that we have results. */
        if(vActive.empty())
            throw APIException(-74, "No registers found");

        /* Sort the list */
        const bool fDesc = (strOrder == "desc");
        if(vActive[0].Check(strSort))
        {
            /* Sort in decending/ascending order based on order param */
            std::sort(vActive.begin(), vActive.end(), [strSort, fDesc]
                    (const TAO::Register::Object &a, const TAO::Register::Object &b)
            {
                if(fDesc)
                    return ( a.get<uint64_t>(strSort) > b.get<uint64_t>(strSort) );
                else
                    return ( a.get<uint64_t>(strSort) < b.get<uint64_t>(strSort) );
            });
        }


        /* The return json array */
        json::json jRet = json::json::array();

        /* Iterate the list and build the response */
        uint32_t nTotal = 0;
        for(const auto& rAccount : vActive)
        {
            /* Populate the response */
            json::json jAccount = ObjectToJSON(rAccount);

            /* Check the offset. */
            if(++nTotal <= nOffset)
                continue;

            /* Check the limit */
            if(nTotal - nOffset > nLimit)
                break;

            jRet.push_back(jAccount);
        }

        return jRet;
    }
}
