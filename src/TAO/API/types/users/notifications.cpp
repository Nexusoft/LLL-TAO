/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <tuple>

#include <LLD/include/global.h>

#include <TAO/API/types/users.h>
#include <TAO/API/include/global.h>
#include <TAO/API/include/utils.h>
#include <TAO/API/include/json.h>

#include <TAO/Ledger/include/constants.h>
#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/include/create.h>

#include <TAO/Ledger/types/transaction.h>
#include <TAO/Ledger/types/mempool.h>
#include <TAO/Ledger/types/sigchain.h>

#include <TAO/Operation/include/enum.h>

#include <TAO/Register/include/constants.h>
#include <TAO/Register/include/unpack.h>
#include <TAO/Register/types/object.h>

#include <Util/include/hex.h>
#include <Util/include/debug.h>

#include <Legacy/include/evaluate.h>

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        /*  Gets the currently outstanding contracts that have not been matched with a credit or claim. */
        bool Users::GetOutstanding(const uint256_t& hashGenesis,
                std::vector<std::tuple<TAO::Operation::Contract, uint32_t, uint256_t>> &vContracts)
        {
            /* Get the last transaction. */
            uint512_t hashLast = 0;
            if(LLD::Ledger->ReadLast(hashGenesis, hashLast, TAO::Ledger::FLAGS::MEMPOOL))
            {
                /* Get the coinbase transactions. */
                get_coinbases(hashGenesis, hashLast, vContracts);

                /* Get split dividend payments to assets tokenized by tokens we hold */
                get_tokenized_debits(hashGenesis, vContracts);
                
                /* Get the debit and transfer transactions. */
                get_events(hashGenesis, vContracts);

            }

            //TODO: sort transactions by timestamp here.

            return true;
        }


        /*  Gets the currently outstanding legacy UTXO to register transactions that have not been matched with a credit */
        bool Users::GetOutstanding(const uint256_t& hashGenesis,
                std::vector<std::pair<std::shared_ptr<Legacy::Transaction>, uint32_t>> &vContracts)
        {
            get_events(hashGenesis, vContracts);

            //TODO: sort transactions by timestamp here.

            return true;
        }


        /* Get the outstanding debits and transfer transactions. */
        bool Users::get_events(const uint256_t& hashGenesis,
                std::vector<std::tuple<TAO::Operation::Contract, uint32_t, uint256_t>> &vContracts)
        {
            /* Get notifications for personal genesis indexes. */
            TAO::Ledger::Transaction tx;

            uint32_t nSequence = 0;
            while(LLD::Ledger->ReadEvent(hashGenesis, nSequence, tx))
            {
                /* Loop through transaction contracts. */
                uint32_t nContracts = tx.Size();
                for(uint32_t nContract = 0; nContract < nContracts; ++nContract)
                {
                    /* The proof to check for this contract */
                    uint256_t hashProof = 0;

                    /* REset the op stream */
                    tx[nContract].Reset();

                    /* The operation */
                    uint8_t nOp;
                    tx[nContract] >> nOp;

                    /* Check for that the debit is meant for us. */
                    if(nOp == TAO::Operation::OP::DEBIT)
                    {
                        /* Get the source address which is the proof for the debit */
                        tx[nContract] >> hashProof;
                        
                        /* Get the recipient account */
                        uint256_t hashTo = 0;
                        tx[nContract] >> hashTo;
                        
                        /* Retrieve the account. */
                        TAO::Register::State state;
                        if(!LLD::Register->ReadState(hashTo, state))
                            continue;

                        /* Check owner that we are the owner of the recipient account  */
                        if(state.hashOwner != hashGenesis)
                            continue;
                    }
                    else if(nOp == TAO::Operation::OP::TRANSFER)
                    {
                        /* The register address being transferred */
                        uint256_t hashRegister;
                        tx[nContract] >> hashRegister;
                        
                        /* Get recipient genesis hash */
                        tx[nContract] >> hashProof;

                        /* Check that we are the recipient */
                        if(hashGenesis != hashProof) 
                            continue;

                        /* Check that the sender has not claimed it back (voided) */
                        TAO::Register::State state;
                        if(!LLD::Register->ReadState(hashRegister, state))
                            continue;

                        /* Make sure the register claim is in SYSTEM pending from a transfer.  */
                        if(state.hashOwner != 0)
                            continue;

                    }
                    else if(nOp == TAO::Operation::OP::COINBASE)
                    {
                        /* Unpack the miners genesis from the contract */
                        if(!TAO::Register::Unpack(tx[nContract], hashProof))
                            continue;

                        /* Check that we mined it */
                        if(hashGenesis != hashProof) 
                            continue;
                    }
                    else 
                        continue;

                    /* Check to see if we have already credited this debit. */
                    if(LLD::Ledger->HasProof(hashProof, tx.GetHash(), nContract, TAO::Ledger::FLAGS::MEMPOOL))
                        continue;

                    /* Add the coinbase transaction and skip rest of contracts. */
                    vContracts.push_back(std::make_tuple(tx[nContract], nContract, 0));
                }

                /* Iterate the sequence id forward. */
                ++nSequence;
            }

            return true;
        }

        /* Get the outstanding legacy UTXO to register transactions. */
        bool Users::get_events(const uint256_t& hashGenesis,
                std::vector<std::pair<std::shared_ptr<Legacy::Transaction>, uint32_t>> &vContracts)
        {
            /* Get notifications for personal genesis indexes. */
            Legacy::Transaction tx;

            uint32_t nSequence = 0;
            while(LLD::Legacy->ReadEvent(hashGenesis, nSequence, tx))
            {
                /* Make a shared pointer to the transaction so that we can keep it alive until the caller 
                   is done processing the contracts */
                std::shared_ptr<Legacy::Transaction> ptx(new Legacy::Transaction(tx));

                /* Loop through transaction outputs. */
                uint32_t nContracts = ptx->vout.size();
                for(uint32_t nContract = 0; nContract < nContracts; ++nContract)
                {
                    /* check the script to see if it contains a register address */
                    uint256_t hashTo;
                    if(!Legacy::ExtractRegister(ptx->vout[nContract].scriptPubKey, hashTo))
                        continue;
                    
                    /* Read the hashTo account */
                    TAO::Register::State state;
                    if(!LLD::Register->ReadState(hashTo, state))
                        continue;

                    /* Check owner. */
                    if(state.hashOwner != hashGenesis)
                        continue;
                
                    /* Check if proofs are spent. NOTE the proof is the wildcard address since this is a legacy transaction*/
                    if(LLD::Ledger->HasProof(TAO::Register::WILDCARD_ADDRESS, ptx->GetHash(), nContract, TAO::Ledger::FLAGS::MEMPOOL))
                        continue;

                    /* Add the coinbase transaction and skip rest of contracts. */
                    vContracts.push_back(std::make_pair(ptx, nContract));
                }

                /* Iterate the sequence id forward. */
                ++nSequence;
            }

            return true;
        }


        /*  Get the outstanding coinbases. */
        bool Users::get_coinbases(const uint256_t& hashGenesis,
                uint512_t hashLast, std::vector<std::tuple<TAO::Operation::Contract, uint32_t, uint256_t>> &vContracts)
        {
            /* Reverse iterate until genesis (newest to oldest). */
            while(hashLast != 0)
            {
                /* Get the transaction from disk. */
                TAO::Ledger::Transaction tx;
                if(!LLD::Ledger->ReadTx(hashLast, tx, TAO::Ledger::FLAGS::MEMPOOL))
                    return debug::error(FUNCTION, "Failed to read transaction");

                /* Skip this transaction if it is immature. */
                if(!LLD::Ledger->ReadMature(hashLast))
                {
                    /* Set the next last. */
                    hashLast = tx.hashPrevTx;
                    continue;
                }

                /* Loop through all contracts and add coinbase contracts to vector. */
                uint32_t nContracts = tx.Size();
                for(uint32_t nContract = 0; nContract < nContracts; ++nContract)
                {
                    /* Check for coinbase opcode */
                    if(TAO::Register::Unpack(tx[nContract], Operation::OP::COINBASE))
                    {
                        /* Seek past operation. */
                        tx[nContract].Seek(1);

                        /* Get the proof to check coinbase. */
                        uint256_t hashProof;
                        tx[nContract] >> hashProof;

                        /* Check that the proof is to your genesis. */
                        if(hashProof != hashGenesis)
                            continue;

                        /* Check if proofs are spent. */
                        if(LLD::Ledger->HasProof(hashGenesis, hashLast, nContract, TAO::Ledger::FLAGS::MEMPOOL))
                            continue;

                        /* Add the coinbase transaction and skip rest of contracts. */
                        vContracts.push_back(std::make_tuple(tx[nContract], nContract, 0));
                    }
                }

                /* Set the next last. */
                hashLast = tx.hashPrevTx;
            }

            return true;
        }


        /* Get the outstanding debit transactions made to assets owned by tokens you hold. */
        bool Users::get_tokenized_debits(const uint256_t& hashGenesis,
                std::vector<std::tuple<TAO::Operation::Contract, uint32_t, uint256_t>> &vContracts)
        {
            /* Get the list of registers owned by this sig chain */
            std::vector<uint256_t> vRegisters;
            if(!ListRegisters(hashGenesis, vRegisters))
                throw APIException(-74, "No registers found");

            /* Iterate registers to find all token accounts. */
            for(const auto& hashRegister : vRegisters)
            {
                /* Read the object register. */
                TAO::Register::Object object;
                if(!LLD::Register->ReadState(hashRegister, object, TAO::Ledger::FLAGS::MEMPOOL))
                    continue;

                /* Parse the object register. */
                if(!object.Parse())
                    continue;

                /* Check that this is an account */
                if(object.Base() != TAO::Register::OBJECTS::ACCOUNT )
                    continue;

                /* Get the token address */
                uint256_t hashToken = object.get<uint256_t>("token");
                
                /* Check the account is a not NXS account */
                if(hashToken == 0)
                    continue;

                /* Get the balance  */
                uint64_t nBalance = object.get<uint64_t>("balance");

                /* Check that we have some tokens in the account, otherwise there is nothing else to check for this account */
                if(nBalance == 0)
                    continue;

                /* Read the token register. */
                TAO::Register::Object token;
                if(!LLD::Register->ReadState(hashToken, token, TAO::Ledger::FLAGS::MEMPOOL))
                    continue;

                /* Parse the object register. */
                if(!token.Parse())
                    continue;

                /* The last modified time the balance of this token account changed */
                uint64_t nModified = object.nModified;

                /* Loop through all events for the token (split payments). */
                TAO::Ledger::Transaction tx;
                uint32_t nSequence = 0;
                while(LLD::Ledger->ReadEvent(hashToken, nSequence, tx))
                {
                    /* Iterate sequence forward. */
                    ++nSequence;

                    /* Firstly we can ignore any transactions that occurred before our token account was last modified, as only 
                       the balance at the time of the transaction can be used as proof */
                    if(tx.nTimestamp < nModified)
                        continue;

                    /* Loop through transaction contracts. */
                    uint32_t nContracts = tx.Size();
                    for(uint32_t nContract = 0; nContract < nContracts; ++nContract)
                    {
                        TAO::Operation::Contract& contract = tx[nContract];
                        /* Check that this is a debit contract */
                        if(!TAO::Register::Unpack(contract, Operation::OP::DEBIT))
                            continue;
                        
                        /* The account/token the debit came from  */
                        uint256_t hashFrom = 0;

                        /* Seek to the hash from. */
                        contract.Seek(1, Operation::Contract::OPERATIONS);
                        contract >> hashFrom;

                        /* Check to see if we have already claimed our credit. */
                        if(LLD::Ledger->HasProof(hashRegister, tx.GetHash(), nContract, TAO::Ledger::FLAGS::MEMPOOL))
                            continue;

                        /* Now check to see whether the sender has voided (credited back to themselves) */
                        if(LLD::Ledger->HasProof(hashFrom, tx.GetHash(), nContract, TAO::Ledger::FLAGS::MEMPOOL))
                            continue;

                        /* Add the contract to the return list  . */
                        vContracts.push_back(std::make_tuple(contract, nContract, hashRegister));
                    }
                }
            }

            return true;
        }



        /* Get a user's account. */
        json::json Users::Notifications(const json::json& params, bool fHelp)
        {
            /* JSON return value. */
            json::json ret;

            /* Get the Genesis ID. */
            uint256_t hashGenesis = 0;

            /* Get genesis by raw hex. */
            if(params.find("genesis") != params.end())
                hashGenesis.SetHex(params["genesis"].get<std::string>());

            /* Get genesis by username. */
            else if(params.find("username") != params.end())
                hashGenesis = TAO::Ledger::SignatureChain::Genesis(params["username"].get<std::string>().c_str());

            /* Get genesis by session. */
            else if(!config::fMultiuser.load() && mapSessions.count(0))
                hashGenesis = mapSessions[0]->Genesis();

            /* Handle for no genesis. */
            else
                throw APIException(-111, "Missing genesis / username");

            /* The genesis hash of the API caller, if logged in */
            uint256_t hashCaller = users->GetCallersGenesis(params);

            /* Check for paged parameter. */
            uint32_t nPage = 0;
            if(params.find("page") != params.end())
                nPage = std::stoul(params["page"].get<std::string>());

            /* Check for username parameter. */
            uint32_t nLimit = 100;
            if(params.find("limit") != params.end())
                nLimit = std::stoul(params["limit"].get<std::string>());

            /* The total number of notifications. */
            uint32_t nTotal = 0;

            /* Get the outstanding contracts not yet credited or claimed. */
            std::vector<std::tuple<TAO::Operation::Contract, uint32_t, uint256_t>> vContracts;
            GetOutstanding(hashGenesis, vContracts);

            /* Get notifications for foreign token registers. */
            for(const auto& contract : vContracts)
            {
                /* Get a reference to the contract */
                const TAO::Operation::Contract& refContract = std::get<0>(contract); 
                
                /* LOOP: Get the current page. */
                uint32_t nCurrentPage = nTotal / nLimit;

                /* Check the paged data. */
                if(nCurrentPage < nPage)
                    continue;
                if(nCurrentPage > nPage)
                    break;
                if(nTotal - (nPage * nLimit) > nLimit)
                    break;

                /* Get contract JSON data. */
                json::json obj = ContractToJSON(hashCaller, refContract, 1);
                obj["txid"]      = refContract.Hash().ToString();
                obj["time"]      = refContract.Timestamp();

                /* Check to see if there is a proof for the contract, indicating this is a split dividend payment and the
                   hashProof is the account the proves the ownership of it*/
                uint256_t hashProof = std::get<2>(contract);
                
                if(hashProof != 0)
                {
                    /* Read the object register, which is the token . */
                    TAO::Register::Object account;
                    if(!LLD::Register->ReadState(hashProof, account, TAO::Ledger::FLAGS::MEMPOOL))
                        throw APIException(-13, "Account not found");

                    /* Parse the object register. */
                    if(!account.Parse())
                        throw APIException(-36, "Failed to parse object register");

                    /* Check that this is an account */
                    if(account.Base() != TAO::Register::OBJECTS::ACCOUNT )
                        throw APIException(-65, "Object is not an account");

                    /* Get the token address */
                    uint256_t hashToken = account.get<uint256_t>("token");                    

                    /* Read the token register. */
                    TAO::Register::Object token;
                    if(!LLD::Register->ReadState(hashToken, token, TAO::Ledger::FLAGS::MEMPOOL))
                        throw APIException(-125, "Token not found");

                    /* Parse the object register. */
                    if(!token.Parse())
                        throw APIException(-36, "Failed to parse object register");

                    /* Get the token supply so that we an determine our share */
                    uint64_t nSupply = token.get<uint64_t>("supply");

                    /* Get the balance of our token account */
                    uint64_t nBalance = account.get<uint64_t>("balance");

                    /* Get the amount from the debit contract*/
                    uint64_t nAmount = 0;
                    TAO::Register::Unpack(refContract, nAmount);

                    /* Calculate the partial debit amount that this token holder is entitled to. */
                    uint64_t nPartial = (nAmount * nBalance) / nSupply;

                    /* Update the JSON with the partial amount */
                    obj["amount"] = (double) nPartial / pow(10, GetDigits(token));

                    /* Add the token account to the notification */
                    obj["proof"] = hashProof.GetHex();

                    std::string strProof = Names::ResolveName(hashCaller, hashProof);
                    if(!strProof.empty())
                        obj["proof_name"] = strProof;

                    /* Also flag this notification as a split dividend */
                    obj["dividend_payment"] = true;
                    
                }

                /* Add to return object. */
                ret.push_back(obj);

                /* Increment the total number of notifications. */
                ++nTotal;
            }

            /* Get the outstanding legacy transactions not yet credited. */
            std::vector<std::pair<std::shared_ptr<Legacy::Transaction>, uint32_t>> vLegacy;
            GetOutstanding(hashGenesis, vLegacy);

            /* Get notifications for foreign token registers. */
            for(const auto& tx : vLegacy)
            {
                /* LOOP: Get the current page. */
                uint32_t nCurrentPage = nTotal / nLimit;

                /* Check the paged data. */
                if(nCurrentPage < nPage)
                    continue;
                if(nCurrentPage > nPage)
                    break;
                if(nTotal - (nPage * nLimit) > nLimit)
                    break;

                /* The register address of the recipient account */
                uint256_t hashTo;
                Legacy::ExtractRegister(tx.first->vout[tx.second].scriptPubKey, hashTo);

                /* Get transaction JSON data. */
                json::json obj; 
                obj["OP"]       = "LEGACY";
                obj["address"]  = hashTo.GetHex();

                /* Resolve the name of the token/account/register that the debit is to */
                std::string strTo = Names::ResolveName(hashCaller, hashTo);
                if(!strTo.empty())
                    obj["account_name"] = strTo;

                obj["amount"]   = (double)tx.first->vout[tx.second].nValue / TAO::Ledger::NXS_COIN;
                obj["txid"]     = tx.first->GetHash().GetHex();
                obj["time"]     = tx.first->nTime;

                /* Add to return object. */
                ret.push_back(obj);

                /* Increment the total number of notifications. */
                ++nTotal;
            }

            /* Check for size. */
            if(ret.empty())
                throw APIException(-143, "No notifications available");

            return ret;
        }
    }
}
