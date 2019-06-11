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

#include <TAO/Operation/include/enum.h>

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

            /* Main block hash. */
            result["hash"] = block.GetHash().GetHex();

            /* The hash that was relevant for Proof of Stake or Proof of Work (depending on block version) */
            result["proofhash"]  =
                                    block.nVersion < 5 ? block.GetHash().GetHex() :
                                    ((block.nChannel == 0) ? block.StakeHash().GetHex() : block.ProofHash().GetHex());

            /* Body of the block with relevant data. */
            result["size"]       = (uint32_t)::GetSerializeSize(block, SER_NETWORK, LLP::PROTOCOL_VERSION);
            result["height"]     = (uint32_t)block.nHeight;
            result["channel"]    = (uint32_t)block.nChannel;
            result["version"]    = (uint32_t)block.nVersion;
            result["merkleroot"] = block.hashMerkleRoot.GetHex();
            result["time"]       = convert::DateTimeStrFormat(block.GetBlockTime());
            result["nonce"]      = (uint64_t)block.nNonce;
            result["bits"]       = HexBits(block.nBits);
            result["difficulty"] = TAO::Ledger::GetDifficulty(block.nBits, block.nChannel);
            result["mint"]       = Legacy::SatoshisToAmount(block.nMint);

            /* Add previous block if not null. */
            if(block.hashPrevBlock != 0)
                result["previousblockhash"] = block.hashPrevBlock.GetHex();

            /* Add next hash if not null. */
            if(block.hashNextBlock != 0)
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
                        if(LLD::Ledger->ReadTx(vtx.second, tx))
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
                        if(LLD::Legacy->ReadTx(vtx.second, tx))
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
        json::json TransactionToJSON(const TAO::Ledger::Transaction& tx, const TAO::Ledger::BlockState& block, uint32_t nTransactionVerbosity)
        {
            /* Declare JSON object to return */
            json::json txdata;

            /* Always add the transaction hash */
            txdata["txid"] = tx.GetHash().GetHex();

            /* Always add the contracts if level 1 and up */
            if(nTransactionVerbosity >= 1)
                txdata["contracts"] = ContractsToJSON(tx);

            /* Basic TX info for level 2 and up */
            if(nTransactionVerbosity >= 2)
            {
                /* Build base transaction data. */
                txdata["type"]      = tx.GetTxTypeString();
                txdata["version"]   = tx.nVersion;
                txdata["sequence"]  = tx.nSequence;
                txdata["timestamp"] = tx.nTimestamp;
                txdata["confirmations"] = block.IsNull() ? 0 : TAO::Ledger::ChainState::nBestHeight.load() - block.nHeight + 1;

                /* Genesis and hashes are verbose 3 and up. */
                if(nTransactionVerbosity >= 3)
                {
                    /* More sigchain level details. */
                    txdata["genesis"]   = tx.hashGenesis.ToString();
                    txdata["nexthash"]  = tx.hashNext.ToString();
                    txdata["prevhash"]  = tx.hashPrevTx.ToString();

                    /* The cryptographic data. */
                    txdata["pubkey"]    = HexStr(tx.vchPubKey.begin(), tx.vchPubKey.end());
                    txdata["signature"] = HexStr(tx.vchSig.begin(),    tx.vchSig.end());
                }
            }

            return txdata;
        }

        /* Converts the transaction to formatted JSON */
        json::json TransactionToJSON(const Legacy::Transaction& tx, const TAO::Ledger::BlockState& block, uint32_t nTransactionVerbosity)
        {
            /* Declare JSON object to return */
            json::json txdata;

            /* Always add the hash */
            txdata["txid"] = tx.GetHash().GetHex();

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
                        if(!LLD::Legacy->ReadTx(txin.prevout.hash, txprev))
                            throw APIException(-5, "Invalid transaction id");

                        /* Extract the Nexus Address. */
                        Legacy::NexusAddress address;
                        if(!Legacy::ExtractAddress(txprev.vout[txin.prevout.n].scriptPubKey, address))
                            throw APIException(-5, "Unable to Extract Input Address");

                        /* Push new input to return value. */
                        inputs.push_back(debug::safe_printstr("%s:%f",
                                address.ToString().c_str(), (double) tx.vout[txin.prevout.n].nValue / TAO::Ledger::NXS_COIN));
                    }
                    txdata["inputs"] = inputs;
                }

                /* Declare the output JSON array */
                json::json outputs = json::json::array();

                /* Iterate through each output */
                for(const Legacy::TxOut& txout : tx.vout)
                {
                    /* Extract the Nexus Address. */
                    Legacy::NexusAddress address;
                    if(!Legacy::ExtractAddress(txout.scriptPubKey, address))
                        throw APIException(-5, "Unable to Extract Input Address");

                    /* Add to the outputs. */
                    outputs.push_back(debug::safe_printstr("%s:%f", address.ToString().c_str(), (double) txout.nValue / Legacy::COIN));
                }
                txdata["outputs"] = outputs;
            }

            return txdata;
        }


        /* Converts a transaction object into a formatted JSON list of contracts bound to the transaction. */
        json::json ContractsToJSON(const TAO::Ledger::Transaction &tx)
        {
            json::json ret = json::json::array();

            /* Add a contract to the list of contracts. */
            uint32_t nContracts = tx.Size();
            for(uint32_t nContract = 0; nContract < nContracts; ++nContract)
                ret.push_back(ContractToJSON(tx[nContract]));

            /* Return the list of contracts. */
            return ret;
        }


        /* Converts a serialized operation stream to formattted JSON */
        json::json ContractToJSON(const TAO::Operation::Contract& contract)
        {
            /* Declare the return JSON object*/
            json::json ret;

            /* Start the stream at the beginning. */
            contract.Reset();

            /* Make sure no exceptions are thrown. */
            try
            {
                /* Get the contract operations. */
                uint8_t OPERATION = 0;
                contract >> OPERATION;

                /* Check the current opcode. */
                switch(OPERATION)
                {
                    /* Record a new state to the register. */
                    case TAO::Operation::OP::WRITE:
                    {
                        /* Get the Address of the Register. */
                        uint256_t hashAddress = 0;
                        contract >> hashAddress;

                        /* Deserialize the register from contract. */
                        std::vector<uint8_t> vchData;
                        contract >> vchData;

                        /* Output the json information. */
                        ret["OP"]      = "WRITE";
                        ret["address"] = hashAddress.ToString();
                        ret["data"]    = HexStr(vchData.begin(), vchData.end());

                        break;
                    }


                    /* Append a new state to the register. */
                    case TAO::Operation::OP::APPEND:
                    {
                        /* Get the Address of the Register. */
                        uint256_t hashAddress = 0;
                        contract >> hashAddress;

                        /* Deserialize the register from contract. */
                        std::vector<uint8_t> vchData;
                        contract >> vchData;

                        /* Output the json information. */
                        ret["OP"]      = "APPEND";
                        ret["address"] = hashAddress.ToString();
                        ret["data"]    = HexStr(vchData.begin(), vchData.end());

                        break;
                    }


                    /* Create a new register. */
                    case TAO::Operation::OP::CREATE:
                    {
                        /* Extract the address from the contract. */
                        uint256_t hashAddress = 0;
                        contract >> hashAddress;

                        /* Extract the register type from contract. */
                        uint8_t nType = 0;
                        contract >> nType;

                        /* Extract the register data from the contract. */
                        std::vector<uint8_t> vchData;
                        contract >> vchData;

                        /* Output the json information. */
                        ret["OP"]      = "REGISTER";
                        ret["address"] = hashAddress.ToString();
                        ret["type"]    = nType;
                        ret["data"]    = HexStr(vchData.begin(), vchData.end());


                        break;
                    }


                    /* Transfer ownership of a register to another signature chain. */
                    case TAO::Operation::OP::TRANSFER:
                    {
                        /* Extract the address from the contract. */
                        uint256_t hashAddress = 0;
                        contract >> hashAddress;

                        /* Read the register transfer recipient. */
                        uint256_t hashTransfer = 0;
                        contract >> hashTransfer;

                        /* Get the flag byte. */
                        uint8_t nType = 0;
                        contract >> nType;

                        /* Output the json information. */
                        ret["OP"]       = "TRANSFER";
                        ret["address"]  = hashAddress.ToString();
                        ret["transfer"] = hashTransfer.ToString();
                        ret["force"]    = (nType == TAO::Operation::TRANSFER::FORCE ? "True" : "False");

                        break;
                    }


                    /* Claim ownership of a register to another signature chain. */
                    case TAO::Operation::OP::CLAIM:
                    {
                        /* Extract the transaction from contract. */
                        uint512_t hashTx = 0;
                        contract >> hashTx;

                        /* Extract the contract-id. */
                        uint32_t nContract = 0;
                        contract >> nContract;

                        /* Extract the address from the contract. */
                        uint256_t hashAddress = 0;
                        contract >> hashAddress;

                        /* Output the json information. */
                        ret["OP"]         = "CLAIM";
                        ret["txid"]       = hashTx.ToString();
                        ret["output"]     = nContract;
                        ret["address"]    = hashAddress.ToString();

                        break;
                    }


                    /* Coinbase operation. Creates an account if none exists. */
                    case TAO::Operation::OP::COINBASE:
                    {
                        /* Get the genesis. */
                        uint256_t hashGenesis = 0;
                        contract >> hashGenesis;

                        /* The total to be credited. */
                        uint64_t nCredit = 0;
                        contract >> nCredit;

                        /* The extra nNonce available in script. */
                        uint64_t nExtraNonce = 0;
                        contract >> nExtraNonce;

                        /* Output the json information. */
                        ret["OP"]      = "COINBASE";
                        ret["genesis"] = hashGenesis.ToString();
                        ret["nonce"]   = nExtraNonce;
                        ret["amount"]  = nCredit;

                        break;
                    }


                    /* Trust operation. Builds trust and generates reward. */
                    case TAO::Operation::OP::TRUST:
                    {
                        /* Get the genesis. */
                        uint256_t hashLastTrust = 0;
                        contract >> hashLastTrust;

                        /* The total trust score. */
                        uint64_t nScore = 0;
                        contract >> nScore;

                        /* The total trust reward. */
                        uint64_t nReward = 0;
                        contract >> nReward;

                        /* Output the json information. */
                        ret["OP"]      = "TRUST";
                        ret["last"]    = hashLastTrust.ToString();
                        ret["score"]   = nScore;
                        ret["amount"]  = nReward;

                        break;
                    }


                    /* Genesis operation. Begins trust and stakes. */
                    case TAO::Operation::OP::GENESIS:
                    {
                        /* Get the genesis. */
                        uint256_t hashAddress = 0;
                        contract >> hashAddress;

                        /* The total trust reward. */
                        uint64_t nReward = 0;
                        contract >> nReward;

                        /* Output the json information. */
                        ret["OP"]        = "GENESIS";
                        ret["address"]   = hashAddress.ToString();
                        ret["amount"]    = nReward;

                        break;
                    }


                    /* Stake operation. Move amount from trust account balance to stake. */
                    case TAO::Operation::OP::STAKE:
                    {
                        /* Amount to of funds to move. */
                        uint64_t nAmount;
                        contract >> nAmount;

                        /* Output the json information. */
                        ret["OP"]      = "STAKE";
                        ret["amount"]  = nAmount;

                        break;
                    }


                    /* Unstake operation. Move amount from trust account stake to balance (with trust penalty). */
                    case TAO::Operation::OP::UNSTAKE:
                    {
                        /* Amount of funds to move. */
                        uint64_t nAmount;
                        contract >> nAmount;

                        /* Trust score penalty from unstake. */
                        uint64_t nTrustPenalty;
                        contract >> nTrustPenalty;

                        /* Output the json information. */
                        ret["OP"]      = "UNSTAKE";
                        ret["penalty"] = nTrustPenalty;
                        ret["amount"]  = nAmount;

                        break;
                    }


                    /* Debit tokens from an account you own. */
                    case TAO::Operation::OP::DEBIT:
                    {
                        /* Get the register address. */
                        uint256_t hashFrom = 0;
                        contract >> hashFrom;

                        /* Get the transfer address. */
                        uint256_t hashTo = 0;
                        contract >> hashTo;

                        /* Get the transfer amount. */
                        uint64_t  nAmount = 0;
                        contract >> nAmount;

                        /* Output the json information. */
                        ret["OP"]       = "DEBIT";
                        ret["from"]     = hashFrom.ToString();
                        ret["to"]       = hashTo.ToString();
                        ret["amount"]   = nAmount;

                        break;
                    }


                    /* Credit tokens to an account you own. */
                    case TAO::Operation::OP::CREDIT:
                    {
                        /* Extract the transaction from contract. */
                        uint512_t hashTx = 0;
                        contract >> hashTx;

                        /* Extract the contract-id. */
                        uint32_t nContract = 0;
                        contract >> nContract;

                        /* Get the transfer address. */
                        uint256_t hashAddress = 0;
                        contract >> hashAddress;

                        /* Get the transfer address. */
                        uint256_t hashProof = 0;
                        contract >> hashProof;

                        /* Get the transfer amount. */
                        uint64_t  nCredit = 0;
                        contract >> nCredit;

                        /* Output the json information. */
                        ret["OP"]      = "CREDIT";
                        ret["txid"]    = hashTx.ToString();
                        ret["output"]  = nContract;
                        ret["proof"]   = hashProof.ToString();
                        ret["account"] = hashAddress.ToString();
                        ret["amount"]  = nCredit;

                        break;
                    }

                    /* Authorize a transaction to happen with a temporal proof. */
                    case TAO::Operation::OP::AUTHORIZE:
                    {
                        /* The transaction that you are authorizing. */
                        uint512_t hashTx = 0;
                        contract >> hashTx;

                        /* The proof you are using that you have rights. */
                        uint256_t hashProof = 0;
                        contract >> hashProof;

                        /* Output the json information. */
                        ret["OP"]    = "AUTHORIZE";
                        ret["txid"]  = hashTx.ToString();
                        ret["proof"] = hashProof.ToString();

                        break;
                    }
                }
            }
            catch(const std::exception& e)
            {

            }

            return ret;
        }


        /* Converts an Object Register to formattted JSON */
        json::json ObjectToJSON(const json::json& params, const TAO::Register::Object& object, const uint256_t& hashRegister)
        {
            /* Declare the return JSON object */
            json::json ret;

            /* Look up the object name based on the Name records in thecaller's sig chain */
            std::string strName = GetRegisterName(hashRegister, users->GetCallersGenesis(params), object.hashOwner);

            /* Add the name to the response if one is found. */
            if(!strName.empty())
                ret["name"] = strName;

            /* Now build the response based on the register type */
            if(object.nType == TAO::Register::REGISTER::APPEND
            || object.nType == TAO::Register::REGISTER::RAW)
            {
                /* Raw state assets only have one data member containing the raw hex-encoded data*/
                std::string data;
                object >> data;

                ret["address"]    = hashRegister.ToString();
                ret["created"]    = object.nCreated;
                ret["modified"]   = object.nModified;
                ret["owner"]      = object.hashOwner.ToString();
                ret["data"]       = data;
            }
            else if(object.nType == TAO::Register::REGISTER::OBJECT)
            {
                /* Get the object standard. */
                uint8_t nStandard = object.Standard();
                switch(nStandard)
                {

                    /* Handle for a regular account. */
                    case TAO::Register::OBJECTS::ACCOUNT:
                    {
                        ret["address"]    = hashRegister.ToString();

                        /* Get the token names. */
                        std::string strTokenName = GetTokenNameForAccount(users->GetCallersGenesis(params), object);
                        if(!strTokenName.empty())
                            ret["token_name"] = strTokenName;

                        /* Set the value to the token contract address. */
                        ret["token"] = object.get<uint256_t>("token").GetHex();

                        /* Handle digit conversion. */
                        uint64_t nDigits = GetDigits(object);

                        ret["balance"]    = (double)object.get<uint64_t>("balance") / pow(10, nDigits);

                        break;
                    }


                    /* Handle for a trust account. */
                    case TAO::Register::OBJECTS::TRUST:
                    {
                        ret["address"]    = hashRegister.ToString();
                        ret["token_name"] = "NXS";
                        ret["token"] = object.get<uint256_t>("token").GetHex();

                        /* Handle digit conversion. */
                        uint64_t nDigits = GetDigits(object);

                        ret["balance"]    = (double)object.get<uint64_t>("balance") / pow(10, nDigits);

                        /* General trust account output same as ACCOUNT. Leave off stake-related values */
                        //ret["trust"]    = (double)object.get<uint64_t>("trust") / pow(10, nDigits);
                        //ret["stake"]    = (double)object.get<uint64_t>("stake") / pow(10, nDigits);

                        break;
                    }


                    /* Handle for a token contract. */
                    case TAO::Register::OBJECTS::TOKEN:
                    {
                        /* Handle the digits.  The digits represent the maximum number of decimal places supported by the token
                        Therefore, to convert the internal value to a floating point value we need to reduce the internal value
                        by 10^digits  */
                        uint64_t nDigits = GetDigits(object);

                        ret["address"]          = hashRegister.ToString();
                        ret["balance"]          = (double) object.get<uint64_t>("balance") / pow(10, nDigits);
                        ret["maxsupply"]        = (double) object.get<uint64_t>("supply") / pow(10, nDigits);
                        ret["currentsupply"]    = (double) (object.get<uint64_t>("supply")
                                                - object.get<uint64_t>("balance")) / pow(10, nDigits);
                        ret["digits"]           = nDigits;

                        break;
                    }

                    /* Handle for all nonstandard object registers. */
                    default:
                    {
                        ret["address"]    = hashRegister.ToString();
                        ret["created"]    = object.nCreated;
                        ret["modified"]   = object.nModified;
                        ret["owner"]      = object.hashOwner.ToString();

                        /* Get List of field names in this asset object */
                        std::vector<std::string> vFieldNames = object.GetFieldNames();

                        /* Declare type and data variables for unpacking the Object fields */
                        for(const auto& strName : vFieldNames)
                        {
                            /* First get the type */
                            uint8_t nType = 0;
                            object.Type(strName, nType);

                            /* Switch based on type. */
                            switch(nType)
                            {
                                /* Check for uint8_t type. */
                                case TAO::Register::TYPES::UINT8_T:
                                {
                                    /* Set the return value from object register data. */
                                    ret[strName] = object.get<uint8_t>(strName);

                                    break;
                                }

                                /* Check for uint16_t type. */
                                case TAO::Register::TYPES::UINT16_T:
                                {
                                    /* Set the return value from object register data. */
                                    ret[strName] = object.get<uint16_t>(strName);

                                    break;
                                }

                                /* Check for uint32_t type. */
                                case TAO::Register::TYPES::UINT32_T:
                                {
                                    /* Set the return value from object register data. */
                                    ret[strName] = object.get<uint32_t>(strName);

                                    break;
                                }

                                /* Check for uint64_t type. */
                                case TAO::Register::TYPES::UINT64_T:
                                {
                                    /* Set the return value from object register data. */
                                    ret[strName] = object.get<uint8_t>(strName);

                                    break;
                                }

                                /* Check for uint256_t type. */
                                case TAO::Register::TYPES::UINT256_T:
                                {
                                    /* Set the return value from object register data. */
                                    ret[strName] = object.get<uint256_t>(strName).GetHex();

                                    break;
                                }

                                /* Check for uint512_t type. */
                                case TAO::Register::TYPES::UINT512_T:
                                {
                                    /* Set the return value from object register data. */
                                    ret[strName] = object.get<uint512_t>(strName).GetHex();

                                    break;
                                }

                                /* Check for uint1024_t type. */
                                case TAO::Register::TYPES::UINT1024_T:
                                {
                                    /* Set the return value from object register data. */
                                    ret[strName] = object.get<uint1024_t>(strName).GetHex();

                                    break;
                                }

                                /* Check for string type. */
                                case TAO::Register::TYPES::STRING:
                                {
                                    /* Set the return value from object register data. */
                                    std::string strRet = object.get<std::string>(strName);

                                    /* Remove trailing nulls from the data, which are padding to maxlength on mutable fields */
                                    ret[strName] = strRet.substr(0, strRet.find_last_not_of('\0') + 1);

                                    break;
                                }

                                /* Check for bytes type. */
                                case TAO::Register::TYPES::BYTES:
                                {
                                    /* Set the return value from object register data. */
                                    std::vector<uint8_t> vRet = object.get<std::vector<uint8_t>>(strName);

                                    /* Remove trailing nulls from the data, which are padding to maxlength on mutable fields */
                                    vRet.erase(std::find(vRet.begin(), vRet.end(), '\0'), vRet.end());

                                    /* Add to return value in base64 encoding. */
                                    ret[strName] = encoding::EncodeBase64(&vRet[0], vRet.size()) ;

                                    break;
                                }
                            }
                        }

                        break;
                    }
                }
            }

            return ret;
        }
    }
}
