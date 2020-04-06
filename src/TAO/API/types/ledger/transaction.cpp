/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/types/ledger.h>
#include <TAO/API/include/global.h>

#include <LLD/include/global.h>
#include <TAO/Ledger/types/mempool.h>

#include <LLP/include/inv.h>
#include <LLP/types/tritium.h>
#include <LLP/include/global.h>

#include <TAO/API/include/utils.h>
#include <TAO/API/include/json.h>

#include <Util/include/string.h>

#include <Legacy/wallet/wallet.h>


/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {
        /* Retrieves the transaction for the given hash. */
        json::json Ledger::Transaction(const json::json& params, bool fHelp)
        {
            if(config::fClient.load())
                throw APIException(-300, "API can only be used to lookup data for the currently logged in signature chain when running in client mode");

            /* Extract the hash out of the JSON params*/
            uint512_t hash;
            if(params.find("hash") != params.end())
                hash.SetHex(params["hash"].get<std::string>());
            else if(params.find("txid") != params.end())
                hash.SetHex(params["txid"].get<std::string>());
            else
                throw APIException(-86, "Missing hash or txid");


            /* Get the transaction verbosity level from the request*/
            std::string strVerbose = "default";
            if(params.find("verbose") != params.end())
                strVerbose = params["verbose"].get<std::string>();

            /* Default to verbosity 1 which includes only the hash */
            uint32_t nVerbose = 1;
            if(strVerbose == "default")
                nVerbose = 2;
            else if(strVerbose == "summary")
                nVerbose = 2;
            else if(strVerbose == "detail")
                nVerbose = 3;

            /* Get the return format */
            std::string strFormat = "JSON";
            if(params.find("format") != params.end() && params["format"].get<std::string>() == "raw")
                strFormat = "raw";

            /* Get the callers genesis hash so that we can use it to include name data in the response */
            uint512_t hashCaller = users->GetCallersGenesis(params);

            /* Declare the tritium and legacy transaction objects ready to attempt to load them from disk*/
            TAO::Ledger::Transaction txTritium;
            Legacy::Transaction txLegacy;

            //NOTE: this code here is messy, needs better formatting
            TAO::Ledger::BlockState blockState;


            /* Read the block state from the the ledger DB using the transaction hash index */
            LLD::Ledger->ReadBlock(hash, blockState);


            /* Declare the JSON return object */
            json::json ret;


            /* Get the transaction either from disk or mempool. */
            if(LLD::Ledger->ReadTx(hash, txTritium, TAO::Ledger::FLAGS::MEMPOOL))
            {
                if(strFormat == "JSON")
                    ret = TAO::API::TransactionToJSON (hashCaller, txTritium, blockState, nVerbose);
                else
                {
                    DataStream ssTx(SER_NETWORK, LLP::PROTOCOL_VERSION);
                    ssTx << (uint8_t)LLP::MSG_TX_TRITIUM << txTritium;
                    ret["data"] = HexStr(ssTx.begin(), ssTx.end());
                }

            }

            /* If it is not a tritium transaction then see if it is a legacy tx in the legacy DB */
            else if(LLD::Legacy->ReadTx(hash, txLegacy, TAO::Ledger::FLAGS::MEMPOOL))
            {
                if(strFormat == "JSON")
                    ret = TAO::API::TransactionToJSON(txLegacy, blockState, nVerbose);
                else
                {
                    DataStream ssTx(SER_NETWORK, LLP::PROTOCOL_VERSION);
                    ssTx << (uint8_t)LLP::MSG_TX_LEGACY << txLegacy;
                    ret["data"]  = HexStr(ssTx.begin(), ssTx.end());
                }
            }

            else
            {
                throw APIException(-87, "Invalid or unknown transaction");
            }

            return ret;
        }


        /* Retrieves the transaction for the given hash. */
        json::json Ledger::Submit(const json::json& params, bool fHelp)
        {
            /* Declare the JSON return object */
            json::json ret;

            /* Check for the transaction data parameter. */
            if(params.find("data") == params.end())
                throw APIException(-18, "Missing data");

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
                        ret["hash"] = tx.GetHash().ToString();
                    else
                        throw APIException(-150, "Transaction rejected.");
                }
                else
                    throw APIException(-151, "Transaction already in database.");

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
                        throw APIException(-150, "Transaction rejected.");

                    /* Add the transaction to the memory pool. */
                    if(TAO::Ledger::mempool.Accept(tx))
                    {
                        /* Add tx to legacy wallet */
                        TAO::Ledger::BlockState notUsed;
                        Legacy::Wallet::GetInstance().AddToWalletIfInvolvingMe(tx, notUsed, true);

                        ret["hash"] = tx.GetHash().ToString();
                    }
                    else
                        throw APIException(-150, "Transaction rejected.");
                }
                else
                    throw APIException(-151, "Transaction already in database.");

            }

            return ret;

        }


    } /* End API namespace */

}/* End Ledger namespace */
