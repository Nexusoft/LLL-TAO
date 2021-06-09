
/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/types/commands.h>

#include <TAO/API/users/types/users.h>
#include <TAO/API/names/types/names.h>
#include <TAO/API/types/commands/finance.h>
#include <TAO/API/objects/types/objects.h>

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
#include <TAO/Ledger/types/sigchain.h>

#include <TAO/Register/types/object.h>

#include <Util/include/debug.h>
#include <Util/include/math.h>


/* Global TAO namespace. */
namespace TAO::API
{
    typedef struct //XXX: this should be in its own header file
    {
        /* The confirmed nBalances from the state at the last block*/
        uint64_t nBalance = 0;

        /* The available nBalances including mempool transactions (outgoing debits) */
        uint64_t nAvailable = 0;

        /* The sum of all debits that are confirmed but not yet credited */
        uint64_t nUnclaimed = 0;

        /* The sum of all incoming debits that are not yet confirmed or credits we have made that are not yet confirmed*/
        uint64_t nUnconfirmed = 0;

        /* The sum of all unconfirmed outcoing debits */
        uint64_t nUnconfirmedOutgoing = 0; //XXX: this is superfluous, consider removing

        /* The amount currently being staked */
        uint64_t nStake = 0;

        /* The sum of all immature coinbase transactions */
        uint64_t nImmature = 0;

        /* The decimals used for this token for display purposes */
        uint8_t nDecimals = 0;

    } balances_t;


    /* Get a summary of nBalances information across all accounts belonging to the currently logged in signature chain
       for a particular token type */
    encoding::json Finance::GetBalances(const encoding::json& j, const bool fHelp)
    {
        /* The user genesis hash */
        const uint256_t hashGenesis =
            Commands::Get<Users>()->GetSession(j).GetAccount()->Genesis();

        /* The token to return balances for. Default to 0 (NXS) */
        const uint256_t hashToken = ExtractToken(j);

        /* First get the list of registers owned by this sig chain so we can work out which ones are NXS accounts */
        std::vector<TAO::Register::Address> vRegisters;
        if(!ListRegisters(hashGenesis, vRegisters))
            throw APIException(-74, "No registers found");

        /* Iterate through each register we own */
        balances_t nBalances;
        for(const auto& hashRegister : vRegisters)
        {
            /* Initial check that it is an account/trust/token, before we hit the DB to get the nBalances */
            if(!hashRegister.IsAccount() && !hashRegister.IsTrust() && !hashRegister.IsToken())
                continue;

            /* Get the register from the register DB */
            TAO::Register::Object object;
            if(!LLD::Register->ReadObject(hashRegister, object)) // note we don't include mempool state here as we want the confirmed
                continue;

            /* Check that this is an account */
            if(object.Base() != TAO::Register::OBJECTS::ACCOUNT)
                continue;

            /* Check that it is for the correct token */
            if(object.get<uint256_t>("token") != hashToken)
                continue;

            /* Cache this if it is the trust account */
            if(object.Standard() == TAO::Register::OBJECTS::TRUST)
                nBalances.nStake = object.get<uint64_t>("stake");

            /* Increment the nBalances */
            nBalances.nBalance += object.get<uint64_t>("nBalances");

            /* Cache the decimals for this token to use for display */
            nBalances.nDecimals = GetDecimals(object);
        }

        /* Populate the response object */
        nBalances.nUnclaimed           = GetPending(hashGenesis, hashToken);
        nBalances.nUnconfirmed         = GetUnconfirmed(hashGenesis, hashToken, false);
        nBalances.nUnconfirmedOutgoing = GetUnconfirmed(hashGenesis, hashToken, true);
        nBalances.nAvailable           = nBalances.nBalance - nBalances.nUnconfirmedOutgoing;
        nBalances.nImmature            = GetImmature(hashGenesis);

        /* Resolve the name of the token name */
        const std::string strToken =
            (hashToken != 0 ? Names::ResolveName(hashGenesis, hashToken) : "NXS");

        /* Poplate the json response object. */
        encoding::json jRet;
        jRet["token"]        = hashToken.ToString();
        jRet["available"]    = (double)nBalances.nAvailable   / math::pow(10, nBalances.nDecimals);
        jRet["pending"]      = (double)nBalances.nUnclaimed   / math::pow(10, nBalances.nDecimals);
        jRet["unconfirmed"]  = (double)nBalances.nUnconfirmed / math::pow(10, nBalances.nDecimals);

        /* Add the token identifier */
        if(!strToken.empty())
            jRet["token_name"] = strToken;

        /* Add stake/immature for NXS only */
        if(hashToken == 0)
        {
            jRet["stake"]    = (double)nBalances.nStake    / math::pow(10, nBalances.nDecimals);
            jRet["immature"] = (double)nBalances.nImmature / math::pow(10, nBalances.nDecimals);
        }

        return jRet;
    }


