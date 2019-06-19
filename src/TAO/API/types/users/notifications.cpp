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
#include <TAO/API/include/utils.h>
#include <TAO/API/include/json.h>

#include <TAO/Operation/include/enum.h>


#include <TAO/Register/include/unpack.h>
#include <TAO/Register/types/object.h>

#include <TAO/Ledger/include/constants.h>
#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/include/create.h>
#include <TAO/Ledger/types/transaction.h>
#include <TAO/Ledger/types/mempool.h>

#include <Util/include/hex.h>
#include <Util/include/debug.h>

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        /*  Gets the currently outstanding contracts that have not been matched with a credit or claim. */
        bool Users::GetOutstanding(const uint256_t& hashGenesis, std::vector<TAO::Ledger::Transaction> &vTransactions)
        {
            /* Get the last transaction. */
            uint512_t hashLast = 0;
            if(!LLD::Ledger->ReadLast(hashGenesis, hashLast))
                return debug::error(FUNCTION, "No transactions found");

            /* Get the coinbase transactions. */
            get_coinbases(hashGenesis, hashLast, vTransactions);

            /* Get the debit and transfer transactions. */
            get_events(hashGenesis, hashLast, vTransactions);


            //TODO: sort transactions by timestamp here.

            return true;
        }


        /* Get the outstanding debits and transfer transactions. */
        bool Users::get_events(const uint256_t& hashGenesis, uint512_t hashLast, std::vector<TAO::Ledger::Transaction> &vTransactions)
        {
            /* List of token registers to process. */
            std::vector<uint256_t> vRegisters;

            /* Loop until genesis. */
            while(hashLast != 0)
            {
                /* Get the transaction from disk. */
                Ledger::Transaction tx;
                if(!LLD::Ledger->ReadTx(hashLast, tx))
                    return debug::error(FUNCTION, "Failed to read transaction");

                /* Set the next last. */
                hashLast = tx.hashPrevTx;

                /* Loop through all contracts. */
                uint32_t nContracts = tx.Size();
                for(uint32_t nContract = 0; nContract < nContracts; ++nContract)
                {
                    /* Attempt to unpack a register script. */
                    uint256_t hashAddress;
                    Register::Object account;
                    if(!Register::Unpack(tx[nContract], account, hashAddress))
                        continue;

                    /* Parse out the object register. */
                    if(!account.Parse())
                        continue;

                    /* Check that it is an object register account. */
                    if(account.nType != Register::REGISTER::OBJECT || account.Base() != Register::OBJECTS::ACCOUNT)
                        continue;

                    /* Get the token address and ensure it exists. */
                    uint256_t hashToken = account.get<uint256_t>("token");
                    if(!LLD::Register->HasIdentifier(hashToken))
                        continue;

                    /* Check claims against notifications. */
                    if(LLD::Ledger->HasProof(hashAddress, tx.GetHash(), nContract, Ledger::FLAGS::MEMPOOL))
                        continue;

                    vRegisters.push_back(hashToken);
                }
            }


            /* Get notifications for foreign token registers. */
            for(const auto& hashToken : vRegisters)
            {
                /* Read the object register. */
                TAO::Register::Object object;
                if(!LLD::Register->ReadState(hashToken, object))
                    continue;

                /* Parse the object register. */
                if(!object.Parse())
                    continue;

                uint64_t nBalance = object.get<uint64_t>("balance");
                uint64_t nSupply =  object.get<uint64_t>("supply");

                /* Loop through all events for given token (split payments). */
                TAO::Ledger::Transaction tx;
                uint32_t nSequence = 0;
                while(LLD::Ledger->ReadEvent(hashGenesis, nSequence, tx))
                {
                    /* Determine if tx should be added. */
                    bool fAdd = false;

                    /* Loop through transaction contracts. */
                    uint32_t nContracts = tx.Size();
                    for(uint32_t nContract = 0; nContract < nContracts; ++nContract)
                    {
                        /* Attempt to unpack a register script (DEBIT or TRANSFER). */
                        uint256_t hashAddress;
                        if(!Register::Unpack(tx[nContract], hashAddress))
                            continue;

                        /* Get the hash to */
                        uint256_t hashTo;
                        tx[nContract] >> hashTo;

                        /* Verify that the hash to exists. */
                        Register::State stateTo;
                        if(!LLD::Register->ReadState(hashTo, stateTo))
                            continue;

                        /* If the operation is a debit, calculate the partial token debit amount. */
                        if(Register::Unpack(tx[nContract], Operation::OP::DEBIT))
                        {
                            if(stateTo.nType == Register::REGISTER::RAW || stateTo.nType == Register::REGISTER::READONLY)
                            {
                                /* Seek to the debit amount. */
                                tx[nContract].Seek(65, Operation::Contract::OPERATIONS);

                                /* Get the debit amount. */
                                uint64_t nAmount;
                                tx[nContract] >> nAmount;

                                /* Calculate the partial debit amount. */
                                uint64_t nPartial = (nAmount * nBalance) / nSupply;

                                /* Place the partial debit amount in the contract operation stream. */
                                tx[nContract].Rewind(sizeof(uint64_t), Operation::Contract::OPERATIONS);
                                tx[nContract] << nPartial;
                            }
                        }

                        /* Transaction is valid for notifications. */
                        fAdd = true;
                    }

                    /* Add the current transaction to the list of notifications. */
                    if(fAdd)
                        vTransactions.push_back(tx);

                    /* Iterate sequence forward. */
                    ++nSequence;
                }
            }

            /* Get notifications for personal genesis indexes. */
            TAO::Ledger::Transaction tx;
            uint32_t nSequence = 0;
            while(LLD::Ledger->ReadEvent(hashGenesis, nSequence, tx))
            {
                /* Loop through transaction contracts. */
                uint32_t nContracts = tx.Size();
                for(uint32_t nContract = 0; nContract < nContracts; ++nContract)
                {
                    /* Attempt to unpack a register script (DEBIT or TRANSFER). */
                    uint256_t hashAddress;
                    if(!TAO::Register::Unpack(tx[nContract], hashAddress))
                        continue;

                    /* Check if proofs are spent. */
                    if(LLD::Ledger->HasProof(hashAddress, tx.GetHash(), nContract, TAO::Ledger::FLAGS::MEMPOOL))
                        continue;

                    /* Add the current contract to the json contracts array. */
                    vTransactions.push_back(tx);
                    break;
                }

                /* Iterate the sequence id forward. */
                ++nSequence;
            }

            return true;
        }


        /*  Get the outstanding coinbases. */
        bool Users::get_coinbases(const uint256_t& hashGenesis, uint512_t hashLast, std::vector<TAO::Ledger::Transaction> &vTransactions)
        {
            /* Reverse iterate until genesis (newest to oldest). */
            while(hashLast != 0)
            {
                /* Get the transaction from disk. */
                Ledger::Transaction tx;
                if(!LLD::Ledger->ReadTx(hashLast, tx))
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
                    if(Register::Unpack(tx[nContract], Operation::OP::COINBASE))
                    {
                        /* Check if proofs are spent. */
                        if(LLD::Ledger->HasProof(hashGenesis, hashLast, nContract, TAO::Ledger::FLAGS::MEMPOOL))
                            continue;

                        /* Add the coinbase transaction and skip rest of contracts. */
                        vTransactions.push_back(tx);
                        break;
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
                throw APIException(-25, "Missing Genesis or Username");

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
            std::vector<TAO::Ledger::Transaction> vTransactions;
            GetOutstanding(hashGenesis, vTransactions);

            /* Get notifications for foreign token registers. */
            for(const auto& tx : vTransactions)
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

                /* Add the transactions to the JSON object. */
                ret.push_back(ContractsToJSON(tx, 1));

                /* Increment the total number of notifications. */
                ++nTotal;
            }

            /* Check for size. */
            if(ret.empty())
                throw APIException(-26, "No notifications available");

            return ret;
        }
    }
}
