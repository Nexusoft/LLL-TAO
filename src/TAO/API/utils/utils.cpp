/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/



#include <TAO/API/include/utils.h>
#include <TAO/API/types/exception.h>

#include <LLD/include/global.h>

#include <TAO/Ledger/include/constants.h>
#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/include/difficulty.h>
#include <TAO/Ledger/types/tritium.h>

#include <TAO/Operation/include/output.h>

#include <Legacy/include/evaluate.h>

#include <Util/include/args.h>
#include <Util/include/hex.h>
#include <Util/include/json.h>


/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {
        /* Utils Layer namespace. */
        namespace Utils
        {
            /* Converts the block to formatted JSON */
            json::json blockToJSON(const TAO::Ledger::BlockState& block,  int nTransactionVerbosity)
            {
                /* Decalre the response object*/
                json::json result;

                result["hash"] = block.GetHash().GetHex();
                // the hash that was relevant for Proof of Stake or Proof of Work (depending on block version)
                result["proofhash"] =
                                        block.nVersion < 5 ? block.GetHash().GetHex() :
                                        ((block.nChannel == 0) ? block.StakeHash().GetHex() : block.ProofHash().GetHex());

                result["size"] = (int)::GetSerializeSize(block, SER_NETWORK, LLP::PROTOCOL_VERSION);
                result["height"] = (int)block.nHeight;
                result["channel"] = (int)block.nChannel;
                result["version"] = (int)block.nVersion;
                result["merkleroot"] = block.hashMerkleRoot.GetHex();
                result["time"] = convert::DateTimeStrFormat(block.GetBlockTime());
                result["nonce"] = (uint64_t)block.nNonce;
                result["bits"] = HexBits(block.nBits);
                result["difficulty"] = TAO::Ledger::GetDifficulty(block.nBits, block.nChannel);
                result["mint"] = Legacy::SatoshisToAmount(block.nMint);
                if (block.hashPrevBlock != 0)
                    result["previousblockhash"] = block.hashPrevBlock.GetHex();
                if (block.hashNextBlock != 0)
                    result["nextblockhash"] = block.hashNextBlock.GetHex();

                /* Add the transaction data if the caller has requested it*/
                if(nTransactionVerbosity > 0)
                {
                    json::json txinfo = json::json::array();

                    /* Iterate through each transaction hash in the block vtx*/
                    for (const auto& vtx : block.vtx)
                    {
                        if(vtx.first == TAO::Ledger::TYPE::TRITIUM_TX)
                        {
                            /* Get the tritium transaction from the database*/
                            TAO::Ledger::Transaction tx;
                            if(LLD::legDB->ReadTx(vtx.second, tx))
                            {
                                /* add the transaction JSON.  */ 
                                json::json txdata = transactionToJSON(tx, block, nTransactionVerbosity);

                                txinfo.push_back(txdata);
                            }
                        }
                        else if(vtx.first == TAO::Ledger::TYPE::LEGACY_TX)
                        {
                            /* Get the legacy transaction from the database. */
                            Legacy::Transaction tx;
                            if(LLD::legacyDB->ReadTx(vtx.second, tx))
                            {
                                /* add the transaction JSON.  */ 
                                json::json txdata = transactionToJSON(tx, block, nTransactionVerbosity);

                                txinfo.push_back(txdata);

                            }
                        }
                    }
                    /* Add the transaction data to the response */
                    result["tx"] = txinfo;
                }

                return result;
            }

            /* Converts the transaction to formatted JSON */
            json::json transactionToJSON( TAO::Ledger::Transaction& tx, const TAO::Ledger::BlockState& block, int nTransactionVerbosity)
            {
                /* Declare JSON object to return */
                json::json txdata;

                /* Always add the hash if level 1 and up*/
                if( nTransactionVerbosity >= 1)
                    txdata["hash"] = tx.GetHash().ToString();

                /* Basic TX info for level 2 and up */
                if (nTransactionVerbosity >= 2)
                {
                    txdata["type"] = tx.GetTxTypeString();
                    txdata["version"] = tx.nVersion;
                    txdata["sequence"] = tx.nSequence;
                    txdata["timestamp"] = tx.nTimestamp;
                    txdata["confirmations"] = block.IsNull() ? 0 : TAO::Ledger::ChainState::nBestHeight.load() - block.nHeight + 1;


                    /* Genesis and hashes are verbose 3 and up. */
                    if(nTransactionVerbosity >= 3)
                    {
                        txdata["genesis"] = tx.hashGenesis.ToString();
                        txdata["nexthash"] = tx.hashNext.ToString();
                        txdata["prevhash"] = tx.hashPrevTx.ToString();
                    }

                    /* Signatures and public keys are verbose level 4 and up. */
                    if(nTransactionVerbosity >= 4)
                    {
                        txdata["pubkey"] = HexStr(tx.vchPubKey.begin(), tx.vchPubKey.end());
                        txdata["signature"] = HexStr(tx.vchSig.begin(),    tx.vchSig.end());
                    }

                    txdata["operation"]  = TAO::Operation::Output(tx);                    
                }

                return txdata;
            }

            /* Converts the transaction to formatted JSON */
            json::json transactionToJSON( Legacy::Transaction& tx, const TAO::Ledger::BlockState& block, int nTransactionVerbosity)
            {
                /* Declare JSON object to return */
                json::json txdata;

                /* Always add the hash */
                txdata["hash"] = tx.GetHash().GetHex();

                /* Basic TX info for level 1 and up */
                if (nTransactionVerbosity > 0)
                {
                    txdata["type"] = tx.GetTxTypeString();
                    txdata["timestamp"] = tx.nTime;
                    txdata["amount"] = Legacy::SatoshisToAmount( tx.GetValueOut());
                    txdata["confirmations"] = block.IsNull() ? 0 : TAO::Ledger::ChainState::nBestHeight.load() - block.nHeight + 1;

                    /* Don't add inputs for coinbase or coinstake transactions */
                    if(!tx.IsCoinBase() && !tx.IsCoinStake())
                    {            
                        /* Declare the inputs JSON array */
                        json::json inputs = json::json::array();
                        /* Iterate through each input */
                        for(const Legacy::TxIn& txin : tx.vin)
                        {
                            /* Read the previous transaction in order to get its outputs */
                            Legacy::Transaction txprev;
                            if(!LLD::legacyDB->ReadTx(txin.prevout.hash, txprev))
                                throw APIException(-5, "Invalid transaction id");

                            Legacy::NexusAddress cAddress;
                            if(!Legacy::ExtractAddress(txprev.vout[txin.prevout.n].scriptPubKey, cAddress))
                                throw APIException(-5, "Unable to Extract Input Address");
                        
                            inputs.push_back(debug::safe_printstr("%s:%f", cAddress.ToString().c_str(), (double) tx.vout[txin.prevout.n].nValue / Legacy::COIN));
                        }
                        txdata["inputs"] = inputs;
                    }

                    /* Declare the output JSON array */
                    json::json outputs = json::json::array();
                    /* Iterate through each output */
                    for(const Legacy::TxOut& txout : tx.vout)
                    {
                        Legacy::NexusAddress cAddress;
                        if(!Legacy::ExtractAddress(txout.scriptPubKey, cAddress))
                            throw APIException(-5, "Unable to Extract Input Address");
                        
                        outputs.push_back(debug::safe_printstr("%s:%f", cAddress.ToString().c_str(), (double) txout.nValue / Legacy::COIN));
                    }
                    txdata["outputs"] = outputs;
                }

                return txdata;
            }

        }

    }

}