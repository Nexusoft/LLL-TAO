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

#include <TAO/Register/include/unpack.h>
#include <TAO/Register/types/object.h>

#include <Util/include/hex.h>
#include <Util/include/debug.h>

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        /*  Gets the currently outstanding contracts that have not been matched with a credit or claim. */
        bool Users::GetOutstanding(const uint256_t& hashGenesis,
                std::vector<std::pair<uint32_t, TAO::Operation::Contract>> &vContracts)
        {
            /* Get the last transaction. */
            uint512_t hashLast = 0;
            if(LLD::Ledger->ReadLast(hashGenesis, hashLast, TAO::Ledger::FLAGS::MEMPOOL))
            {
                /* Get the coinbase transactions. */
                get_coinbases(hashGenesis, hashLast, vContracts);

                /* Get the debit and transfer transactions. */
                get_events(hashGenesis, hashLast, vContracts);

            }

            //TODO: sort transactions by timestamp here.

            return true;
        }


        /* Get the outstanding debits and transfer transactions. */
        bool Users::get_events(const uint256_t& hashGenesis,
                uint512_t hashLast, std::vector<std::pair<uint32_t, TAO::Operation::Contract>> &vContracts)
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
                    /* Attempt to unpack a register script (DEBIT or TRANSFER or COINBASE). */
                    uint256_t hashTransfer;
                    if(!TAO::Register::Unpack(tx[nContract], hashTransfer))
                        continue;

                    /* Check for genesis. */
                    if(tx[nContract].Primitive() == TAO::Operation::OP::DEBIT)
                    {
                        /* Check to genesis. */
                        TAO::Register::State state;
                        if(!LLD::Register->ReadState(hashTransfer, state))
                            continue;

                        /* Check owner. */
                        if(state.hashOwner != hashGenesis)
                            continue;
                    }
                    else if(hashGenesis != hashTransfer)
                        continue;

                    /* Check if proofs are spent. */
                    if(LLD::Ledger->HasProof(hashTransfer, tx.GetHash(), nContract, TAO::Ledger::FLAGS::MEMPOOL))
                        continue;

                    /* Add the coinbase transaction and skip rest of contracts. */
                    vContracts.push_back(std::make_pair(nContract, tx[nContract]));
                }

                /* Iterate the sequence id forward. */
                ++nSequence;
            }

            return true;
        }


        /*  Get the outstanding coinbases. */
        bool Users::get_coinbases(const uint256_t& hashGenesis,
                uint512_t hashLast, std::vector<std::pair<uint32_t, TAO::Operation::Contract>> &vContracts)
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
                        vContracts.push_back(std::make_pair(nContract, tx[nContract]));
                    }
                }

                /* Set the next last. */
                hashLast = tx.hashPrevTx;
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
            std::vector<std::pair<uint32_t, TAO::Operation::Contract>> vContracts;
            GetOutstanding(hashGenesis, vContracts);

            /* Get notifications for foreign token registers. */
            for(const auto& contract : vContracts)
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

                /* Get contract JSON data. */
                json::json obj = ContractToJSON(hashCaller, contract.second, 1);
                obj["txid"]   = contract.second.Hash().ToString();
                obj["output"] = contract.first;

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
