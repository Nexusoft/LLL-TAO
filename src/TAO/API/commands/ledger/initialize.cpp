/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/types/commands/ledger.h>

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        /* Standard initialization function. */
        void Ledger::Initialize()
        {
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


        /* Allows derived API's to handle custom/dynamic URL's where the strMethod does not
        *  map directly to a function in the target API.  Insted this method can be overriden to
        *  parse the incoming URL and route to a different/generic method handler, adding parameter
        *  values if necessary.  E.g. get/myasset could be rerouted to get/asset with name=myasset
        *  added to the jsonParams
        *  The return json contains the modifed method URL to be called.
        */
        std::string Ledger::RewriteURL(const std::string& strMethod, encoding::json& jsonParams)
        {
            std::string strMethodRewritten = strMethod;
            std::size_t nPos = strMethodRewritten.find("transaction/");

            if(nPos != std::string::npos)
            {
                /* get the method name from before the transaction/ */
                strMethodRewritten = strMethod.substr(0, nPos+11);

                /* Get the transaction id that comes after the transaction/ part. */
                std::string strTxid = strMethod.substr(nPos + 12);

                /* Set the transaction id parameter for ledger API. */
                jsonParams["hash"] = strTxid;
            }

            return strMethodRewritten;
        }
    }
}
