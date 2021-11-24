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
#include <LLP/impl/tritium.h>
#include <LLP/include/global.h>

#include <TAO/API/include/json.h>

#include <Util/include/string.h>

#include <Legacy/wallet/wallet.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Retrieves the transaction for the given hash. */
    encoding::json Ledger::SubmitTransaction(const encoding::json& params, const bool fHelp)
    {
        /* Declare the JSON return object */
        encoding::json jRet;

        /* Check for the transaction data parameter. */
        if(params.find("data") == params.end())
            throw Exception(-18, "Missing data");

        /* Extract the data out of the JSON params*/
        std::vector<uint8_t> vData = ParseHex(params["data"].get<std::string>());

        DataStream ssData(vData, SER_NETWORK, LLP::PROTOCOL_VERSION);

        /* Deserialize the tx. */
        uint8_t nType;
        ssData >> nType;

        /* Check the transaction type so that we can deserialize the correct class */
        if(nType == LLP::MSG_TX_TRITIUM)
        {
            TAO::Ledger::Transaction tx;
            ssData >> tx;

            /* Check if we have it. */
            if(!LLD::Ledger->HasTx(tx.GetHash()))
            {
                /* Add the transaction to the memory pool. */
                if(TAO::Ledger::mempool.Accept(tx, nullptr))
                    jRet["hash"] = tx.GetHash().ToString();
                else
                    throw Exception(-150, "Transaction rejected.");
            }
            else
                throw Exception(-151, "Transaction already in database.");

        }
        else if(nType == LLP::MSG_TX_LEGACY)
        {
            Legacy::Transaction tx;
            ssData >> tx;


            /* Check if we have it. */
            if(!LLD::Legacy->HasTx(tx.GetHash()))
            {
                /* Check if tx is valid. */
                if(!tx.CheckTransaction())
                    throw Exception(-150, "Transaction rejected.");

                /* Add the transaction to the memory pool. */
                if(TAO::Ledger::mempool.Accept(tx))
                {
                    #ifndef NO_WALLET

                    /* Add tx to legacy wallet */
                    TAO::Ledger::BlockState notUsed;
                    Legacy::Wallet::Instance().AddToWalletIfInvolvingMe(tx, notUsed, true);

                    #endif

                    jRet["hash"] = tx.GetHash().ToString();
                }
                else
                    throw Exception(-150, "Transaction rejected.");
            }
            else
                throw Exception(-151, "Transaction already in database.");

        }

        return jRet;

    }
}/* End Ledger namespace */
