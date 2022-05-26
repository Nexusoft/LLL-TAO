/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/include/build.h>
#include <TAO/API/include/extract.h>
#include <TAO/API/include/format.h>
#include <TAO/API/include/get.h>
#include <TAO/API/types/commands.h>
#include <TAO/API/types/commands/market.h>

#include <TAO/Operation/include/enum.h>

#include <TAO/Register/include/constants.h>

#include <TAO/Ledger/types/transaction.h>

#include <Util/include/math.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Create an asset or digital item. */
    encoding::json Market::Create(const encoding::json& jParams, const bool fHelp)
    {
        /* Get our current object type. */
        const std::string strType = ExtractType(jParams);

        /* Grab our market pair. */
        const std::pair<uint256_t, uint256_t> pairMarket = ExtractMarket(jParams);

        /* Extract our order addresses. */
        const uint256_t hashRegister = ExtractAddress(jParams, "from");
        const uint256_t hashDeposit  = ExtractAddress(jParams, "to");

        /* Grab our object we are placing order for. */
        TAO::Register::Object oAccount;
        if(!LLD::Register->ReadObject(hashRegister, oAccount))
            throw Exception(-13, "Object not found");

        /* Get the token-id of our debiting account. */
        const uint256_t hashToken =
            oAccount.get<uint256_t>("token");

        /* Grab our object we are depositing order in. */
        TAO::Register::Object oDeposit;
        if(!LLD::Register->ReadObject(hashDeposit, oDeposit))
            throw Exception(-13, "Object not found");

        /* Get the token-id of our deposit account. */
        const uint256_t hashRequest =
            oDeposit.get<uint256_t>("token");

        /* Check they have the required funds */
        const uint64_t nAmount =
            ExtractAmount(jParams, GetFigures(pairMarket.second));

        /* Extract the price field. */
        const uint64_t nPrice =
            ExtractAmount(jParams, GetFigures(pairMarket.second), "price");

        /* Calculate our required amount in order. */
        uint64_t nTotal = 0;
        if(hashRequest != pairMarket.second)
        {
            /* Check our account token types. */
            if(oDeposit.get<uint256_t>("token") != pairMarket.first)
                throw Exception(-26, "Invalid parameter [deposit], [", strType, "] requires correct token");

            /* Check our account token types. */
            if(oAccount.get<uint256_t>("token") != pairMarket.second)
                throw Exception(-26, "Invalid parameter [from], [", strType, "] requires correct token");

            /* Check our type is bid. */
            if(strType != "bid")
                throw Exception(-7, "Unsupported type [", strType, "] for parameters");

            /* Check if we need to adjust our figures. */
            const uint8_t nRequestDecimals = GetDecimals(pairMarket.first);
            const uint8_t nOrderedDecimals = GetDecimals(pairMarket.second);

            /* Check if we need to adjust requested total. */
            uint64_t nPriceAdjusted = nPrice, nAmountAdjusted = nAmount;
            if(nRequestDecimals < nOrderedDecimals)
                nPriceAdjusted *= uint64_t(math::pow(10, nOrderedDecimals - nRequestDecimals));

            /* Check if we need to adjust the order total. */
            else if(nOrderedDecimals < nRequestDecimals)
                nAmountAdjusted *= uint64_t(math::pow(10, nRequestDecimals - nOrderedDecimals));

            /* Calculate our price. */
            nTotal =
                (nAmountAdjusted * math::pow(10, nOrderedDecimals)) / nPriceAdjusted;
        }

        /* Handle for asks. */
        else
        {
            /* Check our account token types. */
            if(oAccount.get<uint256_t>("token") != pairMarket.first)
                throw Exception(-26, "Invalid parameter [from], [", strType, "] requires correct token");

            /* Check our account token types. */
            if(oDeposit.get<uint256_t>("token") != pairMarket.second)
                throw Exception(-26, "Invalid parameter [deposit], [", strType, "] requires correct token");

            /* Check our type is an ask. */
            if(strType != "ask")
                throw Exception(-7, "Unsupported type [", strType, "] for parameters");

            /* Check if we need to adjust our figures. */
            const uint8_t nRequestDecimals = GetDecimals(pairMarket.second);
            const uint8_t nOrderedDecimals = GetDecimals(pairMarket.first);

            /* Check if we need to adjust requested total. */
            uint64_t nPriceAdjusted = nPrice, nAmountAdjusted = nAmount;
            if(nRequestDecimals < nOrderedDecimals)
                nPriceAdjusted *= uint64_t(math::pow(10, nOrderedDecimals - nRequestDecimals));

            /* Check if we need to adjust the order total. */
            else if(nOrderedDecimals < nRequestDecimals)
                nAmountAdjusted *= uint64_t(math::pow(10, nRequestDecimals - nOrderedDecimals));

            /* Calculate our price. */
            nTotal =
                (nAmountAdjusted * math::pow(10, nRequestDecimals)) / nPriceAdjusted;
        }

        /* Transation payload. */
        std::vector<TAO::Operation::Contract> vContracts(1);
        vContracts[0] << uint8_t(TAO::Operation::OP::CONDITION) << uint8_t(TAO::Operation::OP::DEBIT);
        vContracts[0] << hashRegister << TAO::Register::WILDCARD_ADDRESS << nAmount << uint64_t(0);

        /* Create a comparison binary stream we will use for the contract requirement. */
        TAO::Operation::Stream ssCompare;
        ssCompare << uint8_t(TAO::Operation::OP::DEBIT) << uint256_t(0) << hashDeposit << nTotal << uint64_t(0);

        /* This conditional contract will require a DEBIT of given amount. */
        vContracts[0] <= uint8_t(TAO::Operation::OP::GROUP);
        vContracts[0] <= uint8_t(TAO::Operation::OP::CALLER::OPERATIONS) <= uint8_t(TAO::Operation::OP::CONTAINS);
        vContracts[0] <= uint8_t(TAO::Operation::OP::TYPES::BYTES) <= ssCompare.Bytes();
        vContracts[0] <= uint8_t(TAO::Operation::OP::AND);
        vContracts[0] <= uint8_t(TAO::Operation::OP::CALLER::PRESTATE::VALUE) <= std::string("token");
        vContracts[0] <= uint8_t(TAO::Operation::OP::EQUALS);
        vContracts[0] <= uint8_t(TAO::Operation::OP::TYPES::UINT256_T) <= hashRequest;
        vContracts[0] <= uint8_t(TAO::Operation::OP::UNGROUP);

        /* Logical operator between clauses. */
        vContracts[0] <= uint8_t(TAO::Operation::OP::OR);

        /* This is our clawback clause in case order needs to be cancelled. */
        vContracts[0] <= uint8_t(TAO::Operation::OP::GROUP);
        vContracts[0] <= uint8_t(TAO::Operation::OP::CALLER::GENESIS);
        vContracts[0] <= uint8_t(TAO::Operation::OP::EQUALS);
        vContracts[0] <= uint8_t(TAO::Operation::OP::CONTRACT::GENESIS);
        vContracts[0] <= uint8_t(TAO::Operation::OP::UNGROUP);

        return BuildResponse(jParams, hashRegister, vContracts);
    }
}
