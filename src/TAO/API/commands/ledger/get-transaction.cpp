/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <LLP/include/inv.h>

#include <TAO/API/types/commands/ledger.h>

#include <TAO/API/include/extract.h>

#include <TAO/Ledger/types/transaction.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Retrieves the transaction for the given hash. */
    encoding::json Ledger::GetTransaction(const encoding::json& jParams, const bool fHelp)
    {
        /* Extract the hash out of the JSON parameters. */
        const uint512_t hashTx = ExtractHash(jParams);

        /* Default to verbosity 1 which includes only the hash */
        const uint32_t nVerbose =
            ExtractVerbose(jParams, 2);

        /* Get the return format */
        const std::string strFormat =
            ExtractFormat(jParams, "json", "json, raw");

        /* Attempt to get the block. */
        TAO::Ledger::BlockState rBlock;
        LLD::Ledger->ReadBlock(hashTx, rBlock);

        /* Switch based on type. */
        switch(hashTx.GetType())
        {
            /* Handle for tritium transaction. */
            case TAO::Ledger::TRITIUM:
            {
                /* Grab our transaction and block. */
                TAO::Ledger::Transaction rTx;
                if(!LLD::Ledger->ReadTx(hashTx, rTx, TAO::Ledger::FLAGS::MEMPOOL))
                    throw Exception(-87, "Invalid or unknown transaction");

                /* Handle for json format. */
                if(strFormat == "json")
                    return TransactionToJSON(rTx, rBlock, nVerbose);

                /* Handle for raw format. */
                if(strFormat == "raw")
                {
                    /* Build a transaction datastream. */
                    DataStream ssTx(SER_NETWORK, LLP::PROTOCOL_VERSION);

                    /* Serialize transaction data. */
                    ssTx << uint8_t(LLP::MSG_TX_TRITIUM) << rTx;

                    /* Add data string for return. */
                    const encoding::json jRet =
                        { { "data", HexStr(ssTx.begin(), ssTx.end()) } };

                    return jRet;
                }

                break;
            }

            /* Handle for legacy transaction. */
            case TAO::Ledger::LEGACY:
            {
                /* Grab our transaction and block. */
                Legacy::Transaction rTx;
                if(!LLD::Legacy->ReadTx(hashTx, rTx, TAO::Ledger::FLAGS::MEMPOOL))
                    throw Exception(-87, "Invalid or unknown transaction");

                /* Handle for json format. */
                if(strFormat == "json")
                    return TransactionToJSON(rTx, rBlock, nVerbose);

                /* Handle for raw format. */
                if(strFormat == "raw")
                {
                    /* Build a transaction datastream. */
                    DataStream ssTx(SER_NETWORK, LLP::PROTOCOL_VERSION);

                    /* Serialize transaction data. */
                    ssTx << uint8_t(LLP::MSG_TX_LEGACY) << rTx;

                    /* Add data string for return. */
                    const encoding::json jRet =
                        { { "data", HexStr(ssTx.begin(), ssTx.end()) } };

                    return jRet;
                }

                break;
            }
        }

        throw Exception(-87, "Invalid or unknown transaction");
    }
}
