/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/types/commands/ledger.h>
#include <TAO/API/include/global.h>

#include <LLD/include/global.h>
#include <TAO/Ledger/types/mempool.h>

#include <LLP/include/inv.h>
#include <LLP/types/tritium.h>
#include <LLP/include/global.h>

#include <TAO/API/include/json.h>

#include <Util/include/string.h>

#include <Legacy/wallet/wallet.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Retrieves the transaction for the given hash. */
    encoding::json Ledger::SubmitTransaction(const encoding::json& jParams, const bool fHelp)
    {
        /* Declare the JSON return object */
        encoding::json jRet;

        /* Check for the transaction data parameter. */
        if(jParams.find("data") == jParams.end())
            throw Exception(-18, "Missing data");

        /* Extract the data out of the JSON parameters*/
        const DataStream ssData =
            DataStream(ParseHex(jParams["data"].get<std::string>()), SER_NETWORK, LLP::PROTOCOL_VERSION);

        /* Deserialize the tx. */
        uint8_t nType;
        ssData >> nType;

        /* Handle for a serialized tritium transaction. */
        if(nType == LLP::MSG_TX_TRITIUM)
        {
            /* Deserialize the transaction object. */
            TAO::Ledger::Transaction tx;
            ssData >> tx;

            /* Accept the transaction to the memory pool. */
            if(!TAO::Ledger::mempool.Accept(tx, nullptr))
                throw Exception(-150, "Transaction rejected.");

            /* If accepted add txid to returned json. */
            jRet["hash"] = tx.GetHash().ToString();
        }

        /* Handle for a serialized legacy transaction. */
        else if(nType == LLP::MSG_TX_LEGACY)
        {
            /* Deserialize the transaction object. */
            Legacy::Transaction tx;
            ssData >> tx;

            /* Check if tx is valid. */
            if(!tx.Check())
                throw Exception(-150, "Transaction rejected.");

            /* Add the transaction to the memory pool. */
            if(!TAO::Ledger::mempool.Accept(tx))
                throw Exception(-150, "Transaction rejected.");

#ifndef NO_WALLET

            /* Add tx to legacy wallet */
            TAO::Ledger::BlockState tDummy;
            Legacy::Wallet::Instance().AddToWalletIfInvolvingMe(tx, tDummy, true);

#endif

            /* If accepted add txid to returned json. */
            jRet["hash"] = tx.GetHash().ToString();
        }

        return jRet;
    }
}
