/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/include/json.h>
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
#include <TAO/Operation/include/create.h>

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
        json::json BlockToJSON(const TAO::Ledger::BlockState& block, uint32_t nVerbosity)
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
            if(nVerbosity > 0)
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
                            json::json ret = TransactionToJSON(0, tx, block, nVerbosity);

                            txinfo.push_back(ret);
                        }
                    }
                    else if(vtx.first == TAO::Ledger::TYPE::LEGACY_TX)
                    {
                        /* Get the legacy transaction from the database. */
                        Legacy::Transaction tx;
                        if(LLD::Legacy->ReadTx(vtx.second, tx))
                        {
                            /* add the transaction JSON.  */
                            json::json ret = TransactionToJSON(tx, block, nVerbosity);

                            txinfo.push_back(ret);
                        }
                    }
                }
                /* Add the transaction data to the response */
                result["tx"] = txinfo;
            }

            return result;
        }

        /* Converts the transaction to formatted JSON */
        json::json TransactionToJSON(const uint256_t& hashCaller, const TAO::Ledger::Transaction& tx, const TAO::Ledger::BlockState& block, uint32_t nVerbosity)
        {
            /* Declare JSON object to return */
            json::json ret;

            /* Always add the transaction hash */
            ret["txid"] = tx.GetHash().GetHex();

            /* Basic TX info for level 2 and up */
            if(nVerbosity >= 2)
            {
                /* Build base transaction data. */
                ret["type"]      = tx.TypeString();
                ret["version"]   = tx.nVersion;
                ret["sequence"]  = tx.nSequence;
                ret["timestamp"] = tx.nTimestamp;
                ret["confirmations"] = block.IsNull() ? 0 : TAO::Ledger::ChainState::nBestHeight.load() - block.nHeight + 1;

                /* Genesis and hashes are verbose 3 and up. */
                if(nVerbosity >= 3)
                {
                    /* More sigchain level details. */
                    ret["genesis"]   = tx.hashGenesis.ToString();
                    ret["nexthash"]  = tx.hashNext.ToString();
                    ret["prevhash"]  = tx.hashPrevTx.ToString();

                    /* The cryptographic data. */
                    ret["pubkey"]    = HexStr(tx.vchPubKey.begin(), tx.vchPubKey.end());
                    ret["signature"] = HexStr(tx.vchSig.begin(),    tx.vchSig.end());
                }
            }

            /* Always add the contracts if level 2 and up */
            if(nVerbosity >= 2)
                ret["contracts"] = ContractsToJSON(hashCaller, tx, nVerbosity);

            return ret;
        }

        /* Converts the transaction to formatted JSON */
        json::json TransactionToJSON(const Legacy::Transaction& tx, const TAO::Ledger::BlockState& block, uint32_t nVerbosity)
        {
            /* Declare JSON object to return */
            json::json ret;

            /* Always add the hash */
            ret["txid"] = tx.GetHash().GetHex();

            /* Basic TX info for level 1 and up */
            if(nVerbosity > 0)
            {
                ret["type"] = tx.TypeString();
                ret["timestamp"] = tx.nTime;
                ret["amount"] = Legacy::SatoshisToAmount(tx.GetValueOut());
                ret["confirmations"] = block.IsNull() ? 0 : TAO::Ledger::ChainState::nBestHeight.load() - block.nHeight + 1;

                /* Don't add inputs for coinbase or coinstake transactions */
                if(!tx.IsCoinBase() && !tx.IsCoinStake())
                {
                    /* Declare the inputs JSON array */
                    json::json inputs = json::json::array();

                    /* Iterate through each input */
                    for(const Legacy::TxIn& txin : tx.vin)
                    {
                        json::json input;
                        /* Read the previous transaction in order to get its outputs */
                        Legacy::Transaction txprev;
                        if(!LLD::Legacy->ReadTx(txin.prevout.hash, txprev))
                            throw APIException(-7, "Invalid transaction id");

                        /* Extract the Nexus Address. */
                        Legacy::NexusAddress address;
                        uint256_t hashRegister;
                        if(Legacy::ExtractAddress(txprev.vout[txin.prevout.n].scriptPubKey, address))
                            input["address"] = address.ToString();
                        else if(Legacy::ExtractRegister(txprev.vout[txin.prevout.n].scriptPubKey, hashRegister))
                            input["address"] = hashRegister.GetHex();
                        else
                            throw APIException(-8, "Unable to Extract Input Address");

                        input["amount"] = (double) tx.vout[txin.prevout.n].nValue / TAO::Ledger::NXS_COIN;

                        inputs.push_back(input);
                        
                    }
                    ret["inputs"] = inputs;
                }

                /* Declare the output JSON array */
                json::json outputs = json::json::array();

                /* Iterate through each output */
                for(const Legacy::TxOut& txout : tx.vout)
                {
                    json::json output;
                    /* Extract the Nexus Address. */
                    Legacy::NexusAddress address;
                    uint256_t hashRegister;
                    if(Legacy::ExtractAddress(txout.scriptPubKey, address))
                        output["address"] = address.ToString();
                    else if(Legacy::ExtractRegister(txout.scriptPubKey, hashRegister))
                        output["address"] = hashRegister.GetHex();
                    else
                        throw APIException(-8, "Unable to Extract Output Address");

                    output["amount"] = (double) txout.nValue / TAO::Ledger::NXS_COIN;

                    outputs.push_back(output);

                }
                ret["outputs"] = outputs;
            }

            return ret;
        }


        /* Converts a transaction object into a formatted JSON list of contracts bound to the transaction. */
        json::json ContractsToJSON(const uint256_t& hashCaller, const TAO::Ledger::Transaction &tx, uint32_t nVerbosity)
        {
            /* Declare the return JSON object*/
            json::json ret = json::json::array();

            /* Add a contract to the list of contracts. */
            uint32_t nContracts = tx.Size();
            for(uint32_t nContract = 0; nContract < nContracts; ++nContract)
                ret.push_back(ContractToJSON(hashCaller, tx[nContract], nVerbosity));

            return ret;
        }


        /* Converts a serialized operation stream to formattted JSON */
        json::json ContractToJSON(const uint256_t& hashCaller, const TAO::Operation::Contract& contract, uint32_t nVerbosity)
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
                        ret["OP"]      = "CREATE";
                        ret["address"] = hashAddress.ToString();
                        ret["type"]    = RegisterType(nType);
                        
                        /* If this is a register object then decode the object type */
                        if(nType == TAO::Register::REGISTER::OBJECT)
                        {
                            /* Create the register object. */
                            TAO::Register::Object state;
                            state.nVersion   = 1;
                            state.nType      = nType;
                            state.hashOwner  = contract.Caller();

                            /* Calculate the new operation. */
                            if(!TAO::Operation::Create::Execute(state, vchData, contract.Timestamp()))
                                throw APIException(-110, "Contract execution failed");

                            /* parse object so that the data fields can be accessed */
                            if(!state.Parse())
                                throw APIException(-36, "Failed to parse object register");

                            ret["object_type"] = ObjectType(state.Standard());
                        }

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
                        ret["destination"] = hashTransfer.ToString();

                        /* If this is a register object then decode the object type */
                        if(nType == TAO::Register::REGISTER::OBJECT)
                        {
                            TAO::Register::Object object;
                            if(LLD::Register->ReadState(hashAddress, object, TAO::Ledger::FLAGS::MEMPOOL))
                            {
                                /* parse object so that the data fields can be accessed */
                                if(!object.Parse())
                                    throw APIException(-36, "Failed to parse object register");

                                ret["object_type"] = ObjectType(object.Standard());
                            }
                        }

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
                        ret["amount"]  = (double) nCredit / TAO::Ledger::NXS_COIN;;

                        if(nVerbosity > 0)
                        {
                            uint32_t nConfirms = 0;
                            LLD::Ledger->ReadConfirmations(contract.Hash(), nConfirms);
                            ret["confirms"] = nConfirms;
                        }

                        break;
                    }


                    /* Trust operation. Builds trust and generates reward. */
                    case TAO::Operation::OP::TRUST:
                    {
                        /* Get the genesis. */
                        uint512_t hashLastTrust = 0;
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
                        ret["amount"]  = (double) nReward / TAO::Ledger::NXS_COIN;;

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
                        ret["amount"]    = (double) nReward / TAO::Ledger::NXS_COIN;;

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
                        ret["amount"]  = (double) nAmount / TAO::Ledger::NXS_COIN;;

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
                        ret["amount"]  = (double) nAmount / TAO::Ledger::NXS_COIN;

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

                        /* Get the reference */
                        uint64_t nReference = 0;
                        contract >> nReference;

                        /* Output the json information. */
                        ret["OP"]       = "DEBIT";
                        ret["from"]     = hashFrom.ToString();

                        /* Resolve the name of the token/account that the debit is from */
                        std::string strFrom = Names::ResolveName(hashCaller, hashFrom);
                        if(!strFrom.empty())
                            ret["from_name"] = strFrom;

                        ret["to"]       = hashTo.ToString();

                        /* Resolve the name of the token/account/register that the debit is to */
                        std::string strTo = Names::ResolveName(hashCaller, hashTo);
                        if(!strTo.empty())
                            ret["to_name"] = strTo;

                        /* Get the token/account we are debiting from so that we can output the token address / name. */
                        TAO::Register::Object object;
                        if(!LLD::Register->ReadState(hashFrom, object))
                            throw APIException(-13, "Account not found");

                        /* Parse the object register. */
                        if(!object.Parse())
                            throw APIException(-14, "Object failed to parse");

                        /* Add the amount to the response */
                        ret["amount"]  = (double) nAmount / pow(10, GetDigits(object));

                        /* Add the reference to the response */
                        ret["reference"] = nReference;

                        /* Get the object standard. */
                        uint8_t nStandard = object.Standard();

                        /* Check the object standard. */
                        if(nStandard != TAO::Register::OBJECTS::ACCOUNT
                        && nStandard != TAO::Register::OBJECTS::TRUST
                        && nStandard != TAO::Register::OBJECTS::TOKEN)
                            throw APIException(-15, "Object is not an account or token");

                        /* Get the token address */
                        uint256_t hashToken = object.get<uint256_t>("token");

                        /* Add the token address to the response */
                        ret["token"]   = hashToken.GetHex();

                        /* Resolve the name of the token name */
                        std::string strToken = hashToken != 0 ? Names::ResolveName(hashCaller, hashToken) : "NXS";
                        if(!strToken.empty())
                            ret["token_name"] = strToken;


                        break;
                    }


                    /* Credit tokens to an account you own. */
                    case TAO::Operation::OP::CREDIT:
                    {
                        /* Extract the transaction from contract. */
                        uint512_t hashTx = 0;
                        contract >> hashTx;

                        /* Extract the contract-id. */
                        uint32_t nID = 0;
                        contract >> nID;

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
                        ret["output"]  = nID;
                        ret["proof"]   = hashProof.ToString();
                        ret["account"] = hashAddress.ToString();

                        /* Resolve the name of the account that the credit is to */
                        std::string strAccount = Names::ResolveName(hashCaller, hashAddress);
                        if(!strAccount.empty())
                            ret["account_name"] = strAccount;

                        /* Get the token/account we are crediting to so that we can output the token address / name. */
                        TAO::Register::Object account;
                        if(!LLD::Register->ReadState(hashAddress, account))
                            throw APIException(-13, "Account not found");

                        /* Parse the object register. */
                        if(!account.Parse())
                            throw APIException(-14, "Object failed to parse");

                        /* Add the amount to the response */
                        ret["amount"]  = (double) nCredit / pow(10, GetDigits(account));

                        /* Get the object standard. */
                        uint8_t nStandard = account.Standard();

                        /* Check the object standard. */
                        if(nStandard != TAO::Register::OBJECTS::ACCOUNT
                        && nStandard != TAO::Register::OBJECTS::TRUST
                        && nStandard != TAO::Register::OBJECTS::TOKEN)
                            throw APIException(-15, "Object is not an account or token");

                        /* Get the token address */
                        uint256_t hashToken = account.get<uint256_t>("token");

                        /* Add the token address to the response */
                        ret["token"]   = hashToken.GetHex();

                        /* Resolve the name of the token name */
                        std::string strToken = hashToken != 0 ? Names::ResolveName(hashCaller, hashToken) : "NXS";
                        if(!strToken.empty())
                            ret["token_name"] = strToken;

                        /* The debit transaction being credited */
                        TAO::Ledger::Transaction txDebit;

                        /* Read the corresponding debit transaction to look up the reference field */
                        if(LLD::Ledger->ReadTx(hashTx, txDebit))
                        {
                            /* Get the contract. */
                            const TAO::Operation::Contract& debitContract = txDebit[nID];

                            /* Reset the operation stream position in case it was loaded from mempool and therefore still in previous state */
                            debitContract.Reset();

                            /* Seek to reference. */
                            debitContract.Seek(73);

                            /* The reference */
                            uint64_t nReference = 0;
                            debitContract >> nReference;

                            ret["reference"] = nReference;

                        }

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

                    case TAO::Operation::OP::FEE:
                    {
                        /* Get the address of the account the fee came from. */
                        uint256_t hashAccount = 0;
                        contract >> hashAccount;

                        /* The fee amount. */
                        uint64_t nFee = 0;
                        contract >> nFee;

                        /* Output the json information. */
                        ret["OP"]      = "FEE";
                        ret["account"] = hashAccount.ToString();

                        /* Resolve the name of the account that the credit is to */
                        std::string strAccount = Names::ResolveName(hashCaller, hashAccount);
                        if(!strAccount.empty())
                            ret["account_name"] = strAccount;

                        ret["amount"]  = (double) nFee / TAO::Ledger::NXS_COIN;

                        break;
                    }

                    /* Debit NXS to legacy address. */
                    case TAO::Operation::OP::LEGACY:
                    {
                        /* Get the register address. */
                        uint256_t hashFrom = 0;
                        contract >> hashFrom;


                        /* Get the transfer amount. */
                        uint64_t  nAmount = 0;
                        contract >> nAmount;

                        /* Get the output script. */
                        Legacy::Script script;
                        contract >> script;

                        /* The receiving legacy address */
                        Legacy::NexusAddress legacyAddress;

                        /* Extract the receiving legacy address */
                        Legacy::ExtractAddress(script, legacyAddress);

                        /* Output the json information. */
                        ret["OP"]       = "LEGACY";
                        ret["from"]     = hashFrom.ToString();

                        /* Resolve the name of the token/account that the debit is from */
                        std::string strFrom = Names::ResolveName(hashCaller, hashFrom);
                        if(!strFrom.empty())
                            ret["from_name"] = strFrom;

                        ret["to"]       = legacyAddress.ToString();

                        /* Get the token/account we are debiting from so that we can output the token address / name. */
                        TAO::Register::Object object;
                        if(!LLD::Register->ReadState(hashFrom, object))
                            throw APIException(-13, "Account not found");

                        /* Parse the object register. */
                        if(!object.Parse())
                            throw APIException(-14, "Object failed to parse");

                        /* Add the amount to the response */
                        ret["amount"]  = (double) nAmount / pow(10, GetDigits(object));

                        /* Get the object standard. */
                        uint8_t nStandard = object.Standard();

                        /* Check the object standard. */
                        if(nStandard != TAO::Register::OBJECTS::ACCOUNT
                        && nStandard != TAO::Register::OBJECTS::TRUST
                        && nStandard != TAO::Register::OBJECTS::TOKEN)
                            throw APIException(-15, "Object is not an account or token");

                        /* Get the token address */
                        uint256_t hashToken = object.get<uint256_t>("token");

                        /* Add the token address to the response */
                        ret["token"]   = hashToken.GetHex();
                        ret["token_name"] = "NXS";


                        break;
                    }
                }
            }
            catch(const std::exception& e)
            {
                debug::error(FUNCTION, e.what());
            }

            return ret;
        }


        /* Converts an Object Register to formattted JSON */
        json::json ObjectToJSON(const json::json& params,
                                const TAO::Register::Object& object,
                                const uint256_t& hashRegister,
                                bool fLookupName /*= true*/)
        {
            /* Declare the return JSON object */
            json::json ret;

            /* If the caller has specified to look up the name */
            if(fLookupName)
            {
                /* Get callers hashGenesis . */
                uint256_t hashGenesis = users->GetCallersGenesis(params);

                /* Don't attempt to resolve the object name if there is no logged in user as there will be no sig chain  to scan */
                if(hashGenesis != 0)
                {
                    /* Look up the object name based on the Name records in the caller's sig chain */
                    std::string strName = Names::ResolveName(hashGenesis, hashRegister);

                    /* Add the name to the response if one is found. */
                    if(!strName.empty())
                        ret["name"] = strName;
                }
            }

            /* Now build the response based on the register type */
            if(object.nType == TAO::Register::REGISTER::APPEND
            || object.nType == TAO::Register::REGISTER::RAW)
            {
                /* Raw state assets only have one data member containing the raw hex-encoded data*/

                ret["address"]    = hashRegister.ToString();
                ret["created"]    = object.nCreated;
                ret["modified"]   = object.nModified;
                ret["owner"]      = object.hashOwner.ToString();

                /* If this is an append register we need to grab the data from the end of the stream which will be the most recent data */
                while(!object.end())
                {
                    /* If the data type is string. */
                    std::string data;
                    object >> data;

                    //ret["checksum"] = state.hashChecksum;
                    ret["data"] = data;
                }

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
                        std::string strTokenName = Names::ResolveAccountTokenName(params, object);
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

                    /* Handle for a Name Object. */
                    case TAO::Register::OBJECTS::NAME:
                    {
                        ret["address"]          = hashRegister.ToString();
                        ret["name"]             = object.get<std::string>("name");
                        
                        std::string strNamespace = object.get<std::string>("namespace");
                        if(strNamespace == TAO::Register::NAMESPACE::GLOBAL)
                        {
                            ret["namespace"] = "";
                            ret["global"] = true;
                        }
                        else
                        {
                            ret["namespace"] = strNamespace;
                            ret["global"] = true;
                        }
                        
                        
                        ret["register_address"] = object.get<uint256_t>("address").GetHex();

                        break;
                    }

                    /* Handle for a Name Object. */
                    case TAO::Register::OBJECTS::NAMESPACE:
                    {
                        ret["address"]          = hashRegister.ToString();
                        ret["name"]             = object.get<std::string>("namespace");

                        break;
                    }

                    /* Handle for all nonstandard object registers. */
                    default:
                    {
                        ret["address"]    = hashRegister.ToString();

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
                                    ret[strName] = object.get<uint64_t>(strName);

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
