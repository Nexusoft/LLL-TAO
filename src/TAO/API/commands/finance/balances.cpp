
/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2023

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/types/commands.h>

#include <TAO/API/types/authentication.h>
#include <TAO/API/types/commands/names.h>
#include <TAO/API/types/commands/finance.h>


#include <TAO/API/include/build.h>
#include <TAO/API/include/check.h>
#include <TAO/API/include/constants.h>
#include <TAO/API/include/extract.h>
#include <TAO/API/include/filter.h>
#include <TAO/API/include/format.h>
#include <TAO/API/include/list.h>
#include <TAO/API/include/get.h>
#include <TAO/API/include/json.h>

#include <TAO/Ledger/include/constants.h>
#include <TAO/Ledger/types/credentials.h>

#include <TAO/Register/types/object.h>

#include <Util/include/debug.h>
#include <Util/include/math.h>


/* Global TAO namespace. */
namespace TAO::API
{
    /* Get balances for a particular token type including immature, stake, and unclaimed  */
    encoding::json Finance::GetBalances(const encoding::json& jParams, const bool fHelp)
    {
        /* The user genesis hash */
        const uint256_t hashGenesis =
            Authentication::Caller(jParams);

        /* Number of results to return. */
        uint32_t nLimit = 100, nOffset = 0, nTotal = 0;

        /* Get the params to apply to the response. */
        std::string strOrder = "desc";
        ExtractList(jParams, strOrder, nLimit, nOffset);

        /* First get the list of registers owned by this sig chain so we can work out which ones are NXS accounts */
        std::set<TAO::Register::Address> setAddresses;
        if(!LLD::Logical->ListRegisters(hashGenesis, setAddresses))
            throw Exception(-74, "No registers found");

        /* Keep a map to track our aggregated balance, we use a second map for better readability. */
        std::map<uint256_t, std::map<std::string, uint64_t>> mapBalances;

        /* Iterate through each register we own */
        for(const auto& hashRegister : setAddresses)
        {
            /* Initial check that it is an account/trust/token, before we hit the DB to get the nBalances */
            if(!hashRegister.IsAccount() && !hashRegister.IsTrust() && !hashRegister.IsToken())
                continue;

            /* Get the register from the register DB */
            TAO::Register::Object object;
            if(!LLD::Register->ReadObject(hashRegister, object))
                continue;

            /* Check that this is an account */
            if(object.Base() != TAO::Register::OBJECTS::ACCOUNT)
                continue;

            /* Get the token */
            const uint256_t hashToken =
                object.get<uint256_t>("token");

            /* Cache the decimals for this token to use for display */
            if(!mapBalances.count(hashToken))
            {
                /* Grab our decimals and store as string key. */
                mapBalances[hashToken]["decimals"] = GetDecimals(object);

                /* We are initializing here to not leave any room for uninitialed values across platforms. */
                mapBalances[hashToken]["balance"]  = 0;
                mapBalances[hashToken]["stake"]    = 0;
            }

            /* Increment the balance */
            mapBalances[hashToken]["balance"] += object.get<uint64_t>("balance");

            /* Check for available stake. */
            if(object.Standard() == TAO::Register::OBJECTS::TRUST)
                mapBalances[hashToken]["stake"] += object.get<uint64_t>("stake");
        }

        /* Iterate through each register we own */
        encoding::json jRet = encoding::json::array();
        for(const auto& rBalances : mapBalances)
        {
            /* Get the token */
            const TAO::Register::Address hashToken = rBalances.first;

            /* Grab our decimals from balances map. */
            const uint8_t nDecimals =
                rBalances.second.at("decimals");

            /* Grab unconfirmed outgoing balances. */
            const uint64_t nOutgoing =
                GetUnconfirmed(hashGenesis, hashToken, true);

            /* Poplate the json response object. */
            encoding::json jBalances;

            /* Populate the rest of the balances. */
            jBalances["available"]    = FormatBalance(rBalances.second.at("balance") - nOutgoing, nDecimals);
            jBalances["confirmed"]    = FormatBalance(rBalances.second.at("balance"), nDecimals);
            jBalances["unclaimed"]    = FormatBalance(GetUnclaimed(hashGenesis, hashToken), nDecimals);
            jBalances["unconfirmed"]  = FormatBalance(GetUnconfirmed(hashGenesis, hashToken, false), nDecimals);
            jBalances["decimals"]     = nDecimals;
            jBalances["token"]        = hashToken.ToString();

            /* Add a ticker if found. */
            std::string strName;
            if(Names::ReverseLookup(hashToken, strName))
                jBalances["ticker"] = strName;

            /* Add stake/immature for NXS only */
            if(hashToken == TOKEN::NXS)
            {
                jBalances["stake"]    = FormatBalance(rBalances.second.at("stake"));
                jBalances["immature"] = FormatBalance(GetImmature(hashGenesis));
            }

            /* Filter results now. */
            if(!FilterResults(jParams, jBalances))
                continue;

            /* Check the offset. */
            if(++nTotal <= nOffset)
                continue;

            /* Check the limit */
            if(nTotal - nOffset > nLimit)
                break;

            jRet.push_back(jBalances);
        }

        return jRet;
    }
}
