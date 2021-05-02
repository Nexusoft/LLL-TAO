/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/tokens/types/tokens.h>
#include <TAO/API/finance/types/finance.h>

#include <TAO/API/include/check.h>
#include <TAO/API/include/global.h>

#include <Util/include/string.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Standard initialization function. */
    void Tokens::Initialize()
    {
        /* Handle for all CREATE operations. */
        mapFunctions["create"] = Function
        (
            std::bind
            (
                &Finance::Create,
                TAO::API::finance,
                std::placeholders::_1,
                std::placeholders::_2
            )
        );


        /* Handle for all CREDIT operations. */
        mapFunctions["credit"] = Function
        (
            std::bind
            (
                &Finance::Credit,
                TAO::API::finance,
                std::placeholders::_1,
                std::placeholders::_2
            )
        );

        //XXX: we should format this better, we can't go over 132 characters in a line based on formatting guidelines.
        mapFunctions["create/token"]              = Function(std::bind(&Tokens::Create,            this, std::placeholders::_1, std::placeholders::_2));
        //mapFunctions["credit"]                    = Function(std::bind(&Finance::Credit,           TAO::API::finance, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["debit/token"]               = Function(std::bind(&Tokens::Debit,             this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["get/token"]                 = Function(std::bind(&Tokens::Get,               this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["list/token/transactions"]   = Function(std::bind(&Tokens::ListTransactions,  this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["burn/token"]                = Function(std::bind(&Tokens::Burn,              this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["list/token/accounts"]       = Function(std::bind(&Tokens::ListTokenAccounts, this, std::placeholders::_1, std::placeholders::_2));

        /* Temporary reroute of the account methods to the finance API equivalents XXX: this is really hacky */
        //mapFunctions["credit"]                    = Function(std::bind(&Finance::Credit,           TAO::API::finance, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["debit/account"]             = Function(std::bind(&Finance::Debit,            TAO::API::finance, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["get/account"]               = Function(std::bind(&Finance::Get,              TAO::API::finance, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["list/accounts"]             = Function(std::bind(&Finance::List,             TAO::API::finance, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["list/account/transactions"] = Function(std::bind(&Finance::ListTransactions, TAO::API::finance, std::placeholders::_1, std::placeholders::_2));
    }

    /* Allows derived API's to handle custom/dynamic URL's where the strMethod does not
    *  map directly to a function in the target API.  Insted this method can be overriden to
    *  parse the incoming URL and route to a different/generic method handler, adding parameter
    *  values if necessary.  E.g. get/myasset could be rerouted to get/asset with name=myasset
    *  added to the jParams
    *  The return json contains the modifed method URL to be called.
    */
    std::string Tokens::RewriteURL(const std::string& strMethod, json::json &jParams)
    {
        /* Grab our components of the URL to rewrite. */
        const std::vector<std::string> vMethods = Split(strMethod, '/');

        /* Check that we have all of our values. */
        if(vMethods.empty()) //we are ignoring everything past first noun on rewrite
            throw APIException(-14, debug::safe_printstr(FUNCTION, "Malformed request URL: ", strMethod));

        /* Track if we've found an explicet type. */
        bool fNoun = false, fAddress = false, fField = false;;

        /* Grab the components of this URL. */
        const std::string& strVerb = vMethods[0];
        for(uint32_t n = 1; n < vMethods.size(); ++n)
        {
            /* Grab our current noun. */
            const std::string& strNoun = vMethods[n];

            /* Now lets do some rules for the different nouns. */
            if(!fNoun && (strNoun == "token" || strNoun == "account"))
            {
                debug::log(0, FUNCTION, "Adding ", strNoun, " as type");
                jParams["type"] = strNoun;

                /* Set our explicet flag now. */
                fNoun = true;
                continue;
            }

            /* If we have reached here, we know we are an address or name. */
            else if(!fAddress)
            {
                /* Check if this value is an address. */
                if(CheckAddress(strNoun))
                {
                    debug::log(0, FUNCTION, "Adding ", strNoun, " as address");
                    jParams["address"] = strNoun;

                    continue;
                }

                /* If not address it must be a nme. */
                else
                {
                    debug::log(0, FUNCTION, "Adding ", strNoun, " as name");
                    jParams["name"] = strNoun;

                    continue;
                }

                /* Set both address and noun to handle ommiting the noun if desired. */
                fAddress = true;
                fNoun    = true;
            }

            /* If we have reached here, we know we are a fieldname. */
            else if(!fField)
            {
                debug::log(0, FUNCTION, "Adding ", strNoun, " as fieldname");
                jParams["fieldname"] = strNoun;

                fField = true;
                continue;
            }

            /* If we get here, we need to throw for malformed URL. */
            else
                throw APIException(-14, debug::safe_printstr(FUNCTION, "Malformed request URL: ", strMethod));
        }

        return strVerb;
    }
}
