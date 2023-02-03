/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2021

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <Legacy/include/evaluate.h>
#include <Legacy/include/money.h>
#include <Legacy/types/address.h>

#include <TAO/API/include/check.h>
#include <TAO/API/include/format.h>
#include <TAO/API/include/get.h>
#include <TAO/API/include/json.h>
#include <TAO/API/types/exception.h>
#include <TAO/API/types/commands.h>

#include <TAO/API/types/commands/names.h>

#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/include/difficulty.h>
#include <TAO/Ledger/include/retarget.h>
#include <TAO/Ledger/include/supply.h>
#include <TAO/Ledger/types/tritium.h>
#include <TAO/Ledger/types/sigchain.h>

#include <TAO/Operation/include/enum.h>
#include <TAO/Operation/types/contract.h>

#include <TAO/Register/include/constants.h>

#include <Util/include/debug.h>
#include <Util/include/hex.h>
#include <Util/include/json.h>
#include <Util/include/base64.h>
#include <Util/include/string.h>
#include <Util/include/math.h>

#include <Util/encoding/include/utf-8.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Converts the block to formatted JSON */
    encoding::json BlockToJSON(const TAO::Ledger::BlockState& block, const uint32_t nVerbose)
    {
        /* Decalre the response object*/
        encoding::json result;

        /* Main block hash. */
        result["hash"] = block.GetHash().GetHex();

        /* The hash that was relevant for Proof of Stake or Proof of Work (depending on block version) */
        result["proofhash"]  =
                                block.nVersion < 5 ? block.GetHash().GetHex() :
                                ((block.nChannel == 0) ? block.StakeHash().GetHex() : block.ProofHash().GetHex());

        /* Body of the block with relevant data. */
        result["size"]       = ::GetSerializeSize(block, SER_NETWORK, LLP::PROTOCOL_VERSION);
        result["height"]     = block.nHeight;
        result["channel"]    = block.nChannel;
        result["version"]    = block.nVersion;
        result["merkleroot"] = block.hashMerkleRoot.GetHex();
        result["timestamp"]       = block.GetBlockTime();
        result["date"]       = convert::DateTimeStrFormat(block.GetBlockTime());
        result["nonce"]      = (uint64_t)block.nNonce;
        result["bits"]       = HexBits(block.nBits);
        result["difficulty"] = TAO::Ledger::GetDifficulty(block.nBits, block.nChannel);
        result["mint"]       = FormatMint(block.nMint);

        /* Add previous block if not null. */
        if(block.hashPrevBlock != 0)
            result["previousblockhash"] = block.hashPrevBlock.GetHex();

        /* Add next hash if not null. */
        if(block.hashNextBlock != 0)
            result["nextblockhash"] = block.hashNextBlock.GetHex();

        /* Add the transaction data if the caller has requested it*/
        if(nVerbose > 0)
        {
            /* Build our transactions array seperate from results. */
            encoding::json jTransactions = encoding::json::array();

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
                        const encoding::json jRet =
                            TransactionToJSON(tx, block, nVerbose);

                        /* Only add the transaction if it has not been filtered out */
                        if(!jRet.empty())
                            jTransactions.push_back(jRet);
                    }
                }
                else if(vtx.first == TAO::Ledger::TRANSACTION::LEGACY)
                {
                    /* Get the legacy transaction from the database. */
                    Legacy::Transaction tx;
                    if(LLD::Legacy->ReadTx(vtx.second, tx))
                    {
                        /* add the transaction JSON.  */
                        const encoding::json jRet =
                            TransactionToJSON(tx, block, nVerbose);

                        /* Only add the transaction if it has not been filtered out */
                        if(!jRet.empty())
                            jTransactions.push_back(jRet);
                    }
                }
            }

            /* Check to see if any transactions were returned.  If not then return an empty tx array */
            if(!jTransactions.empty())
                result["tx"] = jTransactions;
            else
                result = encoding::json();

        }

        return result;
    }


    /* Converts the transaction to formatted JSON */
    encoding::json TransactionToJSON(const TAO::Ledger::Transaction& tx, const TAO::Ledger::BlockState& block, const uint32_t nVerbose)
    {
        /* Declare JSON object to return */
        encoding::json jRet;

        /* Always add the transaction hash */
        jRet["txid"] = tx.GetHash().GetHex();

        /* Basic TX info for level 2 and up */
        if(nVerbose >= 2)
        {
            /* Build base transaction data. */
            jRet["type"]      = tx.TypeString();
            jRet["version"]   = tx.nVersion;
            jRet["sequence"]  = tx.nSequence;
            jRet["timestamp"] = tx.nTimestamp;

            /* Add blockchain related data if requested. */
            if(!block.IsNull())
            {
                jRet["blockhash"]     = block.GetHash().GetHex();
                jRet["confirmations"] = TAO::Ledger::ChainState::nBestHeight.load() - block.nHeight + 1;
            }

            /* Genesis and hashes are verbose 3 and up. */
            if(nVerbose >= 3)
            {
                /* More sigchain level details. */
                jRet["genesis"]   = tx.hashGenesis.ToString();
                jRet["nexthash"]  = tx.hashNext.ToString();
                jRet["prevhash"]  = tx.hashPrevTx.ToString();

                /* Check for recovery hash. */
                if(tx.hashRecovery != 0)
                    jRet["recovery"] = tx.hashRecovery.ToString();

                /* The cryptographic data. */
                jRet["pubkey"]    = HexStr(tx.vchPubKey.begin(), tx.vchPubKey.end());
                jRet["signature"] = HexStr(tx.vchSig.begin(),    tx.vchSig.end());
            }

            /* Check to see if any contracts were returned.  If not then return an empty transaction */
            jRet["contracts"] = encoding::json::array();

            /* Add a contract to the list of contracts. */
            const uint32_t nContracts = tx.Size();
            for(uint32_t nContract = 0; nContract < nContracts; ++nContract)
            {
                /* Grab a reference of our contract. */
                const TAO::Operation::Contract& rContract = tx[nContract];

                /* Add the contract to the array */
                jRet["contracts"].push_back
                (
                    ContractToJSON(rContract, nContract, nVerbose)
                );
            }
        }

        return jRet;
    }

    /* Converts the transaction to formatted JSON */
    encoding::json TransactionToJSON(const Legacy::Transaction& tx, const TAO::Ledger::BlockState& block, const uint32_t nVerbose)
    {
        /* Declare JSON object to return */
        encoding::json jRet;

        /* Always add the hash */
        jRet["txid"] = tx.GetHash().GetHex();

        /* Basic TX info for level 1 and up */
        if(nVerbose > 0)
        {
            /* Populate our JSON object. */
            jRet["type"]          = tx.TypeString();
            jRet["timestamp"]     = tx.nTime;
            jRet["amount"]        = Legacy::SatoshisToAmount(tx.GetValueOut());
            jRet["blockhash"]     = block.IsNull() ? "" : block.GetHash().GetHex();
            jRet["confirmations"] = block.IsNull() ? 0 : TAO::Ledger::ChainState::nBestHeight.load() - block.nHeight + 1;

            /* Don't add inputs for coinbase or coinstake transactions */
            if(!tx.IsCoinBase())
            {
                /* Declare the inputs JSON array */
                encoding::json jInputs = encoding::json::array();

                /* Iterate through each input */
                for (uint32_t i = (uint32_t)tx.IsCoinStake(); i < tx.vin.size(); ++i)
                {
                    const Legacy::TxIn& txin = tx.vin[i];

                    encoding::json jInput;
                    bool fFound = false;

                    if(tx.nVersion >= 2 && txin.prevout.hash.GetType() == TAO::Ledger::TRITIUM)
                    {
                        /* Previous output likely a Tritium send-to-legacy contract. Check for that first. It is possible for
                         * an older legacy tx to collide with the tritium hash type, so if not found still must check legacy.
                         */
                        TAO::Ledger::Transaction txPrev;
                        if(LLD::Ledger->ReadTx(txin.prevout.hash, txPrev))
                        {
                            fFound = true;

                            if(txPrev.Size() < txin.prevout.n)
                                throw Exception(-87, "Invalid or unknown transaction");

                            /* We build based on contract if our input is a tritium contract . */
                            jInput = ContractToJSON(txPrev[txin.prevout.n], txin.prevout.n, nVerbose);
                            jInputs.push_back(jInput);
                        }
                    }

                    if(!fFound)
                    {
                        /* Read the previous legacy transaction in order to get its outputs */
                        Legacy::Transaction txPrev;

                        if(LLD::Legacy->ReadTx(txin.prevout.hash, txPrev))
                        {
                            fFound = true;

                            /* Extract the Nexus Address. */
                            Legacy::NexusAddress address;
                            TAO::Register::Address hashRegister;

                            if(Legacy::ExtractAddress(txPrev.vout[txin.prevout.n].scriptPubKey, address))
                                jInput["address"] = address.ToString();

                            else if(Legacy::ExtractRegister(txPrev.vout[txin.prevout.n].scriptPubKey, hashRegister))
                                jInput["address"] = hashRegister.ToString();

                            else
                                throw Exception(-8, "Unable to Extract Input Address");

                            jInput["amount"] = (double) txPrev.vout[txin.prevout.n].nValue / TAO::Ledger::NXS_COIN;
                            jInputs.push_back(jInput);
                        }
                    }

                    //if(!fFound)
                    //    throw Exception(-7, "Invalid transaction id");
                }

                jRet["inputs"] = jInputs;
            }

            /* Declare the output JSON array */
            encoding::json outputs = encoding::json::array();

            /* Iterate through each output */
            for(const Legacy::TxOut& txout : tx.vout)
            {
                encoding::json output;
                /* Extract the Nexus Address. */
                Legacy::NexusAddress address;
                TAO::Register::Address hashRegister;

                if(Legacy::ExtractAddress(txout.scriptPubKey, address))
                    output["address"] = address.ToString();

                else if(Legacy::ExtractRegister(txout.scriptPubKey, hashRegister))
                    output["address"] = hashRegister.ToString();

                else if(block.nHeight == 112283) // Special case for malformed block 112283 with bad output address
                    output["address"] = "";
                else
                    throw Exception(-8, "Unable to Extract Output Address");

                output["amount"] = (double) txout.nValue / TAO::Ledger::NXS_COIN;

                outputs.push_back(output);

            }
            jRet["outputs"] = outputs;
        }

        return jRet;
    }


    /* Converts a serialized operation stream to formattted JSON */
    encoding::json ContractToJSON(const TAO::Operation::Contract& contract,
                                  const uint32_t nContract, const uint32_t nVerbose)
    {
        /* Declare the return JSON object*/
        encoding::json jRet;

        /* Add the id */
        jRet["id"] = nContract;

        /* Reset all streams */
        contract.Reset();

        /* Seek the contract operation stream to the position of the primitive. */
        contract.SeekToPrimitive();

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
                    jRet["OP"]      = "WRITE";
                    jRet["address"] = hashAddress.ToString();

                    /* Check if we have a name record available. */
                    std::string strName;
                    if(Names::ReverseLookup(hashAddress, strName))
                        jRet["name"] = strName;

                    /* Get the register pre-state so we can build response with old values before OP::WRITE. */
                    TAO::Register::Object object = contract.PreState();

                    /* Format write operation for different register types. */
                    if(object.nType == TAO::Register::REGISTER::OBJECT)
                    {
                        /* Parse our object if needed. */
                        if(!object.Parse())
                            throw Exception(-15, "Object failed to parse");

                        /* Build a response object. */
                        encoding::json jWrite = encoding::json::array();

                        /*  Loop through our OP::WRITE script. */
                        TAO::Operation::Stream stream = TAO::Operation::Stream(vchData);
                        while(!stream.end())
                        {
                            /* Deserialize the named value. */
                            std::string strName;
                            stream >> strName;

                            /* Deserialize the type. */
                            uint8_t nType;
                            stream >> nType;

                            /* Start the group check. */
                            encoding::json jGroup = { { "name", strName } };

                            /* Switch between supported types. */
                            switch(nType)
                            {
                                /* Standard type for C++ uint8_t. */
                                case TAO::Operation::OP::TYPES::UINT8_T:
                                {
                                    /* Get the byte from the stream. */
                                    uint8_t nValue;
                                    stream >> nValue;

                                    /* Add to our group object. */
                                    jGroup["old"] = object.get<uint8_t>(strName);
                                    jGroup["new"] = nValue;

                                    break;
                                }


                                /* Standard type for C++ uint16_t. */
                                case TAO::Operation::OP::TYPES::UINT16_T:
                                {
                                    /* Get the byte from the stream. */
                                    uint16_t nValue;
                                    stream >> nValue;

                                    /* Add to our group object. */
                                    jGroup["old"] = object.get<uint16_t>(strName);
                                    jGroup["new"] = nValue;

                                    break;
                                }


                                /* Standard type for C++ uint32_t. */
                                case TAO::Operation::OP::TYPES::UINT32_T:
                                {
                                    /* Get the byte from the stream. */
                                    uint32_t nValue;
                                    stream >> nValue;

                                    /* Add to our group object. */
                                    jGroup["old"] = object.get<uint32_t>(strName);
                                    jGroup["new"] = nValue;

                                    break;
                                }


                                /* Standard type for C++ uint64_t. */
                                case TAO::Operation::OP::TYPES::UINT64_T:
                                {
                                    /* Get the byte from the stream. */
                                    uint64_t nValue;
                                    stream >> nValue;

                                    /* Add to our group object. */
                                    jGroup["old"] = object.get<uint64_t>(strName);
                                    jGroup["new"] = nValue;

                                    break;
                                }


                                /* Standard type for Custom uint256_t */
                                case TAO::Operation::OP::TYPES::UINT256_T:
                                {
                                    /* Get the byte from the stream. */
                                    uint256_t nValue;
                                    stream >> nValue;

                                    /* Add to our group object. */
                                    jGroup["old"] = object.get<uint256_t>(strName).ToString();
                                    jGroup["new"] = nValue.ToString();

                                    break;
                                }


                                /* Standard type for Custom uint512_t */
                                case TAO::Operation::OP::TYPES::UINT512_T:
                                {
                                    /* Get the byte from the stream. */
                                    uint512_t nValue;
                                    stream >> nValue;

                                    /* Add to our group object. */
                                    jGroup["old"] = object.get<uint512_t>(strName).ToString();
                                    jGroup["new"] = nValue.ToString();

                                    break;
                                }


                                /* Standard type for Custom uint1024_t */
                                case TAO::Operation::OP::TYPES::UINT1024_T:
                                {
                                    /* Get the byte from the stream. */
                                    uint1024_t nValue;
                                    stream >> nValue;

                                    /* Add to our group object. */
                                    jGroup["old"] = object.get<uint1024_t>(strName).ToString();
                                    jGroup["new"] = nValue.ToString();

                                    break;
                                }


                                /* Standard type for STL string */
                                case TAO::Operation::OP::TYPES::STRING:
                                {
                                    /* Get the byte from the stream. */
                                    std::string strValue;
                                    stream >> strValue;

                                    /* Grab a copy of our old value. */
                                    const std::string strOlder =
                                        object.get<std::string>(strName);

                                    /* Add to our group object. */
                                    jGroup["old"] = strOlder.substr(0, strOlder.find_last_not_of('\0') + 1);
                                    jGroup["new"] = strValue.substr(0, strValue.find_last_not_of('\0') + 1);

                                    break;
                                }


                                /* Standard type for STL vector with C++ type uint8_t */
                                case TAO::Operation::OP::TYPES::BYTES:
                                {
                                    /* Get the byte from the stream. */
                                    std::vector<uint8_t> vNew;
                                    stream >> vNew;

                                    /* Grab the old data from pre-state. */
                                    const std::vector<uint8_t> vOld =
                                        object.get<std::vector<uint8_t>>(strName);

                                    /* Add to our group object. */
                                    jGroup["old"] = HexStr(vOld.begin(), vOld.end());
                                    jGroup["new"] = HexStr(vNew.begin(), vNew.end());

                                    break;
                                }


                                /* Fail if types are unknown. */
                                default:
                                    return debug::error(FUNCTION, "malformed stream (unexpected type ", uint32_t(nType), "");
                            }

                            /* Add our write operation to arrays. */
                            jWrite.push_back(jGroup);
                        }

                        jRet["updated"] = jWrite;
                    }

                    /* Regular state register dump of data. */
                    else
                    {
                        /* Get a copy of our old data. */
                        const std::vector<uint8_t> vOld =
                            object.GetState();

                        /* Create our updated field. */
                        jRet["updated"] = encoding::json::array();
                        jRet["updated"].push_back
                        ({
                            { "name", "state"              },
                            { "old",  StateToJSON(vOld)    },
                            { "new",  StateToJSON(vchData) },
                        });
                    }

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
                    jRet["OP"]      = "APPEND";
                    jRet["address"] = hashAddress.ToString();

                    /* Check if we have a name record available. */
                    std::string strName;
                    if(Names::ReverseLookup(hashAddress, strName))
                        jRet["name"] = strName;

                    jRet["data"]    = HexStr(vchData.begin(), vchData.end());

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
                    jRet["OP"]      = "CREATE";
                    jRet["address"] = hashAddress.ToString();

                    /* Check if we have a name record available. */
                    std::string strName;
                    if(Names::ReverseLookup(hashAddress, strName))
                        jRet["name"] = strName;

                    jRet["type"]    = GetRegisterName(nType);

                    /* If this is a register object then decode the object type */
                    if(nType == TAO::Register::REGISTER::OBJECT)
                    {
                        /* Create the register object. */
                        TAO::Register::Object object;
                        object.nVersion   = 1;
                        object.nType      = nType;
                        object.hashOwner  = contract.Caller();

                        /* Set our state to parse. */
                        object.SetState(vchData);

                        /* parse object so that the data fields can be accessed */
                        if(!object.Parse())
                            throw Exception(-36, "Failed to parse object register");

                        /* Add object standard if available. */
                        jRet["standard"] = GetStandardName(object.Standard());

                        /* Add our fields json data now. */
                        jRet["object"]   = encoding::json::object();
                        MembersToJSON(object, jRet["object"]);
                    }

                    /* Handle for state registers. */
                    else
                        StateToJSON(vchData, jRet);

                    break;
                }


                /* Transfer ownership of a register to another signature chain. */
                case TAO::Operation::OP::TRANSFER:
                {
                    /* Extract the address from the contract. */
                    TAO::Register::Address hashAddress;
                    contract >> hashAddress;

                    /* Read the register transfer recipient. */
                    uint256_t hashTransfer;
                    contract >> hashTransfer;

                    /* Get the flag byte. */
                    uint8_t nType = 0;
                    contract >> nType;

                    /* Output the json information. */
                    jRet["OP"]          = "TRANSFER";
                    jRet["address"]     = hashAddress.ToString();

                    /* Check if we have a name record available. */
                    std::string strName;
                    if(Names::ReverseLookup(hashAddress, strName))
                        jRet["name"] = strName;

                    /* Check for tokenized transfer. */
                    if(nType == TAO::Operation::TRANSFER::FORCE)
                    {
                        /* Recipient is register address when tokenized. */
                        jRet["recipient"] =
                            TAO::Register::Address(hashTransfer).ToString();

                        /* Special flag to tell it's tokenized. */
                        jRet["tokenized"] = true;
                    }

                    /* Default logic for regular transfers. */
                    else
                    {
                        /* Add transfer address if not to wildcard. */
                        if(hashTransfer != TAO::Register::WILDCARD_ADDRESS)
                            jRet["recipient"] = hashTransfer.ToString();
                        else
                            jRet["wildcard"] = true;
                    }

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
                    jRet["OP"]         = "CLAIM";
                    jRet["txid"]       = hashTx.ToString();
                    jRet["contract"]   = nContract;
                    jRet["address"]    = hashAddress.ToString();

                    /* Check if we have a name record available. */
                    std::string strName;
                    if(Names::ReverseLookup(hashAddress, strName))
                        jRet["name"] = strName;

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
                    jRet["OP"]      = "COINBASE";
                    jRet["genesis"] = hashGenesis.ToString();
                    jRet["nonce"]   = nExtraNonce;
                    jRet["amount"]  = FormatBalance(nCredit);
                    jRet["token"]   = TOKEN::NXS.ToString();
                    jRet["ticker"]  = "NXS";

                    break;
                }


                /* Trust operation. Builds trust and generates reward. */
                case TAO::Operation::OP::TRUST:
                {
                    /* Get the last stake tx hash. */
                    uint512_t hashLastTrust = 0;
                    contract >> hashLastTrust;

                    /* The total trust score. */
                    uint64_t nScore = 0;
                    contract >> nScore;

                    /* Change to stake amount. */
                    int64_t nStakeChange = 0;
                    contract >> nStakeChange;

                    /* The trust reward. */
                    uint64_t nReward = 0;
                    contract >> nReward;

                    /* Cache our current address for genesis. */
                    const TAO::Register::Address hashAccount =
                        TAO::Register::Address("trust", contract.Caller(), TAO::Register::Address::TRUST);

                    /* Output the json information. */
                    jRet["OP"]      = "TRUST";
                    jRet["address"] = hashAccount.ToString();

                    /* Check if we have a name record available. */
                    std::string strName;
                    if(Names::ReverseLookup(hashAccount, strName))
                        jRet["name"] = strName;

                    jRet["last"]    = hashLastTrust.ToString();
                    jRet["score"]   = nScore;
                    jRet["amount"]  = FormatBalance(nReward);
                    jRet["token"]   = TOKEN::NXS.ToString();
                    jRet["ticker"]  = "NXS";

                    /* Handle for add stake. */
                    if(nStakeChange > 0)
                        jRet["add_stake"] = FormatStake(nStakeChange);

                    /* Handle for remove stake. */
                    if(nStakeChange < 0)
                        jRet["stake_change"] = FormatStake(0 - nStakeChange);

                    break;
                }


                /* Genesis operation. Begins trust and stakes. */
                case TAO::Operation::OP::GENESIS:
                {
                    /* The genesis reward. */
                    uint64_t nReward = 0;
                    contract >> nReward;

                    /* Cache our current address for genesis. */
                    const TAO::Register::Address hashAccount =
                        TAO::Register::Address("trust", contract.Caller(), TAO::Register::Address::TRUST);

                    /* Output the json information. */
                    jRet["OP"]      = "GENESIS";
                    jRet["address"] = hashAccount.ToString();

                    /* Check if we have a name record available. */
                    std::string strName;
                    if(Names::ReverseLookup(hashAccount, strName))
                        jRet["name"] = strName;

                    jRet["amount"]  = FormatBalance(nReward);
                    jRet["token"]   = 0;
                    jRet["ticker"]  = "NXS";

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
                    jRet["OP"]       = "DEBIT";

                    /* Build our from json key. */
                    encoding::json jFrom =
                        hashFrom.ToString();

                    /* Check if we have a name record available. */
                    std::string strFrom;
                    if(Names::ReverseLookup(hashFrom, strFrom))
                        jFrom = { { "address", hashFrom.ToString() }, { "name", strFrom } };

                    /* Add the from key address and name. */
                    jRet["from"]     = jFrom;

                    /* Check for wildcard address before adding key. */
                    if(hashTo != TAO::Register::WILDCARD_ADDRESS)
                    {
                        /* Build our from json key. */
                        encoding::json jTo =
                            hashTo.ToString();

                        /* Check if we have a name record available. */
                        std::string strTo;
                        if(Names::ReverseLookup(hashTo, strTo))
                            jTo = { { "address", hashTo.ToString() }, { "name", strTo } };

                        /* Add the from key address and name. */
                        jRet["to"]     = jTo;
                    }
                    else
                        jRet["wildcard"] = true;

                    /* Get the token/account we are debiting from so that we can output the token address / name. */
                    TAO::Register::Object object = contract.PreState();
                    if(!object.Parse())
                        throw Exception(-15, "Object is not an account or token");

                    /* Get the current token's address.  */
                    const TAO::Register::Address hashToken =
                        object.get<uint256_t>("token");

                    /* Add the amount to the response */
                    jRet["amount"]  = FormatBalance(nAmount, hashToken);
                    jRet["token"]   = hashToken.ToString();

                    /* Add a ticker if found. */
                    std::string strName;
                    if(Names::ReverseLookup(hashToken, strName))
                        jRet["ticker"] = strName;

                    /* Add our total funds claimed. */
                    jRet["claimed"]   = FormatBalance(0, hashToken);

                    /* Otherwise check our partial balance. */
                    if(hashTo.IsObject())
                    {
                        /* Get the partial amount already claimed. */
                        uint64_t nClaimed = 0;
                        if(!LLD::Ledger->ReadClaimed(contract.Hash(), nContract, nClaimed))
                            nClaimed = 0; //reset value to double check here and continue

                        /* Track how much has been claimed. */
                        jRet["claimed"] = FormatBalance(nClaimed, hashToken);
                    }

                    /* Check if this debit has been credited. */
                    else if(LLD::Ledger->HasProof(hashFrom, contract.Hash(), nContract))
                        jRet["claimed"] = FormatBalance(nAmount, hashToken);


                    /* Add the reference to the response */
                    jRet["reference"] = nReference;

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

                    /* Get the account address. */
                    TAO::Register::Address hashAddress;
                    contract >> hashAddress;

                    /* Get the proof address. */
                    TAO::Register::Address hashProof;
                    contract >> hashProof;

                    /* Get the transfer amount. */
                    uint64_t  nCredit = 0;
                    contract >> nCredit;

                    /* Output the json information. */
                    jRet["OP"]      = "CREDIT";

                    /* Determine the transaction type that this credit is made for */
                    std::string strInput;
                    if(hashTx.GetType() == TAO::Ledger::LEGACY)
                        jRet["for"] = "LEGACY";
                    else if(hashProof.IsAccount() || hashProof.IsToken() || hashProof.IsTrust())
                        jRet["for"] = "DEBIT";
                    else
                        jRet["for"] = "COINBASE";

                    /* Populate the rest of our data. */
                    jRet["txid"]     = hashTx.ToString();
                    jRet["contract"] = nContract;

                    /* Get the token/account we are debiting from so that we can output the token address / name. */
                    TAO::Register::Object object = contract.PreState();
                    if(!object.Parse())
                        throw Exception(-15, "Object is not an account or token");

                    /* Get the current token's address.  */
                    const TAO::Register::Address hashToken =
                        object.get<uint256_t>("token");

                    /* Only add proof if not a wildcard address. */
                    if(hashProof != TAO::Register::WILDCARD_ADDRESS)
                    {
                        /* Attempt to read the proof so we can show from or proof fields. */
                        TAO::Register::Object oProof;
                        if(LLD::Register->ReadObject(hashProof, oProof))
                        {
                            /* Check for matching token to indicate proof or from. */
                            const TAO::Register::Address hashProofToken = oProof.get<uint256_t>("token");
                            if(hashProofToken != hashToken)
                            {
                                /* Get our token object. */
                                TAO::Register::Object oToken;
                                if(LLD::Register->ReadObject(hashProofToken, oToken))
                                {
                                    /* Get our account balance. */
                                    const uint64_t nBalance = oProof.get<uint64_t>("balance");

                                    /* Create a proof object to show account and token. */
                                    encoding::json jProof;
                                    jProof["account"] = hashProof.ToString();

                                    /* Calculate our partial ownership now. */
                                    const uint64_t nOwnership =
                                        (nBalance * 10000) / oToken.get<uint64_t>("supply"); //we use 10000 as 100 * 100 for 100.00 percent displayed

                                    /* Add our ownership value to register. */
                                    //jProof["amount"]    = FormatBalance(nBalance, oToken.get<uint8_t>("decimals"));
                                    jProof["token"]     = hashProofToken.ToString();

                                    /* Add a ticker if found. */
                                    std::string strTicker;
                                    if(Names::ReverseLookup(hashProofToken, strTicker))
                                        jProof["ticker"] = strTicker;

                                    /* Track our ownership here. */
                                    jProof["ownership"] = double(nOwnership / 100.0);

                                    /* We want all the token related data for our proof. */
                                    jRet["proof"] = jProof;
                                }
                            }
                            else
                            {
                                /* Build our from json key. */
                                encoding::json jFrom =
                                    hashProof.ToString();

                                /* Check if we have a name record available. */
                                std::string strFrom;
                                if(Names::ReverseLookup(hashProof, strFrom))
                                    jFrom = { { "address", hashProof.ToString() }, { "name", strFrom } };

                                /* Add the from key address and name. */
                                jRet["from"]     = jFrom;
                            }
                        }
                    }
                    else
                        jRet["wildcard"] = true;

                    /* Build our from json key. */
                    encoding::json jAddress =
                        hashAddress.ToString();

                    /* Check if we have a name record available. */
                    std::string strAddress;
                    if(Names::ReverseLookup(hashAddress, strAddress))
                        jAddress = { { "address", hashAddress.ToString() }, { "name", strAddress } };

                    /* Add the from key address and name. */
                    jRet["to"]     = jAddress;

                    /* Add the amount to the response */
                    jRet["amount"]  = FormatBalance(nCredit, hashToken);
                    jRet["token"]   = hashToken.ToString();

                    /* Add a ticker if found. */
                    std::string strName;
                    if(Names::ReverseLookup(hashToken, strName))
                        jRet["ticker"] = strName;

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
                    jRet["OP"]      = "MIGRATE";
                    jRet["txid"]    = hashTx.ToString();
                    jRet["address"] = hashAccount.ToString();
                    jRet["amount"]  = FormatBalance(nAmount);

                    /* Check if we have a name record available. */
                    std::string strName;
                    if(Names::ReverseLookup(hashAccount, strName))
                        jRet["name"] = strName;

                    jRet["score"]   = nScore;
                    jRet["last"]    = hashLast.ToString();
                    jRet["token"]   = TOKEN::NXS.ToString();
                    jRet["ticker"]  = "NXS";

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
                    jRet["OP"]    = "AUTHORIZE";
                    jRet["txid"]  = hashTx.ToString();
                    jRet["proof"] = hashProof.ToString();

                    break;
                }

                /* A fee contract covers the computation expended, a debit essentially. */
                case TAO::Operation::OP::FEE:
                {
                    /* Get the address of the account the fee came from. */
                    TAO::Register::Address hashAccount;
                    contract >> hashAccount;

                    /* The fee amount. */
                    uint64_t nFee = 0;
                    contract >> nFee;

                    /* Output the json information. */
                    jRet["OP"]      = "FEE";

                    /* Build our from json key. */
                    encoding::json jFrom =
                        hashAccount.ToString();

                    /* Check if we have a name record available. */
                    std::string strFrom;
                    if(Names::ReverseLookup(hashAccount, strFrom))
                        jFrom = { { "address", hashAccount.ToString() }, { "name", strFrom } };

                    /* Add the from key address and name. */
                    jRet["from"]    = jFrom;
                    jRet["amount"]  = FormatBalance(nFee);
                    jRet["token"]   = TOKEN::NXS.ToString();
                    jRet["ticker"]  = "NXS";

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

                    /* Output the json information. */
                    jRet["OP"]       = "LEGACY";

                    /* Build our from json key. */
                    encoding::json jFrom =
                        hashFrom.ToString();

                    /* Check if we have a name record available. */
                    std::string strFrom;
                    if(Names::ReverseLookup(hashFrom, strFrom))
                        jFrom = { { "address", hashFrom.ToString() }, { "name", strFrom } };

                    /* Add the from key address and name. */
                    jRet["from"]    = jFrom;

                    /* Extract the receiving legacy address */
                    Legacy::NexusAddress hashLegacy;
                    Legacy::ExtractAddress(script, hashLegacy);

                    /* Add to our response JSON now. */
                    jRet["to"]      = hashLegacy.ToString();

                    /* Add the amount to the response */
                    jRet["amount"]  = FormatBalance(nAmount);
                    jRet["token"]   = TOKEN::NXS.ToString();
                    jRet["ticker"]  = "NXS";

                    break;
                }
            }
        }
        catch(const std::exception& e)
        {
            debug::error(FUNCTION, e.what());
        }

        /* Add our conditions now. */
        const std::string strConditions =
            ConditionToJSON(contract, nVerbose);

        /* Check that string isn't empty. */
        if(!strConditions.empty())
            jRet["conditions"] = strConditions;

        return jRet;
    }


    /* Converts a serialized contract condition stream to formattted JSON */
    std::string ConditionToJSON(const TAO::Operation::Contract& rContract, const uint32_t nVerbose)
    {
        /* Encode a string to output our bytecode. */
        std::string strCode;

        /* Loop through the operation validation code. */
        rContract.Reset(TAO::Operation::Contract::CONDITIONS);
        while(!rContract.End(TAO::Operation::Contract::CONDITIONS))
        {
            /* Grab the next operation. */
            uint8_t nOP = 0;
            rContract >= nOP;

            /* Switch based on our operation code now. */
            switch(nOP)
            {
                /* Handle for the ( operator. */
                case TAO::Operation::OP::GROUP:
                {
                    strCode += "(";
                    break;
                }

                /* Handle for the ) operator. */
                case TAO::Operation::OP::UNGROUP:
                {
                    strCode += ")";
                    break;
                }

                /* Handle for the && operator. */
                case TAO::Operation::OP::AND:
                {
                    strCode += " AND ";
                    break;
                }

                /* Handle for the || operator. */
                case TAO::Operation::OP::OR:
                {
                    strCode += " OR ";
                    break;
                }

                /* Handle for the == operator. */
                case TAO::Operation::OP::EQUALS:
                {
                    strCode += " == ";
                    break;
                }

                /* Handle for < operator. */
                case TAO::Operation::OP::LESSTHAN:
                {
                    strCode += " < ";
                    break;
                }

                /* Handle for the > operator. */
                case TAO::Operation::OP::GREATERTHAN:
                {
                    strCode += " > ";
                    break;
                }

                /* Handle for <= operator. */
                case TAO::Operation::OP::LESSEQUALS:
                {
                    strCode += " <= ";
                    break;
                }

                /* Handle for the >= operator. */
                case TAO::Operation::OP::GREATEREQUALS:
                {
                    strCode += " >= ";
                    break;
                }

                /* Handle for the != operator. */
                case TAO::Operation::OP::NOTEQUALS:
                {
                    strCode += " != ";
                    break;
                }

                /* Handle to check if a sequence of bytes is inside another. */
                case TAO::Operation::OP::CONTAINS:
                {
                    strCode += " ~= ";
                    break;
                }

                /* Add two 64-bit numbers. */
                case TAO::Operation::OP::ADD:
                {
                    strCode += " + ";
                    break;
                }

                /* Subtract one number from another. */
                case TAO::Operation::OP::SUB:
                {
                    strCode += " - ";
                    break;
                }

                /* Increment a number by an order of 1. */
                case TAO::Operation::OP::INC:
                {
                    strCode += "++ ";
                    break;
                }

                /* De-increment a number by an order of 1. */
                case TAO::Operation::OP::DEC:
                {
                    strCode += "-- ";
                    break;
                }

                /* Divide a number by another. */
                case TAO::Operation::OP::DIV:
                {
                    strCode += " / ";
                    break;
                }

                /* Multiply a number by another. */
                case TAO::Operation::OP::MUL:
                {
                    strCode += " * ";
                    break;
                }

                /* Raise a number by the power of another. */
                case TAO::Operation::OP::EXP:
                {
                    strCode += "^";
                    break;
                }

                /* Get the remainder after a division. */
                case TAO::Operation::OP::MOD:
                {
                    strCode += " % ";
                    break;
                }

                /* Parse out subdata from bytes. */
                case TAO::Operation::OP::SUBDATA:
                {
                    /* Get the beginning iterator. */
                    uint16_t nBegin = 0;
                    rContract >= nBegin;

                    /* Get the size to extract. */
                    uint16_t nSize = 0;
                    rContract >= nSize;

                    strCode += debug::safe_printstr(".subdata(", nBegin, ", ", nSize, ")");
                    break;
                }

                /* Parse out subdata from bytes. */
                case TAO::Operation::OP::CAT:
                {
                    strCode += " | ";
                    break;
                }

                /* Extract an uint8_t from the stream. */
                case TAO::Operation::OP::TYPES::UINT8_T:
                {
                    /* Extract the byte. */
                    uint8_t n = 0;
                    rContract >= n;

                    //strCode += debug::safe_printstr("uint8_t(", uint32_t(n), ")");
                    strCode += debug::safe_printstr(uint64_t(n));
                    break;
                }

                /* Extract an uint16_t from the stream. */
                case TAO::Operation::OP::TYPES::UINT16_T:
                {
                    /* Extract the byte. */
                    uint16_t n = 0;
                    rContract >= n;

                    //strCode += debug::safe_printstr("uint16_t(", n, ")");
                    strCode += debug::safe_printstr(uint64_t(n));
                    break;
                }

                /* Extract an uint32_t from the stream. */
                case TAO::Operation::OP::TYPES::UINT32_T:
                {
                    /* Extract the byte. */
                    uint32_t n = 0;
                    rContract >= n;

                    //strCode += debug::safe_printstr("uint32_t(", n, ")");
                    strCode += debug::safe_printstr(uint64_t(n));
                    break;
                }

                /* Extract an uint64_t from the stream. */
                case TAO::Operation::OP::TYPES::UINT64_T:
                {
                    /* Extract the byte. */
                    uint64_t n = 0;
                    rContract >= n;

                    //strCode += debug::safe_printstr("uint64_t(", n, ")");
                    strCode += debug::safe_printstr(uint64_t(n));
                    break;
                }

                /* Extract an uint256_t from the stream. */
                case TAO::Operation::OP::TYPES::UINT256_T:
                {
                    /* Extract the integer. */
                    uint256_t n = 0;
                    rContract >= n;

                    //strCode += debug::safe_printstr("uint256_t(", n.ToString(), ")");
                    strCode += debug::safe_printstr("0x", n.ToString());
                    break;
                }

                /* Extract an uint512_t from the stream. */
                case TAO::Operation::OP::TYPES::UINT512_T:
                {
                    /* Extract the integer. */
                    uint512_t n = 0;
                    rContract >= n;

                    //strCode += debug::safe_printstr("uint512_t(", n.ToString(), ")");
                    strCode += debug::safe_printstr("0x", n.ToString());
                    break;
                }

                /* Extract an uint1024_t from the stream. */
                case TAO::Operation::OP::TYPES::UINT1024_T:
                {
                    /* Extract the integer. */
                    uint1024_t n = 0;
                    rContract >= n;

                    //strCode += debug::safe_printstr("uint1024_t(", n.ToString(), ")");
                    strCode += debug::safe_printstr("0x", n.ToString());
                    break;
                }

                /* Extract a string from the stream. */
                case TAO::Operation::OP::TYPES::STRING:
                {
                    /* Extract the string. */
                    std::string str;
                    rContract >= str;

                    //strCode += debug::safe_printstr("string(", str, ")");
                    strCode += debug::safe_printstr("'", str, "'");
                    break;
                }

                /* Extract bytes from the stream. */
                case TAO::Operation::OP::TYPES::BYTES:
                {
                    /* Extract the string. */
                    std::vector<uint8_t> vData;
                    rContract >= vData;

                    strCode += debug::safe_printstr("data(", HexStr(vData.begin(), vData.end()), ")");
                    break;
                }

                /* Get a register's timestamp and push to the return value. */
                case TAO::Operation::OP::CALLER::PRESTATE::MODIFIED:
                {
                    strCode += "caller.prestate.modified";
                    break;
                }

                /* Get a register's timestamp and push to the return value. */
                case TAO::Operation::OP::REGISTER::MODIFIED:
                {
                    strCode += ".register.modified";
                    break;
                }

                /* Get a register's timestamp and push to the return value. */
                case TAO::Operation::OP::CALLER::PRESTATE::CREATED:
                {
                    strCode += "caller.prestate.created";
                    break;
                }

                /* Get the time register was created. */
                case TAO::Operation::OP::REGISTER::CREATED:
                {
                    strCode += ".register.created";
                    break;
                }

                /* The owner of given register pre-state. */
                case TAO::Operation::OP::CALLER::PRESTATE::OWNER:
                {
                    strCode += "caller.prestate.owner";
                    break;
                }

                /* The owner of given register. */
                case TAO::Operation::OP::REGISTER::OWNER:
                {
                    strCode += ".register.owner";
                    break;
                }

                /* The owner of given register pre-state. */
                case TAO::Operation::OP::CALLER::PRESTATE::STATE:
                {
                    strCode += "caller.prestate.state";
                    break;
                }

                /* The owner of given register. */
                case TAO::Operation::OP::REGISTER::STATE:
                {
                    strCode += ".register.state";
                    break;
                }

                /* The owner of given register pre-state. */
                case TAO::Operation::OP::CALLER::PRESTATE::VALUE:
                {
                    /* Get the value string. */
                    std::string strValue;
                    rContract >= strValue;

                    strCode += debug::safe_printstr("caller.prestate.value('", strValue, "')");
                    break;
                }

                /* The owner of given register. */
                case TAO::Operation::OP::REGISTER::VALUE:
                {
                    /* Get the value string. */
                    std::string strValue;
                    rContract >= strValue;

                    strCode += debug::safe_printstr("caller.register.value('", strValue, "')");
                    break;
                }

                /* Get the genesis-id of the calling sigchain. */
                case TAO::Operation::OP::CALLER::GENESIS:
                {
                    strCode += "caller.genesis";
                    break;
                }

                /* Get the timestamp of the transaction caller. */
                case TAO::Operation::OP::CALLER::TIMESTAMP:
                {
                    strCode += "caller.timestamp";
                    break;
                }

                /* Get the genesis-id of the calling sigchain. */
                case TAO::Operation::OP::CONTRACT::GENESIS:
                {
                    strCode += "contract.genesis";
                    break;
                }

                /* Get the timestamp of the transaction caller. */
                case TAO::Operation::OP::CONTRACT::TIMESTAMP:
                {
                    strCode += "contract.timestamp";
                    break;
                }

                /* Get the operations of the transaction caller. */
                case TAO::Operation::OP::CONTRACT::OPERATIONS:
                {
                    strCode += "contract.operations";
                    break;
                }

                /* Get the operations of the transaction caller. */
                case TAO::Operation::OP::CALLER::OPERATIONS:
                {
                    strCode += "caller.operations";
                    break;
                }

                /* Get the current height of the chain. */
                case TAO::Operation::OP::LEDGER::HEIGHT:
                {
                    strCode += "ledger.height";
                    break;
                }

                /* Get the current supply of the chain. */
                case TAO::Operation::OP::LEDGER::SUPPLY:
                {
                    strCode += "ledger.supply";
                    break;
                }

                /* Get the best block timestamp. */
                case TAO::Operation::OP::LEDGER::TIMESTAMP:
                {
                    strCode += "ledger.timestamp";
                    break;
                }

                /* Compute an SK256 hash of current return value. */
                case TAO::Operation::OP::CRYPTO::SK256:
                {
                    strCode += ".crypto.sk256";
                    break;
                }

                /* Compute an SK512 hash of current return value. */
                case TAO::Operation::OP::CRYPTO::SK512:
                {
                    strCode += ".crypto.sk512";
                    break;
                }
            }
        }

        return strCode;
    }


    /* Converts an Register to formattted JSON */
    encoding::json RegisterToJSON(const TAO::Register::Object& rObject, const TAO::Register::Address& hashRegister)
    {
        /* Add the register owner */
        encoding::json jRet;
        jRet["owner"]    = rObject.hashOwner.ToString();
        jRet["version"]  = rObject.nVersion;
        jRet["created"]  = rObject.nCreated;
        jRet["modified"] = rObject.nModified;
        jRet["type"]     = GetRegisterName(rObject.nType);

        /* Handle if register isn't an object. */
        if(rObject.nType != TAO::Register::REGISTER::OBJECT)
        {
            /* Grab a reference of our state.*/
            const std::vector<uint8_t>& vState = rObject.GetState();

            /* Add our state information. */
            StateToJSON(vState, jRet);

            /* Otherwise output the address if supplied. */
            if(hashRegister != 0)
            {
                /* Add the address field into register. */
                jRet["address"] = hashRegister.ToString();

                /* Check for reverse ptr record if not token (that resolves a ticker). */
                std::string strName = "";
                if(Names::ReverseLookup(hashRegister, strName))
                    jRet["name"] = strName;
            }

            return jRet;
        }

        /* Add our data members to json response. */
        MembersToJSON(rObject, jRet);

        /* Generate address if not specified. */
        if(hashRegister == 0)
        {
            /* Check some standard types to see if we want to add additional data. */
            const uint8_t nStandard = rObject.Standard();
            switch(nStandard)
            {
                /* Special handle for crypto object register. */
                case TAO::Register::OBJECTS::CRYPTO:
                {
                    /* The register address */
                    const TAO::Register::Address hashAddress =
                        TAO::Register::Address("crypto", rObject.hashOwner, TAO::Register::Address::CRYPTO);

                    /* Populate stake related information. */
                    jRet["address"] = hashAddress.ToString();

                    break;
                }

                /* Special handle for trust object register. */
                case TAO::Register::OBJECTS::TRUST:
                {
                    /* The register address */
                    const TAO::Register::Address hashAddress =
                        TAO::Register::Address("trust", rObject.hashOwner, TAO::Register::Address::TRUST);

                    /* Populate stake related information. */
                    jRet["address"] = hashAddress.ToString();

                    break;
                }

                /* Special handle for names and namespaces. */
                case TAO::Register::OBJECTS::NAME:
                case TAO::Register::OBJECTS::NAMESPACE:
                {
                    /* Start with default namespace generated by user-id. */
                    TAO::Register::Address hashNamespace = rObject.hashOwner;

                    /* Check if object contains a namespace. */
                    const std::string strNamespace = rObject.get<std::string>("namespace");
                    if(!strNamespace.empty())
                    {
                        /* Add our global flag to tell if it's global name. */
                        jRet["global"]    = (strNamespace == TAO::Register::NAMESPACE::GLOBAL);

                        /* Add our namespace hash if not global or local. */
                        if(!jRet["global"])
                            hashNamespace = TAO::Register::Address(strNamespace, TAO::Register::Address::NAMESPACE);
                    }

                    /* Generate namespace object's address. */
                    if(nStandard == TAO::Register::OBJECTS::NAMESPACE)
                        jRet["address"] = hashNamespace.ToString();

                    /* Generate name object's address. */
                    if(nStandard == TAO::Register::OBJECTS::NAME)
                        jRet["address"] = TAO::Register::Address(rObject.get<std::string>("name"), hashNamespace, TAO::Register::Address::NAME).ToString();

                    break;
                }
            }
        }

        /* Otherwise output the address if supplied. */
        else
        {
            /* Add address from input parameters. */
            jRet["address"] = hashRegister.ToString();

            /* Check for reverse ptr record if not token (that resolves a ticker). */
            std::string strName = "";
            if(!hashRegister.IsToken() && Names::ReverseLookup(hashRegister, strName))
                jRet["name"] = strName;
        }

        return jRet;
    }


    /* Converts an Object Register's data members to formattted JSON with no external lookups */
    void MembersToJSON(const TAO::Register::Object& rObject, encoding::json &jRet)
    {
        /* Check for valid object types. */
        if(rObject.nType != TAO::Register::REGISTER::OBJECT)
            return;

        /* Declare type and data variables for unpacking the Object fields */
        const std::vector<std::string> vMembers = rObject.Members();
        for(const auto& strMember : vMembers)
        {
            /* First get the type */
            uint8_t nType = 0;
            rObject.Type(strMember, nType);

            /* Switch based on type. */
            switch(nType)
            {
                /* Check for uint8_t type. */
                case TAO::Register::TYPES::UINT8_T:
                {
                    /* Set the return value from object register data. */
                    jRet[strMember] = rObject.get<uint8_t>(strMember);

                    break;
                }

                /* Check for uint16_t type. */
                case TAO::Register::TYPES::UINT16_T:
                {
                    /* Skip over our system usertype. */
                    if(strMember == "_usertype")
                        break;

                    /* Set the return value from object register data. */
                    jRet[strMember] = rObject.get<uint16_t>(strMember);

                    break;
                }

                /* Check for uint32_t type. */
                case TAO::Register::TYPES::UINT32_T:
                {
                    /* Set the return value from object register data. */
                    jRet[strMember] = rObject.get<uint32_t>(strMember);

                    break;
                }

                /* Check for uint64_t type. */
                case TAO::Register::TYPES::UINT64_T:
                {
                    /* Grab a copy of our object register value. */
                    const uint64_t nValue = rObject.get<uint64_t>(strMember);

                    /* Specific rule for balance. */
                    if(strMember == "balance" || strMember == "stake" || strMember == "supply")
                    {
                        /* Handle decimals conversion. */
                        const uint8_t nDecimals = GetDecimals(rObject);

                        /* Special rule for supply. */
                        if(strMember == "supply")
                        {
                            /* Find our current supply. */
                            const uint64_t nCurrent = (rObject.get<uint64_t>("supply") - rObject.get<uint64_t>("balance"));

                            /* Populate json values. */
                            jRet["currentsupply"] = FormatBalance(nCurrent, nDecimals);
                            jRet["maxsupply"]     = FormatBalance(rObject.get<uint64_t>("supply"), nDecimals);
                        }
                        else
                            jRet[strMember] = FormatBalance(nValue, nDecimals);
                    }
                    else if(strMember == "trust")
                    {
                        /* Get our total counters. */
                        uint32_t nDays = 0, nHours = 0, nMinutes = 0;
                        convert::i64todays(nValue / 60, nDays, nHours, nMinutes);

                        /* Add string to trust. */
                        jRet["trust"]  = nValue;
                        jRet["age"]    = debug::safe_printstr(nDays, " days, ", nHours, " hours, ", nMinutes, " minutes");
                        jRet["rate"]   = FormatStakeRate(nValue);
                    }

                    /* Set the return value from object register data. */
                    else
                        jRet[strMember] = nValue;

                    break;
                }

                /* Check for uint256_t type. */
                case TAO::Register::TYPES::UINT256_T:
                {
                    /* Grab a copy of our hash to check. */
                    const uint256_t hash = rObject.get<uint256_t>(strMember);

                    /* Specific rule for token ticker. */
                    if(strMember == "token")
                    {
                        /* Encode token in Base58 Encoding. */
                        jRet["token"] = TAO::Register::Address(hash).ToString();

                        /* Add a ticker if found. */
                        std::string strMember;
                        if(Names::ReverseLookup(hash, strMember))
                            jRet["ticker"] = strMember;
                    }

                    /* Specific rule for register address. */
                    else if(strMember == "address")
                        jRet["register"] = TAO::Register::Address(hash).ToString();

                    /* Set the return value from object register data. */
                    else
                        jRet[strMember] = hash.GetHex();

                    break;
                }

                /* Check for uint512_t type. */
                case TAO::Register::TYPES::UINT512_T:
                {
                    /* Set the return value from object register data. */
                    jRet[strMember] = rObject.get<uint512_t>(strMember).GetHex();

                    break;
                }

                /* Check for uint1024_t type. */
                case TAO::Register::TYPES::UINT1024_T:
                {
                    /* Set the return value from object register data. */
                    jRet[strMember] = rObject.get<uint1024_t>(strMember).GetHex();

                    break;
                }

                /* Check for string type. */
                case TAO::Register::TYPES::STRING:
                {
                    /* Remove trailing nulls from the data, which are padding to maxlength on mutable fields */
                    const std::string strRet = rObject.get<std::string>(strMember);

                    /* Check for json encoding. */
                    if(encoding::json::accept(strRet))
                    {
                        jRet[strMember] = encoding::json::parse(strRet);
                        break;
                    }

                    /* Set the return value from object register data. */
                    jRet[strMember] = strRet.substr(0, strRet.find_last_not_of('\0') + 1);

                    break;
                }

                /* Check for bytes type. */
                case TAO::Register::TYPES::BYTES:
                {
                    /* Set the return value from object register data. */
                    std::vector<uint8_t> vRet = rObject.get<std::vector<uint8_t>>(strMember);

                    /* Remove trailing nulls from the data, which are padding to maxlength on mutable fields */
                    vRet.erase(std::find(vRet.begin(), vRet.end(), '\0'), vRet.end());

                    /* Add to return value in base64 encoding. */
                    jRet[strMember] = encoding::EncodeBase64(&vRet[0], vRet.size()) ;

                    break;
                }
            }
        }
    }


    /* Converts an Register's state into formattted JSON with no external lookups */
    void StateToJSON(const std::vector<uint8_t>& vState, encoding::json &jRet)
    {
        /* Reset our read position. */
        TAO::Register::Stream ssObject = TAO::Register::Stream(vState);

        /* Get our state type. */
        uint16_t nType;
        ssObject >> nType;

        /* Check for a valid known user-type. */
        if(USER_TYPES::Valid(nType))
        {
            /* Now get our state string. */
            std::string strResults;
            ssObject >> strResults;

            /* Let's check if our first few bytes are valid. */
            if(encoding::utf8::is_valid(strResults.begin(), strResults.end()))
            {
                /* Check if we can format as JSON. */
                if(encoding::json::accept(strResults))
                {
                    jRet["json"] = encoding::json::parse(strResults);
                    return;
                }

                /* Otherwise we format by null terminated string. */
                jRet["data"] = strResults.substr(0, strResults.find_last_not_of('\0') + 1);
                return;
            }
        }
        else
        {
            /* Let's check if our first few bytes are valid. */
            if(encoding::utf8::is_valid(vState.begin(), vState.end()))
            {
                /* Copy our results into a utf-8 encoded string. */
                const std::string strResults = std::string(vState.begin(), vState.end());

                /* Check if we can format as JSON. */
                if(encoding::json::accept(strResults))
                {
                    jRet["json"] = encoding::json::parse(strResults);
                    return;
                }

                /* Otherwise we format by null terminated string. */
                jRet["data"] = strResults.substr(0, strResults.find_last_not_of('\0') + 1);
                return;
            }
        }

        /* Fall through resort to regular hex string. */
        jRet["data"] = HexStr(vState.begin(), vState.end());
    }


    /* Converts an Register's state into unformatted string with no external lookups */
    std::string StateToJSON(const std::vector<uint8_t>& vState)
    {
        /* Wrap this call around previous overload. */
        encoding::json jRet;
        StateToJSON(vState, jRet);

        /* Check for json encoding. */
        if(jRet.find("json") != jRet.end())
            return jRet["json"].dump(-1);

        /* Check for data encoding. */
        if(jRet.find("data") != jRet.end())
            return jRet["data"].get<std::string>();

        /* Otherwise fall back to hex encoding. */
        return HexStr(vState.begin(), vState.end());
    }


    /* Generate a json object with block channel related data. */
    encoding::json ChannelToJSON(const uint32_t nChannel)
    {
        /* Build our channel response object. */
        encoding::json jRet =
        {
            { "height",     0 },
            { "weight",     0 },
            { "timespan",   0 },
            { "fees",       0 },
            { "difficulty", 0 },
        };

        /* Get our last block state. */
        TAO::Ledger::BlockState tBlock = TAO::Ledger::ChainState::stateBest.load();
        if(TAO::Ledger::GetLastState(tBlock, nChannel))
        {
            /* Populate our heights and weights. */
            jRet["height"] = tBlock.nChannelHeight;
            jRet["weight"] = tBlock.nChannelWeight[nChannel].GetHex();

            /* Get our average timespan. */
            const uint64_t nTimespan =
                TAO::Ledger::GetAverageTimespan(tBlock, 1440);

            /* Set return data. */
            jRet["timespan"] = nTimespan;

            /* Handle for mined blocks. */
            if(nChannel != 0)
            {
                /* Populate our reserves and rewards. */
                jRet["reserve"] = FormatBalance(tBlock.nReleasedReserve[0]
                                              + tBlock.nReleasedReserve[1]
                                              + tBlock.nReleasedReserve[2]);

                /* Check for coinbase rewards. */
                jRet["reward"]  = FormatBalance(TAO::Ledger::GetCoinbaseReward(tBlock, nChannel, 0));

                /* Calculate our average prime rate. */
                if(nChannel == 1)
                {
                    /* Calculate our prime rate based on constants and average times. */
                    const uint64_t nRate = (2480 / nTimespan) //2480 is difficulty constant XXX: re-check constant
                        * std::pow(50.0, TAO::Ledger::GetDifficulty(tBlock.nBits, nChannel) - 3.0);

                    /* Add to return values. */
                    jRet["primes"] = nRate;
                }

                /* Calculate our average hash rate. */
                if(nChannel == 2)
                {
                    /* Calculate our prime rate based on constants and average times. */
                    const uint64_t nRate = (276758250000 / nTimespan) //276758250000 is difficulty constant XXX: re-check constant
                        * TAO::Ledger::GetDifficulty(tBlock.nBits, nChannel);

                    /* Add to return values. */
                    jRet["hashes"] = nRate;
                }
            }

            /* Include our fee reserves. */
            jRet["fees"]        = FormatBalance(tBlock.nFeeReserve);

            /* Include our difficulty values. */
            jRet["difficulty"]  = TAO::Ledger::GetDifficulty(tBlock.nBits, nChannel);
        }

        return jRet;
    }


    /* Encodes the object based on the given command-set standards. */
    encoding::json StandardToJSON(const encoding::json& jParams, const TAO::Register::Object& rObject, const TAO::Register::Address& hashRegister)
    {
        /* Check for our request parameters first, since this method can be called without */
        if(!CheckRequest(jParams, "type", "string, array"))
            throw Exception(-28, "Missing parameter [request::type] for command");

        /* Check that we have the commands set. */
        const Base* pBase = Commands::Instance(jParams["request"]["commands"].get<std::string>());
        if(!pBase)
            throw Exception(-28, "Missing parameter [request::commands] for command");

        /* Check for array to iterate. */
        const encoding::json jTypes = jParams["request"]["type"];
        if(jTypes.is_array())
        {
            /* Loop through standards. */
            for(const auto& jCheck : jTypes)
            {
                /* Check if at least one standard is correct. */
                if(pBase->CheckObject(jCheck.get<std::string>(), rObject))
                    return pBase->EncodeObject(jCheck.get<std::string>(), rObject, hashRegister);
            }
        }

        /* We only fail here, as we want to isolate returns based on the standards, not parameters. */
        return pBase->EncodeObject(jParams["request"]["type"].get<std::string>(), rObject, hashRegister);
    }


    /* Recursive helper function for QueryToJSON to recursively generate JSON statements for use with filters. */
    encoding::json StatementToJSON(std::vector<std::string> &vWhere, uint32_t &nIndex, encoding::json &jStatement)
    {
        /* Check if we have consumed all of our clauses. */
        if(nIndex >= vWhere.size())
            return jStatement;

        /* Grab a reference of our working string. */
        std::string& strClause = vWhere[nIndex];

        /* Check for logical statement. */
        if(strClause == "AND" || strClause == "OR")
        {
            /* Check for incorrect mixing of AND/OR. */
            if(jStatement.find("logical") == jStatement.end())
                throw Exception(-122, "Query Syntax Error: missing logical operator for group");

            /* Grab a copy of our current logical statement. */
            const std::string strLogical = jStatement["logical"].get<std::string>();
            if(strLogical != "NONE" && strLogical != strClause)
                throw Exception(-121, "Query Syntax Error, must use '(' and ')' to mix AND/OR statements");

            jStatement["logical"] = strClause;
            return StatementToJSON(vWhere, ++nIndex, jStatement);
        }

        /* Check if we have extra spaces in the statement. */
        while(nIndex + 1 < vWhere.size())
        {
            /* Grab a reference of our current statement in query. */
            const std::string& strCheck = vWhere[nIndex + 1];

            /* Check if we can exit now with complete statement. */
            if(strCheck == "AND" || strCheck == "OR")
                break;

            strClause += (std::string(" ") + strCheck);
            vWhere.erase(vWhere.begin() + nIndex + 1); //to clear up iterations for next statement, to ensure no re-use of data
        }

        /* Check if we are recursing up a level. */
        const auto nLeft = strClause.find("(");
        if(nLeft == 0)
        {
            /* Parse out substring removing paranthesis. */
            strClause = strClause.substr(nLeft + 1);

            /* Create a new group to recurse up a level. */
            encoding::json jGroup
            {
                { "logical", "NONE" },
                { "statement", encoding::json::array() },
            };

            /* We want to push this new group recursive field to current level. */
            jStatement["statement"].push_back(StatementToJSON(vWhere, nIndex, jGroup));

            /* Now we continue consuming our clauses to complete the statement. */
            return StatementToJSON(vWhere, ++nIndex, jStatement);
        }

        /* Check if we are recursing back down a level. */
        const auto nRight = strClause.rfind(")");
        if(nRight == strClause.length() - 1)
        {
            /* Parse out substring removing paranthesis. */
            strClause = strClause.substr(0, nRight);

            /* Check if we need to recurse another level still. */
            if(strClause.rfind(")") == strClause.length() - 1)
                return StatementToJSON(vWhere, nIndex, jStatement);

            /* Add our clause to end of statement. */
            jStatement["statement"].push_back(ClauseToJSON(strClause));

            return jStatement;
        }

        /* Regular statement adding clause. */
        jStatement["statement"].push_back(ClauseToJSON(strClause));

        /* Regular recursion to move to next statement. */
        return StatementToJSON(vWhere, ++nIndex, jStatement);
    }


    /* Turns a where query string in url encoding into a formatted JSON object. */
    encoding::json QueryToJSON(const std::string& strWhere)
    {
        /* Split up our query by spaces. */
        std::vector<std::string> vWhere;
        ParseString(strWhere, ' ', vWhere);

        /* Build our return object. */
        encoding::json jRet
        {
            { "logical", "NONE" },
            { "statement", encoding::json::array() },
        };

        /* Recursively process the query. */
        uint32_t n = 0;
        jRet = StatementToJSON(vWhere, n, jRet);

        /* Check for logical operator. */
        //if(jRet["logical"].get<std::string>() == "NONE" && jRet["statement"].size() > 1)
        //    throw Exception(-120, "Query Syntax Error: missing logical operator for base group, too many '()' maybe?");

        return jRet;
    }


    /** VariableToJSON
     *
     *  Converts a query variable into a json string.
     *
     *  Varibles needs to be modular functional statements with return type specifications.
     *  This function is hard coded variables for now, need to make it modular.
     */
    std::string VariableToJSON(const std::string& strValue)
    {
        /* Find where parameters start. */
        const auto nBegin = strValue.find('(');
        if(nBegin == strValue.npos)
            throw Exception(-120, "Query Syntax Error: variable format must be variable(`params`);", strValue);

        /* Parse out our variable name. */
        const std::string strVariable =
            strValue.substr(0, nBegin);

        /* Find our ending iterator. */
        const auto nEnd = strValue.find(')');
        if(nEnd == strValue.npos)
            throw Exception(-120, "Query Syntax Error: variable format must be variable(`params`);", strValue);

        /* Get our parameter values. */
        const std::string strParams =
            strValue.substr(nBegin + 1, nEnd - nBegin - 1);

        /* Date variable requires a string argument. */
        const auto nOpen = strParams.find('`');
        if(nOpen == strParams.npos)
            throw Exception(-120, "Query Syntax Error: variable format requires string ", strVariable, "(`params`); | ", strValue);

        /* Check for closing string. */
        const auto nClose = strParams.rfind('`');
        if(nClose == strParams.npos)
            throw Exception(-120, "Query Syntax Error: variable format requires string ", strVariable, "(`params`); | ", strValue);

        /* Check we have both open and close. */
        if(nOpen == nClose)
            throw Exception(-120, "Query Syntax Error: variable string must close ", strVariable, "(`params`); | ", strValue);

        /* Now get our parameter values. */
        const std::string strParam =
            strParams.substr(nOpen + 1, nClose - 1);

        /* Handle for date variable types. */
        if(strVariable == "date")
        {
            /* Build our time struct from strptime. */
            struct tm tm;
            if(!runtime::strptime(strParam.c_str(), tm))
                throw Exception(-121, "Query Syntax Error: date format must include year ex. date(`2021`);");

            /* Build a simple return string. */
            return debug::safe_printstr(std::mktime(&tm));
        }

        /* Handle for the name variable types. */
        if(strVariable == "name")
        {
            /* Build our address from base58. */
            const TAO::Register::Address hashAddress =
                TAO::Register::Address(strParam);

            /* Check for valid tritium address. */
            if(!hashAddress.IsValid())
                throw Exception(-121, "Query Syntax Error: name reverse lookup invalid address");

            /* Check for a valid reverse lookup entry. */
            std::string strName;
            if(!Names::ReverseLookup(hashAddress, strName))
                throw Exception(-121, "Query Syntax Error: name reverse lookup entry not found");

            return strName;
        }

        /* Handle for address resolver. */
        if(strVariable == "resolve")
        {
            /* Temporary value to pass. */
            encoding::json jParams;

            /* Build our address from name record. */
            return Names::ResolveAddress(jParams, strParam, true).ToString();
        }

        /* Handle for username resolver. */
        if(strVariable == "username")
        {
            /* Get our genesis from username. */
            const uint256_t hashGenesis =
                TAO::Ledger::SignatureChain::Genesis(SecureString(strParam.c_str()));

            return hashGenesis.ToString();
        }

        return strValue;
    }


    /* Turns a where clause string in url encoding into a formatted JSON object. */
    encoding::json ClauseToJSON(const std::string& strClause)
    {
        /* Check for a set to compare. */
        const auto nBegin = strClause.find_first_of("!=<>");
        if(nBegin == strClause.npos)
            throw Exception(-120, "Query Syntax Error: must use <key>=<value> with no extra characters. ", strClause);

        /* Grab our current key. */
        const std::string strKey = strClause.substr(0, nBegin);

        /* Check for our incoming parameter. */
        const std::string::size_type nDot = strKey.find('.');
        if(nDot == strKey.npos)
            throw Exception(-60, "Query Syntax Error: malformed where clause at ", strKey);

        /* Build our current json value. */
        encoding::json jClause =
        {
            { "class", strKey.substr(0, nDot)  },
            { "field", strKey.substr(nDot + 1) },
        };

        /* Check if its < or =. */
        const auto nEnd = strClause.find_first_of("!=<>", nBegin + 1);
        if(nEnd != strClause.npos)
        {
            jClause["operator"] = strClause[nBegin] + std::string("") + strClause[nEnd];
            jClause["value"]    = strClause.substr(nBegin + 2);
        }

        /* It's just < if we reach here. */
        else
        {
            jClause["operator"] = strClause[nBegin] + std::string("");
            jClause["value"]    = strClause.substr(nBegin + 1);
        }

        /* Check the clause for any variables. */
        const std::string strValue = jClause["value"].get<std::string>();
        if(strValue.find(';') != strValue.npos)
            jClause["value"] = VariableToJSON(strValue);

        /* Check for valid values and parameters. */
        if(jClause["value"].get<std::string>().empty())
            throw Exception(-120, "Query Syntax Error: must use <key>=<value> with no extra characters.");

        return jClause;
    }


    /* Turns a query string in url encoding into a formatted JSON object. */
    encoding::json ParamsToJSON(const std::vector<std::string>& vParams)
    {
        /* Track our where clause. */
        std::string strWhere;

        /* Get the parameters. */
        encoding::json jRet;
        for(uint32_t n = 0; n < vParams.size(); ++n)
        {
            /* Set et as reference of our parameter. */
            const std::string& strParam = vParams[n];

            /* Find the key/value delimiter. */
            const auto nPos = strParam.find("=");
            if(nPos != strParam.npos || strParam == "WHERE")
            {
                /* Check for key/value pairs. */
                std::string strKey, strValue;
                if(nPos != strParam.npos)
                {
                    /* Grab the key and value substrings. */
                    strKey   = strParam.substr(0, nPos);
                    strValue = strParam.substr(nPos + 1);

                    /* Check for valid parameter. */
                    while(n < vParams.size() - 1 && vParams[n + 1].find("=") == vParams[n + 1].npos)
                        strValue += " " + vParams[++n];

                    /* Check for empty value, due to ' ' or bad input. */
                    //if(strValue.empty())
                    //    throw Exception(-58, "Empty Parameter [", strKey, "]");
                }

                /* Check for where clauses. */
                if(strParam == "WHERE" || strKey == "where")
                {
                    /* Check if we have operated before. */
                    if(!strWhere.empty()) //check for double WHERE
                        throw Exception(-60, "Query Syntax Error: malformed where clause at ", strValue);

                    /* If where as key/value, append the value we parsed out. */
                    if(strKey == "where")
                    {
                        /* Check that our where clause is a proper conditional statement. */
                        if(strValue.find_first_of("!=<>") == strValue.npos)
                            throw Exception(-60, "Query Syntax Error: malformed where clause at ", strValue);

                        strWhere += std::string(strValue);
                    }

                    /* Check if we have empty WHERE. */
                    if(n + 1 >= vParams.size() && strParam == "WHERE")
                        throw Exception(7, "Query Syntax Error: empty WHERE clause at [", strValue, "]");

                    /* Build a single where string for parsing. */
                    for(uint32_t i = n + 1; i < vParams.size(); ++i)
                    {
                        /* Check if we have operated before. */
                        if(vParams[i] == "WHERE" && strKey == "where") //check for double WHERE
                            throw Exception(-60, "Query Syntax Error: malformed where clause at ", strValue);

                        /* Append the string with remaining arguments. */
                        strWhere += vParams[i];

                        /* Add a space as delimiter. */
                        if(i < vParams.size() - 1)
                            strWhere += std::string(" ");
                    }

                    break;
                }

                /* Add to our return json. */
                else
                {
                    /* Parse if the parameter is a json object or array. */
                    if(encoding::json::accept(strValue))
                    {
                        jRet[strKey] = encoding::json::parse(strValue);
                        continue;
                    }

                    /* Otherwise just copy string argument over. */
                    jRet[strKey] = strValue;
                }
            }
            else
                throw Exception(-120, "Query Syntax Error: must use <key>=<value> with no extra characters.");
        }

        /* Build our query string now. */
        if(!strWhere.empty())
            jRet["where"] = strWhere;

        return jRet;
    }
}
