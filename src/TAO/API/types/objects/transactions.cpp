/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/types/users.h>
#include <TAO/API/include/global.h>
#include <TAO/API/include/utils.h>
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
            json::json ret;

            /* Get the Genesis ID. */
            uint256_t hashGenesis = 0;

            /* The session to use */
            uint256_t nSession = users->GetSession(params, false);

            /* Check to see if caller has supplied a specific genesis or username. */
            if(params.find("genesis") != params.end())
                hashGenesis.SetHex(params["genesis"].get<std::string>());
            else if(params.find("username") != params.end())
                hashGenesis = TAO::Ledger::SignatureChain::Genesis(params["username"].get<std::string>().c_str());
            else if(nSession != -1)
                /* If no specific genesis or username have been provided then fall back to the active sig chain */
                hashGenesis = users->GetGenesis(nSession);
            else
                throw APIException(-111, "Missing genesis / username");

            /* The genesis hash of the API caller, if logged in */
            uint256_t hashCaller = users->GetCallersGenesis(params);

            /* The register address of the object to get the transactions for. */
            TAO::Register::Address hashRegister ;

            /* Flag indicating that the register is a trust account, as we need to process trust-related transactions differently */
            bool fTrust = false;

            /* If name is provided then use this to deduce the register address,
             * otherwise try to find the raw hex encoded address. */
            if(params.find("name") != params.end())
                hashRegister = Names::ResolveAddress(params, params["name"].get<std::string>());
            else if(params.find("address") != params.end())
                hashRegister.SetBase58(params["address"].get<std::string>());
            else
                throw APIException(-33, "Missing name or address");

            /* Get the account object. */
            TAO::Register::Object object;
            if(!LLD::Register->ReadState(hashRegister, object))
                throw APIException(-104, "Object not found");

            /* We need to check if this register is a trust account as stake/genesis/trust/stake/unstake
               transactions don't include the account register address like all other transactions do */
            if(object.Parse() && object.Standard() == TAO::Register::OBJECTS::TRUST)
                fTrust = true;

            /* Check for paged parameter. */
            uint32_t nPage = 0;
            if(params.find("page") != params.end())
                nPage = std::stoul(params["page"].get<std::string>());

            /* Check for username parameter. */
            uint32_t nLimit = 100;
            if(params.find("limit") != params.end())
                nLimit = std::stoul(params["limit"].get<std::string>());

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

            /* Loop until genesis. */
            uint32_t nTotal = 0;
            while(hashLast != 0)
            {
                /* Get the current page. */
                uint32_t nCurrentPage = nTotal / nLimit;

                /* Get the transaction from disk. */
                TAO::Ledger::Transaction tx;
                if(!LLD::Ledger->ReadTx(hashLast, tx, TAO::Ledger::FLAGS::MEMPOOL))
                    throw APIException(-108, "Failed to read transaction");

                /* Flag indicating this transaction is trust related */
                bool fTrustRelated = false;

                /* The contracts to include that relate to the supplied register */
                std::vector<TAO::Operation::Contract> vContracts;

                /* Check all contracts in the transaction to see if any of them relates to the requested object register. */
                uint32_t nContracts = tx.Size();
                for(uint32_t nContract = 0; nContract < nContracts; ++nContract)
                {
                    /* The register address that this contract relates to, if any  */
                    TAO::Register::Address hashAddress;

                    /* Retrieve the contract from the transaction for easier processing */
                    const TAO::Operation::Contract& contract = tx[nContract];

                    /* Start the stream at the beginning. */
                    contract.Reset();

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
                        vContracts.push_back(contract);
                }

                /* Set the next last. */
                hashLast = tx.hashPrevTx;

                /* skip this transaction if none of its contracts relate to the register */
                if(vContracts.size() == 0)
                    continue;

                ++nTotal;

                /* Check the paged data. */
                if(nCurrentPage < nPage)
                    continue;

                if(nCurrentPage > nPage)
                    break;

                if(nTotal - (nPage * nLimit) > nLimit)
                    break;

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
                        jsonContracts.push_back(ContractToJSON(hashCaller, contract, nVerbose));

                    jsonTx["contracts"] = jsonContracts;
                }

                ret.push_back(jsonTx);
            }

            return ret;
        }
    }
}