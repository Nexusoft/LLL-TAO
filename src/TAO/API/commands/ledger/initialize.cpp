/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/types/commands/ledger.h>
#include <TAO/API/types/commands/templates.h>
#include <TAO/API/types/operators/array.h>
#include <TAO/API/types/operators/mean.h>
#include <TAO/API/types/operators/sum.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Standard initialization function. */
    void Ledger::Initialize()
    {
        /* Handle for the SUM operator. */
        mapOperators["sum"] = Operator
        (
            std::bind
            (
                &Operators::Sum,
                std::placeholders::_1,
                std::placeholders::_2
            )
        );

        /* Handle for the ARRAY operator. */
        mapOperators["array"] = Operator
        (
            std::bind
            (
                &Operators::Array,
                std::placeholders::_1,
                std::placeholders::_2
            )
        );

        /* Handle for the MEAN operator. */
        mapOperators["mean"] = Operator
        (
            std::bind
            (
                &Operators::Mean,
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

        /* Handle for get/block. */
        mapFunctions["get/info"] = Function
        (
            std::bind
            (
                &Ledger::GetInfo,
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


        /* DEPRECATED */
        mapFunctions["get/mininginfo"] = Function
        (
            std::bind
            (
                &Templates::Deprecated,
                std::placeholders::_1,
                std::placeholders::_2
            )
            , version::get_version(5, 1, 0)
            , "please use ledger/get/info command instead"
        );
    }
}
