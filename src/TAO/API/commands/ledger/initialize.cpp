/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/types/commands/ledger.h>
#include <TAO/API/types/commands/operators.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Standard initialization function. */
    void Ledger::Initialize()
    {
        /* Add the SUM operator. */
        mapOperators["sum"] = Operator
        (
            std::bind
            (
                &Operators::Sum,
                std::placeholders::_1,
                std::placeholders::_2
            )
        );


        /* Handle for get/blockhash. */
        mapFunctions["get/blockhash"] = Function
        (
            std::bind
            (
                &Ledger::GetBlockHash,
                this,
                std::placeholders::_1,
                std::placeholders::_2
            )
        );

        /* Handle for get/block. */
        mapFunctions["get/block"] = Function
        (
            std::bind
            (
                &Ledger::GetBlock,
                this,
                std::placeholders::_1,
                std::placeholders::_2
            )
        );

        /* Handle for list/blocks. */
        mapFunctions["list/blocks"] = Function
        (
            std::bind
            (
                &Ledger::ListBlocks,
                this,
                std::placeholders::_1,
                std::placeholders::_2
            )
        );

        /* Handle for get/transaction. */
        mapFunctions["get/transaction"] = Function
        (
            std::bind
            (
                &Ledger::GetTransaction,
                this,
                std::placeholders::_1,
                std::placeholders::_2
            )
        );

        /* Handle for get/mininginfo. */
        mapFunctions["get/mininginfo"] = Function
        (
            std::bind
            (
                &Ledger::GetMiningInfo,
                this,
                std::placeholders::_1,
                std::placeholders::_2
            )
        );

        /* Handle for submit/transaction. */
        mapFunctions["submit/transaction"] = Function
        (
            std::bind
            (
                &Ledger::SubmitTransaction,
                this,
                std::placeholders::_1,
                std::placeholders::_2
            )
        );

        /* Handle for sync/headers. */
        mapFunctions["sync/headers"] = Function
        (
            std::bind
            (
                &Ledger::SyncHeaders,
                this,
                std::placeholders::_1,
                std::placeholders::_2
            )
        );

        /* Handle for sync/sigchain. */
        mapFunctions["sync/sigchain"] = Function
        (
            std::bind
            (
                &Ledger::SyncSigChain,
                this,
                std::placeholders::_1,
                std::placeholders::_2
            )
        );

        /* Handle for void/transaction. */
        mapFunctions["void/transaction"] = Function
        (
            std::bind
            (
                &Ledger::VoidTransaction,
                this,
                std::placeholders::_1,
                std::placeholders::_2
            )
        );
    }
}
