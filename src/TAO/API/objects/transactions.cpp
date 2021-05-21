/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/users/types/users.h>

#include <TAO/API/include/global.h>
#include <TAO/API/include/json.h>

#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/include/create.h>
#include <TAO/Ledger/types/mempool.h>
#include <TAO/Ledger/types/sigchain.h>

#include <TAO/Operation/include/enum.h>

#include <Util/include/hex.h>

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        /* Lists all transactions for a given register. */
        json::json Objects::ListTransactions(const json::json& params, bool fHelp)
        {
            /* JSON return value. */
            json::json ret = json::json::array();

            /* Get the Genesis ID. */
            uint256_t hashGenesis = 0;

            /* Check to see if caller has supplied a specific genesis or username. */
            if(params.find("genesis") != params.end())
                hashGenesis.SetHex(params["genesis"].get<std::string>());
            else if(params.find("username") != params.end())
                hashGenesis = TAO::Ledger::SignatureChain::Genesis(params["username"].get<std::string>().c_str());
            else
                /* If no specific genesis or username have been provided then fall back to the active sig chain */
                hashGenesis = Commands::Get<Users>()->GetSession(params).GetAccount()->Genesis();

            /* The genesis hash of the API caller, if logged in */
            uint256_t hashCaller = Commands::Get<Users>()->GetCallersGenesis(params);

            if(config::fClient.load() && hashGenesis != hashCaller)
                throw APIException(-300, "API can only be used to lookup data for the currently logged in signature chain when running in client mode");

            /* The register address of the object to get the transactions for. */
            TAO::Register::Address hashRegister ;

            /* Flag indicating that the register is a trust account, as we need to process trust-related transactions differently */
            bool fTrust = false;

            /* If name is provided then use this to deduce the register address,
             * otherwise try to find the raw hex encoded address. */
            if(params.find("name") != params.end() && !params["name"].get<std::string>().empty())
                hashRegister = Names::ResolveAddress(params, params["name"].get<std::string>());
            else if(params.find("address") != params.end())
                hashRegister.SetBase58(params["address"].get<std::string>());
            else
                throw APIException(-33, "Missing name or address");

            /* Get the account object. */
            TAO::Register::Object object;
            if(!LLD::Register->ReadState(hashRegister, object))
                throw APIException(-104, "Object not found");

            if(config::fClient.load() && object.hashOwner != Commands::Get<Users>()->GetCallersGenesis(params))
                throw APIException(-300, "API can only be used to lookup data for the currently logged in signature chain when running in client mode");

            /* We need to check if this register is a trust account as stake/genesis/trust/stake/unstake
               transactions don't include the account register address like all other transactions do */
            if(object.Parse() && object.Standard() == TAO::Register::OBJECTS::TRUST)
                fTrust = true;

            /* Number of results to return. */
            uint32_t nLimit = 100;

            /* Offset into the result set to return results from */
            uint32_t nOffset = 0;

            /* Sort order to apply */
            std::string strOrder = "desc";

            /* Get the params to apply to the response. */
            GetListParams(params, strOrder, nLimit, nOffset);

            /* Get verbose levels. */
            std::string strVerbose = "default";
            if(params.find("verbose") != params.end())
                strVerbose = params["verbose"].get<std::string>();

            uint32_t nVerbose = 1;
            if(strVerbose == "default")
                nVerbose = 1;
            else if(strVerbose == "summary")
                nVerbose = 2;
            else if(strVerbose == "detail")
                nVerbose = 3;

            /* Get the last transaction. */
            uint512_t hashLast = 0;
            if(!LLD::Ledger->ReadLast(hashGenesis, hashLast, TAO::Ledger::FLAGS::MEMPOOL))
                throw APIException(-144, "No transactions found");

            /* fields to ignore in the where clause.  This is necessary so that name and address params are not treated as
               standard where clauses to filter the json */
            std::vector<std::string> vIgnore = {"name", "address"};

            /* Loop until genesis. */
            uint32_t nTotal = 0;
            while(hashLast != 0)
            {
                /* Get the transaction from disk. */
                TAO::Ledger::Transaction tx;
                if(!LLD::Ledger->ReadTx(hashLast, tx, TAO::Ledger::FLAGS::MEMPOOL))
                {
                    /* In client mode it is possible to not have the full sig chain if it is still being downloaded asynchronously.*/
                    if(config::fClient.load())
                        break;
                    else
                        throw APIException(-108, "Failed to read transaction");
                }

                /* Flag indicating this transaction is trust related */
                bool fTrustRelated = false;

                /* The contracts to include that relate to the supplied register */
                std::vector<std::pair<TAO::Operation::Contract, uint32_t>> vContracts;

                /* Check all contracts in the transaction to see if any of them relates to the requested object register. */
                uint32_t nContracts = tx.Size();
                for(uint32_t nContract = 0; nContract < nContracts; ++nContract)
                {
                    /* reset trust related flag */
                    fTrustRelated = false;

                    /* The register address that this contract relates to, if any  */
                    TAO::Register::Address hashAddress;

                    /* Retrieve the contract from the transaction for easier processing */
                    const TAO::Operation::Contract& contract = tx[nContract];

                    /* Reset the operation stream position in case it was loaded from mempool and therefore still in previous state */
                    contract.Reset();

                    /* Seek the contract operation stream to the position of the primitive. */
                    contract.SeekToPrimitive();

                    /* Get the contract operation. */
                    uint8_t OPERATION = 0;
                    contract >> OPERATION;

                    /* Check the current opcode. */
                    switch(OPERATION)
                    {
                        case TAO::Operation::OP::WRITE:
                        case TAO::Operation::OP::APPEND:
                        case TAO::Operation::OP::CREATE:
                        case TAO::Operation::OP::TRANSFER:
                        case TAO::Operation::OP::DEBIT:
                        case TAO::Operation::OP::FEE:
                        case TAO::Operation::OP::LEGACY:
                        {
                            /* Get the Address of the Register. */
                            contract >> hashAddress;
                            break;
                        }

                        case TAO::Operation::OP::CLAIM:
                        {
                            /* Extract the transaction from contract. */
                            uint512_t hashTx = 0;
                            contract >> hashTx;

                            /* Extract the contract-id. */
                            uint32_t nContract = 0;
                            contract >> nContract;

                            /* Extract the address from the contract. */
                            contract >> hashAddress;
                            break;
                        }

                        case TAO::Operation::OP::CREDIT:
                        {
                            /* Extract the transaction from contract. */
                            uint512_t hashTx = 0;
                            contract >> hashTx;

                            /* Extract the contract-id. */
                            uint32_t nID = 0;
                            contract >> nID;

                            /* Get the receiving account address. */
                            contract >> hashAddress;
                            break;
                        }

                        case TAO::Operation::OP::TRUST:
                        case TAO::Operation::OP::GENESIS:
                        case TAO::Operation::OP::MIGRATE:
                        {
                            fTrustRelated = true;
                            break;
                        }
                    }

                    /* If the register from the contract is the same as the requested register OR if the register is a trust account
                       and the contract is a trust related operation, then add it to the contracts to be included for this tx */
                    if((hashAddress == hashRegister) || (fTrust && fTrustRelated))
                        vContracts.push_back(std::make_pair(contract, nContract));
                }

                /* Set the next last. */
                hashLast = !tx.IsFirst() ? tx.hashPrevTx : 0;

                /* skip this transaction if none of its contracts relate to the register */
                if(vContracts.size() == 0)
                    continue;


                /* Read the block state from the the ledger DB using the transaction hash index */
                TAO::Ledger::BlockState blockState;
                LLD::Ledger->ReadBlock(tx.GetHash(), blockState);

                /* Build the transaction JSON. */
                json::json jsonTx;

                /* Always add the transaction hash */
                jsonTx["txid"] = tx.GetHash().GetHex();

                /* Basic TX info for level 2 and up */
                if(nVerbose >= 2)
                {
                    /* Build base transaction data. */
                    jsonTx["type"]      = tx.TypeString();
                    jsonTx["version"]   = tx.nVersion;
                    jsonTx["sequence"]  = tx.nSequence;
                    jsonTx["timestamp"] = tx.nTimestamp;
                    jsonTx["blockhash"] = blockState.IsNull() ? "" : blockState.GetHash().GetHex();
                    jsonTx["confirmations"] = blockState.IsNull() ? 0 : TAO::Ledger::ChainState::nBestHeight.load() - blockState.nHeight + 1;

                    /* Genesis and hashes are verbose 3 and up. */
                    if(nVerbose >= 3)
                    {
                        /* More sigchain level details. */
                        jsonTx["genesis"]   = tx.hashGenesis.ToString();
                        jsonTx["nexthash"]  = tx.hashNext.ToString();
                        jsonTx["prevhash"]  = tx.hashPrevTx.ToString();

                        /* The cryptographic data. */
                        jsonTx["pubkey"]    = HexStr(tx.vchPubKey.begin(), tx.vchPubKey.end());
                        jsonTx["signature"] = HexStr(tx.vchSig.begin(),    tx.vchSig.end());
                    }
                }

                /* Always add the contracts if level 2 and up */
                if(nVerbose >= 1)
                {
                    /* Declare the return JSON object*/
                    json::json jsonContracts = json::json::array();

                    /* Add all relevant contracts to the response. */
                    for(const auto& contract : vContracts)
                    {
                        /* JSONify the contract */
                        json::json contractJSON = ContractToJSON(hashCaller, contract.first, contract.second, nVerbose);

                        /* add the contract to the array */
                        jsonContracts.push_back(contractJSON);

                    }

                    /* Check to see if any contracts were returned.  If not then return an empty transaction */
                    if(!jsonContracts.empty())
                        jsonTx["contracts"] = jsonContracts;
                    else
                        jsonTx = json::json();
                }

                /* Check to see whether the contracts were filtered out */
                if(jsonTx.empty())
                    continue;

                ++nTotal;

                /* Check the offset. */
                if(nTotal <= nOffset)
                    continue;

                /* Check the limit */
                if(nTotal - nOffset > nLimit)
                    break;

                ret.push_back(jsonTx);
            }

            return ret;
        }
    }
}
