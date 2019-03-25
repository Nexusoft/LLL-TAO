/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/



#include <TAO/API/include/apiutils.h>

#include <LLD/include/global.h>

#include <TAO/Ledger/include/constants.h>
#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/include/difficulty.h>
#include <TAO/Ledger/types/tritium.h>

#include <TAO/Operation/include/output.h>

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

            json::json blockToJSON(const TAO::Ledger::BlockState& block,  int nTransactionVerbosity)
            {
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

                json::json txinfo = json::json::array();

                for (const auto& vtx : block.vtx)
                {
                    if(vtx.first == TAO::Ledger::TYPE::TRITIUM_TX)
                    {

                        /* Get the tritium transaction  from the database*/
                        TAO::Ledger::Transaction tx;
                        if(LLD::legDB->ReadTx(vtx.second, tx))
                        {
                            if (nTransactionVerbosity > 0)
                            {
                                json::json txdata;

                                txdata["type"] = tx.GetTxTypeString();
                                txdata["version"]   = tx.nVersion;
                                txdata["sequence"]  = tx.nSequence;
                                txdata["timestamp"] = tx.nTimestamp;

                                /* Genesis and hashes are verbose 2 and up. */
                                if(nTransactionVerbosity >= 2)
                                {
                                    txdata["genesis"]   = tx.hashGenesis.ToString();
                                    txdata["nexthash"]  = tx.hashNext.ToString();
                                    txdata["prevhash"]  = tx.hashPrevTx.ToString();
                                }

                                /* Signatures and public keys are verbose level 3 and up. */
                                if(nTransactionVerbosity >= 3)
                                {
                                    txdata["pubkey"]    = HexStr(tx.vchPubKey.begin(), tx.vchPubKey.end());
                                    txdata["signature"] = HexStr(tx.vchSig.begin(),    tx.vchSig.end());
                                }

                                txdata["hash"]       = tx.GetHash().ToString();
                                txdata["operation"]  = TAO::Operation::Output(tx);

                                txinfo.push_back(txdata);
                            }
                            else
                                txinfo.push_back(tx.GetHash().GetHex());
                        }
                    }
                    else if(vtx.first == TAO::Ledger::TYPE::LEGACY_TX)
                    {
                        /* Get the legacy transaction from the database. */
                        Legacy::Transaction tx;
                        if(LLD::legacyDB->ReadTx(vtx.second, tx))
                        {
                            if (nTransactionVerbosity > 0)
                            {
                                json::json txdata;

                                txdata["timestamp"] = convert::DateTimeStrFormat(tx.nTime);
                                txdata["type"] = tx.GetTxTypeString();
                                txdata["hash"] = tx.GetHash().GetHex();

                                json::json vin = json::json::array();
                                for(const Legacy::TxIn& txin : tx.vin)
                                    vin.push_back(txin.ToStringShort());
                                txdata["vin"] = vin;

                                json::json vout = json::json::array();
                                for(const Legacy::TxOut& txout : tx.vout)
                                    vout.push_back(txout.ToStringShort());
                                txdata["vout"] = vout;

                                txinfo.push_back(txdata);

                            }
                            else
                                txinfo.push_back(tx.GetHash().GetHex());
                        }
                    }
                }

                result["tx"] = txinfo;
                return result;
            }

        }

    }

}