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
#include <TAO/API/include/get.h>
#include <TAO/API/types/commands.h>
#include <TAO/API/types/commands/market.h>

#include <TAO/Operation/include/enum.h>

#include <TAO/Register/include/constants.h>

#include <TAO/Ledger/types/transaction.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Create an asset or digital item. */
    encoding::json Market::Place(const encoding::json& jParams, const bool fHelp)
    {
        /* Extract our order addresses. */
        const uint256_t hashRegister  = ExtractAddress(jParams, "from");
        const uint256_t hashRecipient = ExtractAddress(jParams, "to");

        /* Grab our object we are placing order for. */
        TAO::Register::Object tDebit;
        if(!LLD::Register->ReadObject(hashRegister, tDebit))
            throw Exception(-13, "Object not found");

        /* Check they have the required funds */
        const uint64_t nAmount = ExtractAmount(jParams, GetFigures(tDebit), "from");

        /* Grab our object we are placing order for. */
        TAO::Register::Object tRecipient;
        if(!LLD::Register->ReadObject(hashRecipient, tRecipient))
            throw Exception(-13, "Object not found");

        /* Get the amount to debit. */
        const uint64_t nRequested =
            ExtractAmount(jParams, GetFigures(tRecipient), "to");

        /* Transation payload. */
        std::vector<TAO::Operation::Contract> vContracts(1);
        vContracts[0] << uint8_t(TAO::Operation::OP::CONDITION) << uint8_t(TAO::Operation::OP::DEBIT);
        vContracts[0] << hashRegister << TAO::Register::WILDCARD_ADDRESS << nAmount << uint64_t(0);

        /* Create a comparison binary stream we will use for the contract requirement. */
        TAO::Operation::Stream ssCompare;
        ssCompare << uint8_t(TAO::Operation::OP::DEBIT) << uint256_t(0) << hashRecipient << nRequested << uint64_t(0);

        /* This conditional contract will require a DEBIT of given amount. */
        vContracts[0] <= uint8_t(TAO::Operation::OP::GROUP);
        vContracts[0] <= uint8_t(TAO::Operation::OP::CALLER::OPERATIONS) <= uint8_t(TAO::Operation::OP::CONTAINS);
        vContracts[0] <= uint8_t(TAO::Operation::OP::TYPES::BYTES) <= ssCompare.Bytes();
        vContracts[0] <= uint8_t(TAO::Operation::OP::AND);
        vContracts[0] <= uint8_t(TAO::Operation::OP::CALLER::PRESTATE::VALUE) <= std::string("token");
        vContracts[0] <= uint8_t(TAO::Operation::OP::EQUALS);
        vContracts[0] <= uint8_t(TAO::Operation::OP::TYPES::UINT256_T) <= tRecipient.get<uint256_t>("token");
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
