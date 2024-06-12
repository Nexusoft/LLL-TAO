/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2023

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/include/build.h>
#include <TAO/API/include/extract.h>
#include <TAO/API/include/format.h>
#include <TAO/API/include/contracts/verify.h>

#include <TAO/API/types/commands.h>
#include <TAO/API/types/commands/market.h>
#include <TAO/API/types/contracts/exchange.h>

#include <TAO/Operation/include/enum.h>

#include <TAO/Ledger/types/transaction.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Create an asset or digital item. */
    encoding::json Market::Execute(const encoding::json& jParams, const bool fHelp)
    {
        /* Check for from parameter. */
        const uint256_t hashAddress =
            ExtractAddress(jParams, "from");

        /* Get the token / account object. */
        TAO::Register::Object tObjFrom;
        if(!LLD::Register->ReadObject(hashAddress, tObjFrom, TAO::Ledger::FLAGS::MEMPOOL))
            throw Exception(-13, "Object [from] not found");

        /* Check for valid types. */
        if(tObjFrom.Base() != TAO::Register::OBJECTS::ACCOUNT)
            throw Exception(-13, "Object [from] is not an account");

        /* Get our txid. */
        const uint512_t hashOrder   = ExtractHash(jParams);

        /* Read the previous transaction. */
        TAO::Ledger::Transaction tx;
        if(!LLD::Ledger->ReadTx(hashOrder, tx))
            throw Exception(-40, "Previous transaction not found.");

        /* Check for from parameter. */
        const uint256_t hashCredit =
            ExtractAddress(jParams, "to");

        /* Get the token / account object. */
        TAO::Register::Object tObjTo;
        if(!LLD::Register->ReadObject(hashCredit, tObjTo, TAO::Ledger::FLAGS::MEMPOOL))
            throw Exception(-13, "Object [to] not found");

        /* Check for valid types. */
        if(tObjTo.Base() != TAO::Register::OBJECTS::ACCOUNT)
            throw Exception(-13, "Object [from] is not an account");

        /* Loop through all transactions. */
        std::vector<TAO::Operation::Contract> vContracts;
        for(uint32_t nContract = 0; nContract < tx.Size(); ++nContract)
        {
            /* Get the contract. */
            const TAO::Operation::Contract& rContract = tx[nContract];

            /* Get the operation byte. */
            uint8_t nType = 0;
            rContract >> nType;

            /* Switch based on type. */
            switch(nType)
            {
                /* Check that transaction has a condition. */
                case TAO::Operation::OP::CONDITION:
                {
                    /* Get the next OP. */
                    rContract.Seek(1, TAO::Operation::Contract::OPERATIONS);

                    /* Verify the contract byte-code now. */
                    if(Contracts::Verify(Contracts::Exchange::Token[0], rContract)) //checking for version 1
                    {
                        /* Extract our source address. */
                        uint256_t hashFrom;
                        rContract >> hashFrom;

                        /* Get the token / account object. */
                        TAO::Register::Object tOrderFrom;
                        if(!LLD::Register->ReadObject(hashFrom, tOrderFrom, TAO::Ledger::FLAGS::MEMPOOL))
                            throw Exception(-13, "Object not found");

                        /* Check that we have the correct token types. */
                        if(tObjTo.get<uint256_t>("token") != tOrderFrom.get<uint256_t>("token"))
                            throw Exception(-26, "Invalid parameter [to], [type] requires correct token");

                        /* Get the next OP. */
                        rContract.Seek(32, TAO::Operation::Contract::OPERATIONS);

                        /* Get the received amount. */
                        uint64_t nOrderAmount;
                        rContract >> nOrderAmount;

                        /* Get the next OP. */
                        rContract.Seek(4, TAO::Operation::Contract::CONDITIONS);

                        /* Get the comparison bytes. */
                        std::vector<uint8_t> vBytes;
                        rContract >= vBytes;

                        /* Extract the data from the bytes. */
                        TAO::Operation::Stream ssCompare(vBytes);
                        ssCompare.seek(33);

                        /* Get the address to. */
                        uint256_t hashTo;
                        ssCompare >> hashTo;

                        /* Get the token / account object. */
                        TAO::Register::Object tOrderTo;
                        if(!LLD::Register->ReadObject(hashTo, tOrderTo, TAO::Ledger::FLAGS::MEMPOOL))
                            throw Exception(-13, "Object [from] not found");

                        /* Get the amount requested. */
                        uint64_t nAmount = 0;
                        ssCompare >> nAmount;

                        /* Check that we have the correct token types. */
                        if(tObjFrom.get<uint256_t>("token") != tOrderTo.get<uint256_t>("token"))
                            throw Exception(-26, "Invalid parameter [from], [type] requires correct token");

                        /* Build the contract. */
                        TAO::Operation::Contract tValidate;
                        tValidate << uint8_t(TAO::Operation::OP::VALIDATE) << hashOrder   << nContract;
                        tValidate << uint8_t(TAO::Operation::OP::DEBIT)    << hashAddress << hashTo << nAmount << uint64_t(0);

                        /* Make this spendable only by recipient. */
                        tValidate <= uint8_t(TAO::Operation::OP::CALLER::GENESIS);
                        tValidate <= uint8_t(TAO::Operation::OP::NOTEQUALS);
                        tValidate <= uint8_t(TAO::Operation::OP::CONTRACT::GENESIS);

                        /* Add contract to our queue. */
                        vContracts.push_back(tValidate);

                        /* Check for a token match for from account. */
                        uint256_t hashToken = tObjFrom.get<uint256_t>("token");
                        if(mapFees.count(hashToken))
                        {
                            /* Calcuate our debit amount now. */
                            const uint64_t nFees =
                                (nAmount * mapFees[hashToken].second) / 100000;

                            /* Build the contract. */
                            TAO::Operation::Contract tDebit;
                            tDebit << uint8_t(TAO::Operation::OP::DEBIT) << hashAddress;
                            tDebit << mapFees[hashToken].first << nFees << uint64_t(0);

                            /* Make this spendable only by recipient. */
                            tDebit <= uint8_t(TAO::Operation::OP::CALLER::GENESIS);
                            tDebit <= uint8_t(TAO::Operation::OP::NOTEQUALS);
                            tDebit <= uint8_t(TAO::Operation::OP::CONTRACT::GENESIS);

                            /* Add contract to our queue. */
                            vContracts.push_back(tDebit);
                        }

                        /* if we passed all of these checks then insert the credit contract into the tx */
                        TAO::Operation::Contract tCredit;
                        tCredit << uint8_t(TAO::Operation::OP::CREDIT) << hashOrder << uint32_t(nContract);
                        tCredit << hashCredit << hashFrom << nOrderAmount;

                        /* Add contract to our queue. */
                        vContracts.push_back(tCredit);

                        /* Check for a token match for from account. */
                        hashToken = tObjTo.get<uint256_t>("token");
                        if(mapFees.count(hashToken))
                        {
                            /* Calcuate our debit amount now. */
                            const uint64_t nFees =
                                (nOrderAmount * mapFees[hashToken].second) / 100000;

                            /* Build the contract. */
                            TAO::Operation::Contract tDebit;
                            tDebit << uint8_t(TAO::Operation::OP::DEBIT) << hashCredit;
                            tDebit << mapFees[hashToken].first << nFees << uint64_t(0);

                            /* Make this spendable only by recipient. */
                            tDebit <= uint8_t(TAO::Operation::OP::CALLER::GENESIS);
                            tDebit <= uint8_t(TAO::Operation::OP::NOTEQUALS);
                            tDebit <= uint8_t(TAO::Operation::OP::CONTRACT::GENESIS);

                            /* Add contract to our queue. */
                            vContracts.push_back(tDebit);
                        }
                    }

                    break;
                }
            }
        }

        /* Check that output was found. */
        if(vContracts.empty())
            throw Exception(-43, "No valid contracts in tx.");

        return BuildResponse(jParams, hashAddress, vContracts);
    }
}
