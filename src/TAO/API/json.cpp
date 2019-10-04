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
#include <Legacy/types/address.h>
#include <Legacy/types/trustkey.h>

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

#include <TAO/Register/include/unpack.h>

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
                    if(vtx.first == TAO::Ledger::TRANSACTION::TRITIUM)
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
                    else if(vtx.first == TAO::Ledger::TRANSACTION::LEGACY)
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
        json::json TransactionToJSON(const uint256_t& hashCaller, const TAO::Ledger::Transaction& tx,
                                     const TAO::Ledger::BlockState& block, uint32_t nVerbosity, const uint256_t& hashCoinbase)
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
                ret["contracts"] = ContractsToJSON(hashCaller, tx, nVerbosity, hashCoinbase);

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
                        TAO::Register::Address hashRegister;
                        if(Legacy::ExtractAddress(txprev.vout[txin.prevout.n].scriptPubKey, address))
                            input["address"] = address.ToString();
                        else if(Legacy::ExtractRegister(txprev.vout[txin.prevout.n].scriptPubKey, hashRegister))
                            input["address"] = hashRegister.ToString();
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
                    TAO::Register::Address hashRegister;
                    if(Legacy::ExtractAddress(txout.scriptPubKey, address))
                        output["address"] = address.ToString();
                    else if(Legacy::ExtractRegister(txout.scriptPubKey, hashRegister))
                        output["address"] = hashRegister.ToString();
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
        json::json ContractsToJSON(const uint256_t& hashCaller, const TAO::Ledger::Transaction &tx, uint32_t nVerbosity, const uint256_t& hashCoinbase)
        {
            /* Declare the return JSON object*/
            json::json ret = json::json::array();

            /* Add a contract to the list of contracts. */
            uint32_t nContracts = tx.Size();
            for(uint32_t nContract = 0; nContract < nContracts; ++nContract)
            {
                const TAO::Operation::Contract& contract = tx[nContract];
                /* If the caller has requested to filter the coinbases then we only include those where the coinbase is meant for hashCoinbase  */
                if(hashCoinbase != 0)
                {
                    if(TAO::Register::Unpack(contract, TAO::Operation::OP::COINBASE))
                    {
                        /* The proof (owner) of the coinbase */
                        uint256_t hashProof = 0;

                        /* Unpack the owner from the contract */
                        TAO::Register::Unpack(contract, hashProof);

                        /* Skip this contract if the proof is not the hashCoinbase */
                        if(hashProof != hashCoinbase)
                            continue;
                    }
                }

                /* JSONify the contract */
                json::json contractJSON = ContractToJSON(hashCaller, contract, nContract, nVerbosity);

                /* add the contract to the array */
                ret.push_back(contractJSON);
            }

            return ret;
        }


        /* Converts a serialized operation stream to formattted JSON */
        json::json ContractToJSON(const uint256_t& hashCaller, const TAO::Operation::Contract& contract, uint32_t nContract, uint32_t nVerbosity)
        {
            /* Declare the return JSON object*/
            json::json ret;

            /* Add the id */
            ret["id"] = nContract;

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
                        TAO::Register::Address hashAddress;
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
                        TAO::Register::Address hashAddress;
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
                        TAO::Register::Address hashAddress;
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
                        TAO::Register::Address hashAddress;
                        contract >> hashAddress;

                        /* Read the register transfer recipient. */
                        TAO::Register::Address hashTransfer;
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

                        ret["force"]    = nType == TAO::Operation::TRANSFER::FORCE;

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
                        TAO::Register::Address hashAddress;
                        contract >> hashAddress;

                        /* Output the json information. */
                        ret["OP"]         = "CLAIM";
                        ret["txid"]       = hashTx.ToString();
                        ret["contract"]     = nContract;
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

                        /* Change to stake amount. */
                        int64_t nStakeChange = 0;
                        contract >> nStakeChange;

                        /* The total trust reward. */
                        uint64_t nReward = 0;
                        contract >> nReward;

                        /* Output the json information. */
                        ret["OP"]      = "TRUST";
                        ret["last"]    = hashLastTrust.ToString();
                        ret["score"]   = nScore;
                        ret["amount"]  = (double) nReward / TAO::Ledger::NXS_COIN;

                        if(nStakeChange > 0)
                            ret["add_stake"] = (double) nStakeChange / TAO::Ledger::NXS_COIN;

                        else if (nStakeChange < 0)
                            ret["unstake"] = (double) (0 - nStakeChange) / TAO::Ledger::NXS_COIN;

                        break;
                    }


                    /* Genesis operation. Begins trust and stakes. */
                    case TAO::Operation::OP::GENESIS:
                    {
                        /* The total trust reward. */
                        uint64_t nReward = 0;
                        contract >> nReward;

                        /* Output the json information. */
                        ret["OP"]        = "GENESIS";
                        ret["amount"]    = (double) nReward / TAO::Ledger::NXS_COIN;;

                        break;
                    }


                    /* Debit tokens from an account you own. */
                    case TAO::Operation::OP::DEBIT:
                    {
                        /* Get the register address. */
                        TAO::Register::Address hashFrom;
                        contract >> hashFrom;

                        /* Get the transfer address. */
                        TAO::Register::Address hashTo;
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
                        ret["amount"]  = (double) nAmount / pow(10, GetDecimals(object));

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
                        TAO::Register::Address hashToken = object.get<uint256_t>("token");

                        /* Add the token address to the response */
                        ret["token"]   = hashToken.ToString();

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
                        TAO::Register::Address hashAddress;
                        contract >> hashAddress;

                        /* Get the transfer address. */
                        TAO::Register::Address hashProof;
                        contract >> hashProof;

                        /* Get the transfer amount. */
                        uint64_t  nCredit = 0;
                        contract >> nCredit;

                        /* Output the json information. */
                        ret["OP"]      = "CREDIT";

                        /* Determine the transaction type that this credit is made for */
                        std::string strInput;
                        if(hashTx.GetType() == TAO::Ledger::LEGACY)
                            strInput = "LEGACY";
                        else if(hashProof == hashCaller)
                            strInput = "COINBASE";
                        else
                            strInput = "DEBIT";

                        ret["for"]      = strInput;

                        ret["txid"]    = hashTx.ToString();
                        ret["contract"]  = nID;
                        ret["proof"]   = hashProof.ToString();
                        ret["to"] = hashAddress.ToString();

                        /* Resolve the name of the account that the credit is to */
                        std::string strAccount = Names::ResolveName(hashCaller, hashAddress);
                        if(!strAccount.empty())
                            ret["to_name"] = strAccount;

                        /* Get the token/account we are crediting to so that we can output the token address / name. */
                        TAO::Register::Object account;
                        if(!LLD::Register->ReadState(hashAddress, account))
                            throw APIException(-13, "Account not found");

                        /* Parse the object register. */
                        if(!account.Parse())
                            throw APIException(-14, "Object failed to parse");

                        /* Add the amount to the response */
                        ret["amount"]  = (double) nCredit / pow(10, GetDecimals(account));

                        /* Get the object standard. */
                        uint8_t nStandard = account.Standard();

                        /* Check the object standard. */
                        if(nStandard != TAO::Register::OBJECTS::ACCOUNT
                        && nStandard != TAO::Register::OBJECTS::TRUST
                        && nStandard != TAO::Register::OBJECTS::TOKEN)
                            throw APIException(-15, "Object is not an account or token");

                        /* Get the token address */
                        TAO::Register::Address hashToken = account.get<uint256_t>("token");

                        /* Add the token address to the response */
                        ret["token"]   = hashToken.ToString();

                        /* Resolve the name of the token name */
                        std::string strToken = hashToken != 0 ? Names::ResolveName(hashCaller, hashToken) : "NXS";
                        if(!strToken.empty())
                            ret["token_name"] = strToken;

                        /* Check type transaction that was credited */
                        if(hashTx.GetType() == TAO::Ledger::TRITIUM)
                        {
                            /* The debit transaction being credited */
                            TAO::Ledger::Transaction txDebit;

                            /* Read the corresponding debit/coinbase transaction */
                            if(LLD::Ledger->ReadTx(hashTx, txDebit))
                            {
                                /* Get the contract. */
                                const TAO::Operation::Contract& debitContract = txDebit[nID];

                                /* Only add reference if the credit is for a debit (rather than a coinbase) */
                                if(TAO::Register::Unpack(debitContract, TAO::Operation::OP::DEBIT))
                                {
                                    /* Get the address the debit came from */
                                    TAO::Register::Address hashFrom;
                                    TAO::Register::Unpack(debitContract, hashFrom);

                                    ret["from"]     = hashFrom.ToString();

                                    /* Resolve the name of the token/account that the debit is from */
                                    std::string strFrom = Names::ResolveName(hashCaller, hashFrom);
                                    if(!strFrom.empty())
                                        ret["from_name"] = strFrom;

                                    /* Reset the operation stream position in case it was loaded from mempool and therefore still in previous state */
                                    debitContract.Reset();

                                    /* Seek to reference. */
                                    debitContract.Seek(73);

                                    /* The reference */
                                    uint64_t nReference = 0;
                                    debitContract >> nReference;

                                    ret["reference"] = nReference;
                                }
                            }

                        }

                        break;
                    }


                    /* Migrate a trust key to a trust account register. */
                    case TAO::Operation::OP::MIGRATE:
                    {
                        /* Extract the transaction from contract. */
                        uint512_t hashTx;
                        contract >> hashTx;

                        /* Get the trust register address. (hash to) */
                        TAO::Register::Address hashAccount;
                        contract >> hashAccount;

                        /* Get thekey for the  Legacy trust key */
                        uint576_t hashTrust;
                        contract >> hashTrust;

                        /* Get the amount to migrate. */
                        uint64_t nAmount;
                        contract >> nAmount;

                        /* Get the trust score to migrate. */
                        uint32_t nScore;
                        contract >> nScore;

                        /* Get the hash last stake. */
                        uint512_t hashLast;
                        contract >> hashLast;

                        /* Output the json information. */
                        ret["OP"]      = "MIGRATE";
                        ret["txid"]    = hashTx.ToString();
                        ret["account"]  = hashAccount.ToString();

                        /* Resolve the name of the account that the credit is to */
                        std::string strAccount = Names::ResolveName(hashCaller, hashAccount);
                        if(!strAccount.empty())
                            ret["account_name"] = strAccount;

                        Legacy::TrustKey trustKey;
                        if(LLD::Trust->ReadTrustKey(hashTrust, trustKey))
                        {
                            Legacy::NexusAddress address;
                            address.SetPubKey(trustKey.vchPubKey);
                            ret["trustkey"] = address.ToString();
                        }

                        ret["amount"] = (double) nAmount / TAO::Ledger::NXS_COIN;
                        ret["score"] = nScore;
                        ret["hashLast"] = hashLast.ToString();

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
                        TAO::Register::Address hashAccount;
                        contract >> hashAccount;

                        /* The fee amount. */
                        uint64_t nFee = 0;
                        contract >> nFee;

                        /* Output the json information. */
                        ret["OP"]      = "FEE";
                        ret["from"] = hashAccount.ToString();

                        /* Resolve the name of the account that the credit is to */
                        std::string strAccount = Names::ResolveName(hashCaller, hashAccount);
                        if(!strAccount.empty())
                            ret["from_name"] = strAccount;

                        ret["amount"]  = (double) nFee / TAO::Ledger::NXS_COIN;

                        break;
                    }

                    /* Debit NXS to legacy address. */
                    case TAO::Operation::OP::LEGACY:
                    {
                        /* Get the register address. */
                        TAO::Register::Address hashFrom;
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
                        ret["amount"]  = (double) nAmount / pow(10, GetDecimals(object));

                        /* Get the object standard. */
                        uint8_t nStandard = object.Standard();

                        /* Check the object standard. */
                        if(nStandard != TAO::Register::OBJECTS::ACCOUNT
                        && nStandard != TAO::Register::OBJECTS::TRUST
                        && nStandard != TAO::Register::OBJECTS::TOKEN)
                            throw APIException(-15, "Object is not an account or token");

                        /* Get the token address */
                        TAO::Register::Address hashToken = object.get<uint256_t>("token");

                        /* Add the token address to the response */
                        ret["token"]   = hashToken.ToString();
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
                                const TAO::Register::Address& hashRegister,
                                bool fLookupName /*= true*/)
        {
            /* Declare the return JSON object */
            json::json ret;

            /* Get callers hashGenesis . */
            uint256_t hashGenesis = users->GetCallersGenesis(params);

            /* The resolved name for this object */
            std::string strName = "";

            /* If the caller has specified to look up the name */
            if(fLookupName)
            {
                /* Look up the object name based on the Name records in the caller's sig chain */
                strName = Names::ResolveName(hashGenesis, hashRegister);

                /* Add the name to the response if one is found. */
                if(!strName.empty())
                    ret["name"] = strName;

            }

            /* Now build the response based on the register type */
            if(object.nType == TAO::Register::REGISTER::APPEND
            || object.nType == TAO::Register::REGISTER::RAW)
            {
                /* Raw state assets only have one data member containing the raw hex-encoded data*/

                ret["address"]    = hashRegister.ToString();
                ret["created"]    = object.nCreated;
                ret["modified"]   = object.nModified;
                ret["owner"]      = TAO::Register::Address(object.hashOwner).ToString();

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
                    case TAO::Register::OBJECTS::TRUST:
                    {
                        ret["address"]    = hashRegister.ToString();

                        /* Get the token names. */
                        std::string strTokenName = Names::ResolveAccountTokenName(params, object);
                        if(!strTokenName.empty())
                            ret["token_name"] = strTokenName;

                        /* Set the value to the token contract address. */
                        TAO::Register::Address hashToken = object.get<uint256_t>("token");
                        ret["token"] = hashToken.ToString();

                        /* Handle digit conversion. */
                        uint8_t nDecimals = GetDecimals(object);

                        /* In order to get the balance for this account we need to ensure that we use the state from disk, which
                           will contain the confirmed balance.  If this is a new account then it won't be on disk yet so the
                           confirmed balance is 0 */
                        uint64_t nConfirmedBalance = 0;
                        TAO::Register::Object account;
                        if(LLD::Register->ReadState(hashRegister, account))
                        {
                            /* Parse the object register. */
                            if(!account.Parse())
                                throw APIException(-14, "Object failed to parse");

                            nConfirmedBalance = account.get<uint64_t>("balance");
                        }

                        /* Find all pending debits to NXS accounts */
                        uint64_t nPending = GetPending(object.hashOwner, hashToken, hashRegister);

                        /* Get unconfirmed debits coming in and credits we have made */
                        uint64_t nUnconfirmed = GetUnconfirmed(object.hashOwner, hashToken, false, hashRegister);

                        /* Get all new debits that we have made */
                        uint64_t nUnconfirmedOutgoing = GetUnconfirmed(object.hashOwner, hashToken, true, hashRegister);

                        /* Calculate the available balance which is the last confirmed balance minus and mempool debits */
                        uint64_t nAvailable = nConfirmedBalance - nUnconfirmedOutgoing;

                        ret["balance"]      = (double)nAvailable / pow(10, nDecimals);
                        ret["pending"]      = (double)nPending / pow(10, nDecimals);
                        ret["unconfirmed"]  = (double)nUnconfirmed / pow(10, nDecimals);

                        /* Add Trust specific fields */
                        if(nStandard == TAO::Register::OBJECTS::TRUST)
                        {
                            /* The amount being staked */
                            ret["stake"]    = (double)object.get<uint64_t>("stake") / pow(10, nDecimals);
                        }


                        break;
                    }


                    /* Handle for a token contract. */
                    case TAO::Register::OBJECTS::TOKEN:
                    {
                        /* Handle decimals conversion. */
                        uint8_t nDecimals = GetDecimals(object);

                        /* In order to get the balance for this account we need to ensure that we use the state from disk, which
                           will contain the confirmed balance.  If this is a new account then it won't be on disk yet so the
                           confirmed balance is 0 */
                        uint64_t nConfirmedBalance = 0;
                        TAO::Register::Object account;
                        if(LLD::Register->ReadState(hashRegister, account))
                        {
                            /* Parse the object register. */
                            if(!account.Parse())
                                throw APIException(-14, "Object failed to parse");

                            nConfirmedBalance = account.get<uint64_t>("balance");
                        }

                        /* Find all pending debits to NXS accounts */
                        uint64_t nPending = GetPending(object.hashOwner, hashRegister, hashRegister);

                        /* Get unconfirmed debits coming in and credits we have made */
                        uint64_t nUnconfirmed = GetUnconfirmed(object.hashOwner, hashRegister, false, hashRegister);

                        /* Get all new debits that we have made */
                        uint64_t nUnconfirmedOutgoing = GetUnconfirmed(object.hashOwner, hashRegister, true, hashRegister);

                        /* Calculate the available balance which is the last confirmed balance minus and mempool debits */
                        uint64_t nAvailable = nConfirmedBalance - nUnconfirmedOutgoing;

                        ret["address"]          = hashRegister.ToString();
                        ret["balance"]          = (double)nAvailable / pow(10, nDecimals);
                        ret["pending"]          = (double)nPending / pow(10, nDecimals);
                        ret["unconfirmed"]      = (double)nUnconfirmed / pow(10, nDecimals);

                        ret["maxsupply"]        = (double) object.get<uint64_t>("supply") / pow(10, nDecimals);
                        ret["currentsupply"]    = (double) (object.get<uint64_t>("supply")
                                                - object.get<uint64_t>("balance")) / pow(10, nDecimals); // current supply is based on unconfirmed balance
                        ret["decimals"]           = nDecimals;

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
                            ret["global"] = false;
                        }


                        ret["register_address"] = TAO::Register::Address(object.get<uint256_t>("address")).ToString();

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
                        /* Get the owner of the object */
                        TAO::Register::Address hashOwner = TAO::Register::Address(object.hashOwner);

                        /* If this is a tokenized asset and we haven't resolved a name for it based on the callers sig chain
                           then attempt to resolve the name of it based on the token owners sig chain */
                        if(strName.empty() && hashOwner.IsToken())
                        {
                            /* Retrieve the token owning the asset */
                            TAO::Register::Object token;
                            if(LLD::Register->ReadState(hashOwner, token))
                            {
                                /* Look up the object name based on the Name records in the token owners sig chain */
                                strName = Names::ResolveName(token.hashOwner, hashRegister);

                                /* Add the name to the response if one is found. */
                                if(!strName.empty())
                                    ret["name"] = strName;
                            }
                        }

                        /* Add the register address */
                        ret["address"]    = hashRegister.ToString();

                        /* Add ownership details */
                        ret["owner"]    = hashOwner.ToString();

                        /* If this is a tokenized asset then work out what % the caller owns */
                        if(hashOwner.IsToken())
                            ret["ownership"] = GetTokenOwnership(hashOwner, hashGenesis);

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
