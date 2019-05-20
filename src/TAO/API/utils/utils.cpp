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

#include <TAO/Operation/include/stream.h>
#include <TAO/Operation/include/enum.h>
#include <TAO/Operation/include/operations.h>

#include <Legacy/include/evaluate.h>

#include <TAO/API/include/users.h>

#include <Util/include/args.h>
#include <Util/include/hex.h>
#include <Util/include/json.h>

#include <Util/include/base64.h>


/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        /* Generates a lightweight argon2 hash of the namespace string.*/
        uint256_t NamespaceHash(const SecureString& strNamespace)
        {
            /* Generate the Secret Phrase */
            std::vector<uint8_t> vNamespace(strNamespace.begin(), strNamespace.end());

            // low-level API
            std::vector<uint8_t> vHash(32);
            std::vector<uint8_t> vSalt(16);

            /* Create the hash context. */
            argon2_context context =
            {
                /* Hash Return Value. */
                &vHash[0],
                32,

                /* Password input data. */
                &vNamespace[0],
                static_cast<uint32_t>(vNamespace.size()),

                /* The salt for usernames */
                &vSalt[0],
                static_cast<uint32_t>(vSalt.size()),

                /* Optional secret data */
                NULL, 0,

                /* Optional associated data */
                NULL, 0,

                /* Computational Cost. */
                10,

                /* Memory Cost (4 MB). */
                (1 << 12),

                /* The number of threads and lanes */
                1, 1,

                /* Algorithm Version */
                ARGON2_VERSION_13,

                /* Custom memory allocation / deallocation functions. */
                NULL, NULL,

                /* By default only internal memory is cleared (pwd is not wiped) */
                ARGON2_DEFAULT_FLAGS
            };

            /* Run the argon2 computation. */
            int32_t nRet = argon2id_ctx(&context);
            if(nRet != ARGON2_OK)
                throw std::runtime_error(debug::safe_printstr(FUNCTION, "Argon2 failed with code ", nRet));

            /* Set the bytes for the key. */
            uint256_t hashKey;
            hashKey.SetBytes(vHash);

            return hashKey;
        }


        /* Resolves a register address from a name.
        *  The register address is a hash of the fully-namespaced name in the format of namespacehash:objecttype:name. */
        uint256_t RegisterAddressFromName(const json::json& params, const std::string& strObjectType, const std::string& strObjectName )
        {
            uint256_t hashRegister = 0;

            /* In order to resolve an object name to a register address we also need to know the namespace.
            *  This must either be provided by the caller explicitly in a namespace parameter or by passing
            *  the name in the format namespace:name.  However since the default namespace is the username
            *  of the sig chain that created the object, if no namespace is explicitly provided we will
            *  also try using the username of currently logged in sig chain */
            std::string strName = strObjectName;
            std::string strNamespace = "";

            /* First check to see if the name parameter has been provided in the namespace:name format*/
            size_t nPos = strName.find(":");
            if(nPos != std::string::npos)
            {
                strNamespace = strName.substr(0, nPos);
                strName = strName.substr(nPos+1);
            }

            /* If not then check for the explicit namespace parameter*/
            else if(params.find("namespace") != params.end())
            {
                strNamespace = params["namespace"].get<std::string>();
            }

            /* If neither of those then check to see if there is an active session to access the sig chain */
            else
            {
                /* Get the session to be used for this API call.  Note we pass in false for fThrow here so that we can
                   throw a missing namespace exception if no valid session could be found */
                uint64_t nSession = users.GetSession(params, false);

                /* Get the account. */
                memory::encrypted_ptr<TAO::Ledger::SignatureChain>& user = users.GetAccount(nSession);
                if(!user)
                    throw APIException(-23, "Missing namespace parameter");

                strNamespace = user->UserName().c_str();
            }

            /* Get the namespace hash to use for this object. */
            uint256_t nNamespaceHash = NamespaceHash(SecureString(strNamespace.c_str()));

            /* register address is a hash of a name in the format of namespacehash:objecttype:name */
            std::string strRegisterName = nNamespaceHash.ToString() + ":" + strObjectType + ":" + strName;

            /* Build the address from an SK256 hash of register name. */
            hashRegister = LLC::SK256(std::vector<uint8_t>(strRegisterName.begin(), strRegisterName.end()));

            return hashRegister;
        }


        /*  Determins whether a string value is a register address.
         *  This only checks to see if the value is 64 characters in length and all hex characters (i.e. can be converted to a uint256).
         *  It does not check to see whether the register address exists in the database
         */
        bool IsRegisterAddress(const std::string& strValueToCheck)
        {
            return strValueToCheck.length() == 64 && strValueToCheck.find_first_not_of("0123456789abcdefABCDEF", 0) == std::string::npos;
        }


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

                txdata["operation"]  = OperationToJSON(tx.ssOperation);
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
                            ret["OP"]      = "WRITE";
                            ret["address"] = hashAddress.ToString();
                            ret["data"]    = HexStr(vchData.begin(), vchData.end());

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
                            ret["OP"]      = "APPEND";
                            ret["address"] = hashAddress.ToString();
                            ret["data"]    = HexStr(vchData.begin(), vchData.end());

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
                            ret["OP"]      = "REGISTER";
                            ret["address"] = hashAddress.ToString();
                            ret["type"]    = nType;
                            ret["data"]    = HexStr(vchData.begin(), vchData.end());


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
                            ret["OP"]       = "TRANSFER";
                            ret["address"]  = hashAddress.ToString();
                            ret["transfer"] = hashTransfer.ToString();

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
                            ret["OP"]       = "DEBIT";
                            ret["address"]  = hashAddress.ToString();
                            ret["transfer"] = hashTransfer.ToString();
                            ret["amount"]   = nAmount;

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

                            /* The total to be credited. */
                            uint64_t  nCredit;
                            ssOperation >> nCredit;

                            /* Output the json information. */
                            ret["OP"]      = "CREDIT";
                            ret["txid"]    = hashTx.ToString();
                            ret["proof"]   = hashProof.ToString();
                            ret["account"] = hashAccount.ToString();
                            ret["amount"]  = nCredit;

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
                            ret["OP"]     = "COINBASE";
                            ret["nonce"]  = nExtraNonce;
                            ret["amount"] = nCredit;

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
                            ret["OP"]       = "TRUST";
                            ret["account"]  = hashAccount.ToString();
                            ret["last"]     = hashLastTrust.ToString();
                            ret["sequence"] = nSequence;
                            ret["trust"]    = nTrust;
                            ret["amount"]   = nStake;

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
                            ret["OP"]    = "AUTHORIZE";
                            ret["txid"]  = hashTx.ToString();
                            ret["proof"] = hashProof.ToString();

                            break;
                        }
                    }
                }
            }
            catch(const std::runtime_error& e)
            {

            }

            return ret;
        }


        /* Converts an Object Register to formattted JSON */
        json::json ObjectRegisterToJSON(const TAO::Register::Object object, const std::string strDataField)
        {
            /* Declare the return JSON object */
            json::json ret;

            if(object.nType == TAO::Register::REGISTER::RAW)
            {
                /* Build the response JSON for the raw format asset. */
                ret["timestamp"]  = object.nTimestamp;
                ret["owner"]      = object.hashOwner.ToString();

                /* raw state assets only have one data member containing the raw hex-encoded data*/
                std::string data;
                object >> data;
                ret["data"] = data;
            }
            else if(object.nType == TAO::Register::REGISTER::OBJECT)
            {

                /* Build the response JSON. */
                if(strDataField.empty())
                {
                    ret["timestamp"]  = object.nTimestamp;
                    ret["owner"]      = object.hashOwner.ToString();
                }

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
                    /* Only return requested data field if one was specifically requested */
                    if(!strDataField.empty() && strDataField != strFieldName)
                        continue;

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
            else
                throw APIException(-24, "Specified name/address is not an asset.");

            return ret;
        }
    }
}
