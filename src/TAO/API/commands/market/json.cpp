/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/types/commands/market.h>

#include <TAO/API/include/format.h>
#include <TAO/API/include/get.h>
#include <TAO/API/include/contracts/verify.h>

#include <TAO/API/types/commands/names.h>
#include <TAO/API/types/contracts/exchange.h>

#include <Util/include/math.h>
#include <Util/types/precision.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Converts an order for marketplace into formatted JSON. */
    encoding::json Market::OrderToJSON(const TAO::Operation::Contract& rContract, const std::pair<uint256_t, uint256_t>& pairMarket)
    {
        /* Start our stream at 0. */
        rContract.Reset();

        /* Get the operation byte. */
        uint8_t nType = 0;
        rContract >> nType;

        /* Switch based on type. */
        switch(nType)
        {
            /* Check that transaction has a condition. */
            case TAO::Operation::OP::CONDITION:
            {
                /* Check our binary templates first. */
                if(!Contracts::Verify(Contracts::Exchange::Token[0], rContract)) //checking for version 1
                    return encoding::json::value_t::null;

                try
                {
                    /* This will hold our market name. */
                    std::string strMarket =
                        (TAO::Register::Address(pairMarket.first).ToString() + "/");

                    /* Build our market-pair. */
                    std::string strName;
                    if(Names::ReverseLookup(pairMarket.first, strName))
                        strMarket = strName + "/";

                    /* Now add our second pair. */
                    if(!Names::ReverseLookup(pairMarket.second, strName))
                        strMarket += TAO::Register::Address(pairMarket.second).ToString();
                    else
                        strMarket += strName;

                    /* Build our JSON object. */
                    encoding::json jRet =
                    {
                        { "txid",      rContract.Hash().ToString()   },
                        { "timestamp", rContract.Timestamp()         },
                        { "owner",     rContract.Caller().ToString() },
                        { "market",    strMarket                     }
                    };

                    /* Seek over our OP::DEBIT enum. */
                    rContract.Seek(1, TAO::Operation::Contract::OPERATIONS);

                    /* Get the spending address. */
                    TAO::Register::Address hashFrom;
                    rContract >> hashFrom;

                    /* Seek to end of debit contract. */
                    rContract.Seek(32, TAO::Operation::Contract::OPERATIONS);

                    /* Deserialize our amount. */
                    uint64_t nSelling = 0;
                    rContract >> nSelling;

                    /* Grab our other token from pre-state. */
                    TAO::Register::Object tPreState = rContract.PreState();

                    /* Parse pre-state if needed. */
                    if(tPreState.nType == TAO::Register::REGISTER::OBJECT)
                        tPreState.Parse();

                    /* Grab the rhs token. */
                    TAO::Register::Address hashSelling =
                        tPreState.get<uint256_t>("token");

                    /* Extract our precision_t from parameters. */
                    const precision_t dSelling =
                        GetPrecision(nSelling, hashSelling);

                    /* Build our from object. */
                    encoding::json jFrom =
                    {
                        { "OP",   "DEBIT"                              },
                        { "from" , hashFrom.ToString()                 },
                        { "amount", dSelling.double_t()                },
                        { "token", hashSelling.ToString()              }
                    };

                    /* Add a ticker if found. */
                    if(Names::ReverseLookup(hashSelling, strName))
                        jFrom["ticker"] = strName;

                    /* Get the next OP. */
                    rContract.Seek(4, TAO::Operation::Contract::CONDITIONS);

                    /* Get the comparison bytes. */
                    std::vector<uint8_t> vBytes;
                    rContract >= vBytes;

                    /* Get the next OP. */
                    rContract.Seek(2, TAO::Operation::Contract::CONDITIONS);

                    /* Grab our pre-state token-id. */
                    std::string strToken;
                    rContract >= strToken;

                    /* Get the next OP. */
                    rContract.Seek(2, TAO::Operation::Contract::CONDITIONS);

                    /* Grab our token-id now. */
                    TAO::Register::Address hashBuying;
                    rContract >= hashBuying;

                    /* Extract the data from the bytes. */
                    TAO::Operation::Stream ssCompare(vBytes);
                    ssCompare.seek(33);

                    /* Get the address to. */
                    TAO::Register::Address hashTo;
                    ssCompare >> hashTo;

                    /* Get the amount requested. */
                    uint64_t nBuying = 0;
                    ssCompare >> nBuying;

                    /* Extract our precision_t from parameters. */
                    const precision_t dBuying =
                        GetPrecision(nBuying, hashBuying);

                    /* Build our from object. */
                    encoding::json jRequired =
                    {
                        { "OP",     "DEBIT"                              },
                        { "to",     hashTo.ToString()                    },
                        { "amount", dBuying.double_t()                   },
                        { "token",  hashBuying.ToString()               }
                    };

                    /* Add a ticker if found. */
                    if(Names::ReverseLookup(hashBuying, strName))
                        jRequired["ticker"] = strName;

                    /* Track our price field with correct number of decimals. */
                    precision_t dPrice =
                        precision_t(GetDecimals(pairMarket.second));

                    /* Handle if required is our base token. */
                    std::string strType = "";
                    if(hashBuying != pairMarket.second)
                    {
                        /* Check for floating point exceptions. */
                        if(dBuying.nValue == 0)
                            return encoding::json::value_t::null;

                        /* Calculate our price using precision figures. */
                        dPrice = dSelling;
                        dPrice /= dBuying;

                        /* Set our price now with correct decimals. */
                        jRet["price"] = dPrice.double_t();

                        /* Check what type of order this is. */
                        strType = "bid";
                    }
                    else
                    {
                        /* Check for floating point exceptions. */
                        if(dSelling.nValue == 0)
                            return encoding::json::value_t::null;

                        /* Calculate our price using precision figures. */
                        dPrice = dBuying;
                        dPrice /= dSelling;

                        /* Set our price now with correct decimals. */
                        jRet["price"] = dPrice.double_t();

                        /* Check what type of order this is. */
                        strType = "ask";
                    }

                    /* Now populate the rest of our data. */
                    jRet["type"]        = strType;
                    jRet["contract"]    = jFrom;
                    jRet["order"]       = jRequired;

                    return jRet;
                }
                catch(const std::exception& e)
                {
                    return encoding::json::value_t::null;
                }
            }

            default:
                throw Exception(-25, "Market order invalid contract: not conditional contract");

        }
    }


    /* Converts an order for marketplace into formatted JSON. */
    encoding::json Market::OrderToJSON(const TAO::Operation::Contract& rContract, const uint256_t& hashBase)
    {
        /* Start our stream at 0. */
        rContract.Reset();

        /* Get the operation byte. */
        uint8_t nType = 0;
        rContract >> nType;

        /* Switch based on type. */
        switch(nType)
        {
            /* Check that transaction has a condition. */
            case TAO::Operation::OP::CONDITION:
            {
                /* Check our binary templates first. */
                if(!Contracts::Verify(Contracts::Exchange::Token[0], rContract)) //checking for version 1
                    throw Exception(-25, "Market order invalid contract: not valid binary order");

                /* Build our base pair to check against. */
                std::pair<uint256_t, uint256_t> pairMarket = std::make_pair(uint256_t(0), hashBase);

                try
                {
                    /* Seek over our OP::DEBIT enum. */
                    rContract.Seek(1, TAO::Operation::Contract::OPERATIONS);

                    /* Get the spending address. */
                    TAO::Register::Address hashFrom;
                    rContract >> hashFrom;

                    /* Seek to end of debit contract. */
                    rContract.Seek(32, TAO::Operation::Contract::OPERATIONS);

                    /* Deserialize our amount. */
                    uint64_t nSelling = 0;
                    rContract >> nSelling;

                    /* Grab our other token from pre-state. */
                    TAO::Register::Object tPreState = rContract.PreState();

                    /* Parse pre-state if needed. */
                    if(tPreState.nType == TAO::Register::REGISTER::OBJECT)
                        tPreState.Parse();

                    /* Grab the rhs token. */
                    TAO::Register::Address hashSelling =
                        tPreState.get<uint256_t>("token");

                    /* Get the next OP. */
                    rContract.Seek(4, TAO::Operation::Contract::CONDITIONS);

                    /* Get the comparison bytes. */
                    std::vector<uint8_t> vBytes;
                    rContract >= vBytes;

                    /* Get the next OP. */
                    rContract.Seek(2, TAO::Operation::Contract::CONDITIONS);

                    /* Grab our pre-state token-id. */
                    std::string strToken;
                    rContract >= strToken;

                    /* Get the next OP. */
                    rContract.Seek(2, TAO::Operation::Contract::CONDITIONS);

                    /* Grab our token-id now. */
                    TAO::Register::Address hashBuying;
                    rContract >= hashBuying;

                    /* Check if the order being placed is for native token. */
                    if(hashSelling == hashBase)
                    {
                        /* This is an ask if for native token. */
                        pairMarket.first = hashBuying;
                        return OrderToJSON(rContract, pairMarket);
                    }

                    /* Otherwise this is a bid. */
                    pairMarket.first = hashSelling;
                    return OrderToJSON(rContract, pairMarket);
                }
                catch(const std::exception& e)
                {
                    throw Exception(-25, "Market order invalid contract: ", e.what());
                }
            }

            default:
                throw Exception(-25, "Market order invalid contract: not conditional contract");

        }
    }
}