    /* Get a summary of nBalances information across all accounts belonging to the currently logged in signature chain */
    encoding::json Finance::ListBalances(const encoding::json& jParams, const bool fHelp)
    {
        /* The user genesis hash */
        const uint256_t hashGenesis =
            Commands::Get<Users>()->GetSession(jParams).GetAccount()->Genesis();

        /* Number of results to return. */
        uint32_t nLimit = 100, nOffset = 0, nTotal = 0;

        /* Get the params to apply to the response. */
        std::string strOrder = "desc";
        ExtractList(jParams, strOrder, nLimit, nOffset);

        /* First get the list of registers owned by this sig chain so we can work out which ones are NXS accounts */
        std::vector<TAO::Register::Address> vRegisters;
        if(!ListRegisters(hashGenesis, vRegisters))
            throw APIException(-74, "No registers found");

        /* Keep a map to track our aggregated balance, we use a second map for better readability. */
        std::map<uint256_t, std::map<std::string, uint64_t>> mapBalances;

        /* Iterate through each register we own */
        for(const auto& hashRegister : vRegisters)
        {
            /* Initial check that it is an account/trust/token, before we hit the DB to get the nBalances */
            if(!hashRegister.IsAccount() && !hashRegister.IsTrust() && !hashRegister.IsToken())
                continue;

            /* Get the register from the register DB */
            TAO::Register::Object object;
            if(!LLD::Register->ReadObject(hashRegister, object))
                continue; // note we don't include mempool state here as we want the confirmed bakances

            /* Check that this is an account */
            if(object.Base() != TAO::Register::OBJECTS::ACCOUNT)
                continue;

            /* Check the accounts match the where filter. */
            if(!FilterObject(jParams, object))
                continue;

            /* Get the token */
            const uint256_t hashToken = object.get<uint256_t>("token");

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
            const uint8_t nDecimals = rBalances.second.at("decimals");

            /* Grab unconfirmed balances as a pair. */
            const uint64_t nOutgoing =
                GetUnconfirmed(hashGenesis, hashToken, true);

            /* Resolve the name of the token name */
            const std::string strToken =
                (hashToken != TOKEN::NXS ? Names::ResolveName(hashGenesis, hashToken) : "NXS");

            /* Poplate the json response object. */
            encoding::json jBalances;
            if(hashToken != TOKEN::NXS)
                jBalances["token"]    = hashToken.ToString();

            /* Populate the rest of the balances. */
            jBalances["available"]    = FormatBalance(rBalances.second.at("balance") - nOutgoing,    nDecimals);
            jBalances["unclaimed"]    = FormatBalance(GetPending(hashGenesis, hashToken),            nDecimals);
            jBalances["unconfirmed"]  = FormatBalance(GetUnconfirmed(hashGenesis, hashToken, false), nDecimals);

            /* Add the token identifier */
            if(!strToken.empty())
                jBalances["token_name"] = strToken;

            /* Add stake/immature for NXS only */
            if(hashToken == TOKEN::NXS)
            {
                jBalances["stake"]    = FormatBalance(rBalances.second.at("stake"), nDecimals);
                jBalances["immature"] = FormatBalance(GetImmature(hashGenesis),     nDecimals);
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
