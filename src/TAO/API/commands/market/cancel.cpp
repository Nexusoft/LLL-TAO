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
#include <TAO/API/types/commands.h>
#include <TAO/API/types/commands/market.h>

#include <TAO/Operation/include/enum.h>

#include <TAO/Ledger/types/transaction.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Cancel an active order in market. */
    encoding::json Market::Cancel(const encoding::json& jParams, const bool fHelp)
    {
        /* Get our txid. */
        const uint512_t hashOrder = ExtractHash(jParams);

        /* Read the previous transaction. */
        TAO::Ledger::Transaction tx;
        if(!LLD::Ledger->ReadTx(hashOrder, tx))
            throw Exception(-40, "Previous transaction not found.");

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
                    /* Process the contract and attempt to void it */
                    if(!BuildVoid(jParams, nContract, rContract, vContracts))
                        throw Exception(-43, "Void failed to build");

                    break;
                }
            }
        }

        /* Check that output was found. */
        if(vContracts.empty())
            throw Exception(-43, "No valid contracts in tx.");

        return BuildResponse(jParams, vContracts);
    }
}
