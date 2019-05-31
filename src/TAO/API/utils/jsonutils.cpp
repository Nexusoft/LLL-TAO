/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/include/jsonutils.h>
#include <TAO/API/include/utils.h>

#include <Legacy/include/evaluate.h>
#include <Legacy/include/money.h>

#include <LLD/include/global.h>

#include <TAO/API/include/global.h>

#include <TAO/API/types/exception.h>

#include <TAO/Ledger/include/constants.h>
#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/include/difficulty.h>
#include <TAO/Ledger/types/tritium.h>
#include <TAO/Ledger/types/mempool.h>

#include <TAO/Operation/include/stream.h>
#include <TAO/Operation/include/enum.h>
#include <TAO/Operation/include/operations.h>

#include <Util/include/args.h>
#include <Util/include/convert.h>
#include <Util/include/hex.h>
#include <Util/include/json.h>
#include <Util/include/base64.h>



/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        /* Converts the block to formatted JSON */
        json::json BlockToJSON(const TAO::Ledger::BlockState& block, uint32_t nTransactionVerbosity)
        {
            /* Decalre the response object*/
            json::json result;

            result["hash"] = block.GetHash().GetHex();
            // the hash that was relevant for Proof of Stake or Proof of Work (depending on block version)
            result["proofhash"] =
                                    block.nVersion < 5 ? block.GetHash().GetHex() :
                                    ((block.nChannel == 0) ? block.StakeHash().GetHex() : block.ProofHash().GetHex());

            result["size"] = (uint32_t)::GetSerializeSize(block, SER_NETWORK, LLP::PROTOCOL_VERSION);
            result["height"] = (uint32_t)block.nHeight;
            result["channel"] = (uint32_t)block.nChannel;
            result["version"] = (uint32_t)block.nVersion;
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
                for(const auto& vtx : block.vtx)
                {
                    if(vtx.first == TAO::Ledger::TYPE::TRITIUM_TX)
                    {
                        /* Get the tritium transaction from the database*/
                        TAO::Ledger::Transaction tx;
                        if(LLD::legDB->ReadTx(vtx.second, tx))
                        {
                            /* add the transaction JSON.  */
                            json::json txdata = TransactionToJSON(tx, block, nTransactionVerbosity);

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
                            json::json txdata = TransactionToJSON(tx, block, nTransactionVerbosity);

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
        json::json TransactionToJSON(TAO::Ledger::Transaction& tx, const TAO::Ledger::BlockState& block, uint32_t nTransactionVerbosity)
        {
            /* Declare JSON object to return */
            json::json txdata;

            /* Always add the hash if level 1 and up*/
            if(nTransactionVerbosity >= 1)
                txdata["hash"] = tx.GetHash().ToString();

            /* Basic TX info for level 2 and up */
            if(nTransactionVerbosity >= 2)
            {
                txdata["type"] = tx.GetTxTypeString();
                txdata["version"] = tx.nVersion;
                txdata["sequence"] = tx.nSequence;
                txdata["timestamp"] = tx.nTimestamp;
                txdata["operation"]  = OperationToJSON(tx.ssOperation);
                txdata["confirmations"] = block.IsNull() ? 0 : TAO::Ledger::ChainState::nBestHeight.load() - block.nHeight + 1;

                /* Genesis and hashes are verbose 3 and up. */
                if(nTransactionVerbosity >= 3)
                {
                    txdata["genesis"] = tx.hashGenesis.ToString();
                    txdata["nexthash"] = tx.hashNext.ToString();
                    txdata["prevhash"] = tx.hashPrevTx.ToString();

                    txdata["pubkey"] = HexStr(tx.vchPubKey.begin(), tx.vchPubKey.end());
                    txdata["signature"] = HexStr(tx.vchSig.begin(),    tx.vchSig.end());
                }


            }

            return txdata;
        }

        /* Converts the transaction to formatted JSON */
        json::json TransactionToJSON(Legacy::Transaction& tx, const TAO::Ledger::BlockState& block, uint32_t nTransactionVerbosity)
        {
            /* Declare JSON object to return */
            json::json txdata;

            /* Always add the hash */
            txdata["hash"] = tx.GetHash().GetHex();

            /* Basic TX info for level 1 and up */
            if(nTransactionVerbosity > 0)
            {
                txdata["type"] = tx.GetTxTypeString();
                txdata["timestamp"] = tx.nTime;
                txdata["amount"] = Legacy::SatoshisToAmount(tx.GetValueOut());
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

                        inputs.push_back(debug::safe_printstr("%s:%f",
                                cAddress.ToString().c_str(), (double) tx.vout[txin.prevout.n].nValue / TAO::Ledger::NXS_COIN));
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


        /* Converts a serialized operation stream to formattted JSON */
        json::json OperationToJSON(const TAO::Operation::Stream& ssOperation)
        {
            /* Declare the return JSON object*/
            json::json ret;

            /* Start the stream at the beginning. */
            ssOperation.seek(0, STREAM::BEGIN);

            /* Make sure no exceptions are thrown. */
            try
            {
                /* Loop through the operations ssOperation. */
                while(!ssOperation.end())
                {
                    json::json op;
                    uint8_t OPERATION;
                    ssOperation >> OPERATION;

                    /* Check the current opcode. */
                    switch(OPERATION)
                    {
                        /* Record a new state to the register. */
                        case TAO::Operation::OP::WRITE:
                        {
                            /* Get the Address of the Register. */
                            uint256_t hashAddress;
                            ssOperation >> hashAddress;

                            /* Deserialize the register from ssOperation. */
                            std::vector<uint8_t> vchData;
                            ssOperation >> vchData;

                            /* Output the json information. */
                            op["OP"]      = "WRITE";
                            op["address"] = hashAddress.ToString();
                            op["data"]    = HexStr(vchData.begin(), vchData.end());

                            break;
                        }


                        /* Append a new state to the register. */
                        case TAO::Operation::OP::APPEND:
                        {
                            /* Get the Address of the Register. */
                            uint256_t hashAddress;
                            ssOperation >> hashAddress;

                            /* Deserialize the register from ssOperation. */
                            std::vector<uint8_t> vchData;
                            ssOperation >> vchData;

                            /* Output the json information. */
                            op["OP"]      = "APPEND";
                            op["address"] = hashAddress.ToString();
                            op["data"]    = HexStr(vchData.begin(), vchData.end());

                            break;
                        }


                        /* Create a new register. */
                        case TAO::Operation::OP::REGISTER:
                        {
                            /* Extract the address from the ssOperation. */
                            uint256_t hashAddress;
                            ssOperation >> hashAddress;

                            /* Extract the register type from ssOperation. */
                            uint8_t nType;
                            ssOperation >> nType;

                            /* Extract the register data from the ssOperation. */
                            std::vector<uint8_t> vchData;
                            ssOperation >> vchData;

                            /* Output the json information. */
                            op["OP"]      = "REGISTER";
                            op["address"] = hashAddress.ToString();
                            op["type"]    = nType;
                            op["data"]    = HexStr(vchData.begin(), vchData.end());


                            break;
                        }


                        /* Transfer ownership of a register to another signature chain. */
                        case TAO::Operation::OP::TRANSFER:
                        {
                            /* Extract the address from the ssOperation. */
                            uint256_t hashAddress;
                            ssOperation >> hashAddress;

                            /* Read the register transfer recipient. */
                            uint256_t hashTransfer;
                            ssOperation >> hashTransfer;

                            /* Output the json information. */
                            op["OP"]       = "TRANSFER";
                            op["address"]  = hashAddress.ToString();
                            op["transfer"] = hashTransfer.ToString();

                            break;
                        }

                        /* Transfer ownership of a register to another signature chain. */
                        case TAO::Operation::OP::CLAIM:
                        {
                            /* Extract the tx id of the corresponding transfer from the ssOperation. */
                            uint512_t hashTransferTx;
                            ssOperation >> hashTransferTx;

                            /* Read the claimed transaction. */
                            TAO::Ledger::Transaction txClaim;

                            /* Check disk of writing new block. */
                            if((!LLD::legDB->ReadTx(hashTransferTx, txClaim) || !LLD::legDB->HasIndex(hashTransferTx)))
                                return debug::error(FUNCTION, hashTransferTx.ToString(), " tx doesn't exist or not indexed");

                            /* Check mempool or disk if not writing. */
                            else if(!TAO::Ledger::mempool.Get(hashTransferTx, txClaim)
                            && !LLD::legDB->ReadTx(hashTransferTx, txClaim))
                                return debug::error(FUNCTION, hashTransferTx.ToString(), " tx doesn't exist");

                            /* Extract the state from tx. */
                            uint8_t TX_OP;
                            txClaim.ssOperation >> TX_OP;

                            /* Extract the address  */
                            uint256_t hashAddress;
                            txClaim.ssOperation >> hashAddress;

                            /* Output the json information. */
                            op["OP"]       = "CLAIM";
                            op["txid"] = hashTransferTx.ToString();
                            op["address"]  = hashAddress.ToString();

                            break;
                        }


                        /* Debit tokens from an account you own. */
                        case TAO::Operation::OP::DEBIT:
                        {
                            uint256_t hashAddress; //the register address debit is being sent from. Hard reject if this register isn't account id
                            ssOperation >> hashAddress;

                            uint256_t hashTransfer;   //the register address debit is being sent to. Hard reject if this register isn't an account id
                            ssOperation >> hashTransfer;

                            uint64_t  nAmount;  //the amount to be transfered
                            ssOperation >> nAmount;

                            /* Output the json information. */
                            op["OP"]       = "DEBIT";
                            op["address_from"]  = hashAddress.ToString();
                            op["address_to"] = hashTransfer.ToString();
                            op["amount"]   = nAmount;

                            break;
                        }


                        /* Credit tokens to an account you own. */
                        case TAO::Operation::OP::CREDIT:
                        {
                            /* The transaction that this credit is claiming. */
                            uint512_t hashTx;
                            ssOperation >> hashTx;

                            /* The proof this credit is using to make claims. */
                            uint256_t hashProof;
                            ssOperation >> hashProof;

                            /* The account that is being credited. */
                            uint256_t hashAccount;
                            ssOperation >> hashAccount;

                            /* Read the corresponding debit transaction. */
                            TAO::Ledger::Transaction txDebit;

                            /* Check disk of writing new block. */
                            if((!LLD::legDB->ReadTx(hashTx, txDebit) || !LLD::legDB->HasIndex(hashTx)))
                                return debug::error(FUNCTION, hashTx.ToString(), " tx doesn't exist or not indexed");

                            /* Check mempool or disk if not writing. */
                            else if(!TAO::Ledger::mempool.Get(hashTx, txDebit)
                            && !LLD::legDB->ReadTx(hashTx, txDebit))
                                return debug::error(FUNCTION, hashTx.ToString(), " tx doesn't exist");

                            /* Extract the state from tx. */
                            uint8_t TX_OP;
                            txDebit.ssOperation >> TX_OP;

                            uint256_t hashDebitAddress; //the register address debit is being sent from.
                            txDebit.ssOperation >> hashDebitAddress;

                            txDebit.ssOperation >> hashAccount;

                            /* The total to be credited. */
                            uint64_t  nCredit;
                            txDebit.ssOperation >> nCredit;

                            /* Output the json information. */
                            op["OP"]      = "CREDIT";
                            op["txid"]    = hashTx.ToString();
                            op["proof"]   = hashProof.ToString();
                            op["account"] = hashAccount.ToString();
                            op["amount"]  = nCredit;

                            break;
                        }


                        /* Coinbase operation. Creates an account if none exists. */
                        case TAO::Operation::OP::COINBASE:
                        {

                            /* The total to be credited. */
                            uint64_t  nCredit;
                            ssOperation >> nCredit;

                            /* The extra nNonce available in script. */
                            uint64_t  nExtraNonce;
                            ssOperation >> nExtraNonce;

                            /* Output the json information. */
                            op["OP"]     = "COINBASE";
                            op["nonce"]  = nExtraNonce;
                            op["amount"] = nCredit;

                            break;
                        }


                        /* Coinstake operation. Requires an account. */
                        case TAO::Operation::OP::TRUST:
                        {

                            /* The account that is being staked. */
                            uint256_t hashAccount;
                            ssOperation >> hashAccount;

                            /* The previous trust block. */
                            uint1024_t hashLastTrust;
                            ssOperation >> hashLastTrust;

                            /* Previous trust sequence number. */
                            uint32_t nSequence;
                            ssOperation >> nSequence;

                            /* The trust calculated. */
                            uint64_t nTrust;
                            ssOperation >> nTrust;

                            /* The total to be staked. */
                            uint64_t  nStake;
                            ssOperation >> nStake;

                            /* Output the json information. */
                            op["OP"]       = "TRUST";
                            op["account"]  = hashAccount.ToString();
                            op["last"]     = hashLastTrust.ToString();
                            op["sequence"] = nSequence;
                            op["trust"]    = nTrust;
                            op["amount"]   = nStake;

                            break;
                        }


                        /* Authorize a transaction to happen with a temporal proof. */
                        case TAO::Operation::OP::AUTHORIZE:
                        {
                            /* The transaction that you are authorizing. */
                            uint512_t hashTx;
                            ssOperation >> hashTx;

                            /* The proof you are using that you have rights. */
                            uint256_t hashProof;
                            ssOperation >> hashProof;

                            /* Output the json information. */
                            op["OP"]    = "AUTHORIZE";
                            op["txid"]  = hashTx.ToString();
                            op["proof"] = hashProof.ToString();

                            break;
                        }
                    }
                    /* Add this operation to the return json */
                    ret.push_back(op);
                }
            }
            catch(const std::runtime_error& e)
            {

            }

            return ret;
        }


        /* Converts an Object Register to formattted JSON */
        json::json ObjectRegisterToJSON(const json::json& params, const TAO::Register::Object& object, const uint256_t& hashRegister)
        {
            /* Declare the return JSON object */
            json::json ret;

            /* Look up the object name based on the Name records in thecaller's sig chain */
            std::string strName = GetObjectRegisterName(hashRegister, users->GetCallersGenesis(params), object.hashOwner);

            /* Add the name to the response if one is found. */
            if( !strName.empty())
                ret["name"] = strName;

            /* Now build the response based on the register type */
            if(object.nType == TAO::Register::REGISTER::APPEND
            || object.nType == TAO::Register::REGISTER::RAW)
            {
                
                /* raw state assets only have one data member containing the raw hex-encoded data*/
                std::string data;
                object >> data;

                ret["address"]    = hashRegister.ToString();
                ret["timestamp"]  = object.nTimestamp;
                ret["owner"]      = object.hashOwner.ToString();
                ret["data"] = data;
            }
            else if(object.nType == TAO::Register::REGISTER::OBJECT)
            {
                /* Get the object standard. */
                uint8_t nStandard = object.Standard();

                if(nStandard == TAO::Register::OBJECTS::ACCOUNT)
                {
                    ret["address"]    = hashRegister.ToString();

                    std::string strTokenName = GetTokenNameForAccount(users->GetCallersGenesis(params), object);
                    if(!strTokenName.empty())
                        ret["token_name"] = strTokenName;
                    
                    ret["token_address"] = object.get<uint256_t>("token_address").GetHex();

                    /* Handle the digits.  The digits represent the maximum number of decimal places supported by the token
                    Therefore, to convert the internal value to a floating point value we need to reduce the internal value
                    by 10^digits  */
                    uint64_t nDigits = GetTokenOrAccountDigits(object);

                    ret["balance"]    = (double)object.get<uint64_t>("balance") / pow(10, nDigits);

                }
                else if(nStandard == TAO::Register::OBJECTS::TRUST)
                {
                    ret["address"]    = hashRegister.ToString();
                    ret["token_name"] = "NXS";
                    ret["token_address"] = object.get<uint256_t>("token_address").GetHex();

                    /* Handle the digits.  The digits represent the maximum number of decimal places supported by the token
                    Therefore, to convert the internal value to a floating point value we need to reduce the internal value
                    by 10^digits  */
                    uint64_t nDigits = GetTokenOrAccountDigits(object);

                    ret["balance"]    = (double)object.get<uint64_t>("balance") / pow(10, nDigits);

                    /* General trust account output same as ACCOUNT. Leave off stake-related values */
                    //ret["trust"]    = (double)object.get<uint64_t>("trust") / pow(10, nDigits);
                    //ret["stake"]    = (double)object.get<uint64_t>("stake") / pow(10, nDigits);

                }
                else if(nStandard == TAO::Register::OBJECTS::TOKEN)
                {

                    /* Handle the digits.  The digits represent the maximum number of decimal places supported by the token
                    Therefore, to convert the internal value to a floating point value we need to reduce the internal value
                    by 10^digits  */
                    uint64_t nDigits = GetTokenOrAccountDigits(object);

                    ret["address"]          = hashRegister.ToString();
                    ret["balance"]          = (double) object.get<uint64_t>("balance") / pow(10, nDigits);
                    ret["maxsupply"]        = (double) object.get<uint64_t>("supply") / pow(10, nDigits);
                    ret["currentsupply"]    = (double) (object.get<uint64_t>("supply")
                                            - object.get<uint64_t>("balance")) / pow(10, nDigits);
                    ret["digits"]           = nDigits;
                }
                /* Must be a user-definable object register (asset) */
                else
                { 
                    ret["address"]    = hashRegister.ToString();
                    ret["timestamp"]  = object.nTimestamp;
                    ret["owner"]      = object.hashOwner.ToString();

                    /* Get List of field names in this asset object */
                    std::vector<std::string> vFieldNames = object.GetFieldNames();

                    /* Declare type and data variables for unpacking the Object fields */
                    uint8_t nType;
                    uint8_t nUint8;
                    uint16_t nUint16;
                    uint32_t nUint32;
                    uint64_t nUint64;
                    uint256_t nUint256;
                    uint512_t nUint512;
                    uint1024_t nUint1024;
                    std::string strValue;
                    std::vector<uint8_t> vchBytes;

                    for(const auto& strFieldName : vFieldNames)
                    {       
                        /* First get the type*/
                        object.Type(strFieldName, nType);

                        if(nType == TAO::Register::TYPES::UINT8_T)
                        {
                            object.Read<uint8_t>(strFieldName, nUint8);
                            ret[strFieldName] = nUint8;
                        }
                        else if(nType == TAO::Register::TYPES::UINT16_T)
                        {
                            object.Read<uint16_t>(strFieldName, nUint16);
                            ret[strFieldName] = nUint16;
                        }
                        else if(nType == TAO::Register::TYPES::UINT32_T)
                        {
                            object.Read<uint32_t>(strFieldName, nUint32);
                            ret[strFieldName] = nUint32;
                        }
                        else if(nType == TAO::Register::TYPES::UINT64_T)
                        {
                            object.Read<uint64_t>(strFieldName, nUint64);
                            ret[strFieldName] = nUint64;
                        }
                        else if(nType == TAO::Register::TYPES::UINT256_T)
                        {
                            object.Read<uint256_t>(strFieldName, nUint256);
                            ret[strFieldName] = nUint256.GetHex();
                        }
                        else if(nType == TAO::Register::TYPES::UINT512_T)
                        {
                            object.Read<uint512_t>(strFieldName, nUint512);
                            ret[strFieldName] = nUint512.GetHex();
                        }
                        else if(nType == TAO::Register::TYPES::UINT1024_T)
                        {
                            object.Read<uint1024_t>(strFieldName, nUint1024);
                            ret[strFieldName] = nUint1024.GetHex();
                        }
                        else if(nType == TAO::Register::TYPES::STRING )
                        {
                            object.Read<std::string>(strFieldName, strValue);

                            /* Remove trailing nulls from the data, which are added for padding to maxlength on mutable fields */
                            ret[strFieldName] = strValue.substr(0, strValue.find_last_not_of('\0') + 1);
                        }
                        else if(nType == TAO::Register::TYPES::BYTES)
                        {
                            object.Read<std::vector<uint8_t>>(strFieldName, vchBytes);

                            /* Remove trailing nulls from the data, which are added for padding to maxlength on mutable fields */
                            vchBytes.erase(std::find(vchBytes.begin(), vchBytes.end(), '\0'), vchBytes.end());

                            ret[strFieldName] = encoding::EncodeBase64(&vchBytes[0], vchBytes.size()) ;
                        }

                    }
                }
            }
            return ret;
        }
    }
}
