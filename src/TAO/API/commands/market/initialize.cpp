/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/types/commands/market.h>
#include <TAO/API/types/commands/names.h>
#include <TAO/API/types/operators/initialize.h>

#include <TAO/API/include/extract.h>

#include <Util/include/args.h>
#include <Util/include/string.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Standard initialization function. */
    void Market::Initialize()
    {
        /* Populate our operators. */
        Operators::Initialize(mapOperators);


        /* Handle for our market fees. */
        if(config::mapMultiArgs["-marketfee"].size() > 0)
        {
            /* Add connections and resolve potential DNS lookups. */
            for(const std::string& strFee : config::mapMultiArgs["-marketfee"])
            {
                /* Parse out our fee contracts. */
                std::vector<std::string> vFees;
                ParseString(strFee, ':', vFees);

                /* Check for expected sizes. */
                if(vFees.size() != 3)
                    throw Exception(0, "-marketfee parameter parse error: ", strFee);

                /* Extract the token-id. */
                const TAO::Register::Address hashToken =
                    ExtractAddress(vFees[0], encoding::json::object());

                /* Extract the deposit address. */
                const TAO::Register::Address hashDeposit =
                    ExtractAddress(vFees[1], encoding::json::object());

                /* Extract the percentage. */
                const uint64_t nPercent =
                    std::stod(vFees[2]) * 1000; //we maintain 3 decimal places precision

                /* Check for valid token-id. */
                TAO::Register::Object tToken;
                if(!LLD::Register->ReadObject(hashToken, tToken))
                    throw Exception(0, "-marketfee token ", hashToken.ToString(), " doesn't exist");

                /* Check the deposit account exists. */
                TAO::Register::Object tDeposit;
                if(!LLD::Register->ReadObject(hashDeposit, tDeposit))
                    throw Exception(0, "-marketfee account ", hashDeposit.ToString(), " doesn't exist");

                /* Check that our deposit account is the correct token. */
                if(tDeposit.get<uint256_t>("token") != hashToken)
                    throw Exception(0, "-marketfee account ", hashDeposit.ToString(), " incorrect token");

                /* Add our token and values to market fees. */
                mapFees[hashToken] = std::make_pair(hashDeposit, nPercent);

                /* Get our token string if resolved. */
                std::string strToken;
                if(!Names::ReverseLookup(hashToken, strToken))
                    strToken = hashToken.ToString();

                /* Log that we have added the fees. */
                debug::notice("Market Fee (", nPercent / 1000.0, "%) created for ",
                    strToken, " with deposit address of ", hashDeposit.ToString());
            }
        }


        /* Standard contract to create new order. */
        mapFunctions["create"] = Function
        (
            std::bind
            (
                &Market::Create,
                this,
                std::placeholders::_1,
                std::placeholders::_2
            )
            , "bid, ask"
        );

        /* Standard contract to create new order. */
        mapFunctions["list"] = Function
        (
            std::bind
            (
                &Market::List,
                this,
                std::placeholders::_1,
                std::placeholders::_2
            )
            , "bid, ask, order, executed"
        );

        /* Standard contract to execute an order. */
        mapFunctions["execute"] = Function
        (
            std::bind
            (
                &Market::Execute,
                this,
                std::placeholders::_1,
                std::placeholders::_2
            )
            , "bid, ask"
        );

        /* Standard contract to execute an order. */
        mapFunctions["cancel"] = Function
        (
            std::bind
            (
                &Market::Cancel,
                this,
                std::placeholders::_1,
                std::placeholders::_2
            )
            , "bid, ask"
        );

        /* Standard contract to create new order. */
        mapFunctions["user"] = Function
        (
            std::bind
            (
                &Market::User,
                this,
                std::placeholders::_1,
                std::placeholders::_2
            )
            , "order, executed"
        );
    }
}
