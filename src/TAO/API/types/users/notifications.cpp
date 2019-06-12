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

#include <TAO/Ledger/include/create.h>
#include <TAO/Ledger/types/mempool.h>

#include <Util/include/hex.h>
#include <Util/include/debug.h>

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

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


            /* Get the last transaction. */
            uint512_t hashLast = 0;
            if(!LLD::Ledger->ReadLast(hashGenesis, hashLast))
                throw APIException(-28, "No transactions found");

            /* Keep a running list of owned registers. */
            std::vector<std::tuple<uint256_t, uint256_t, uint64_t> > vRegisters;

            /* Loop until genesis. */
            while(hashLast != 0)
            {
                /* Get the transaction from disk. */
                TAO::Ledger::Transaction tx;
                if(!LLD::Ledger->ReadTx(hashLast, tx))
                    throw APIException(-28, "Failed to read transaction");

                /* Set the next last. */
                hashLast = tx.hashPrevTx;

                /* Loop through all contracts. */
                uint32_t nContracts = tx.Size();

                json::json contracts = json::json::array();

                for(uint32_t nContract = 0; nContract < nContracts; ++nContract)
                {
                    /* Attempt to unpack a coinbase transaction. */
                    if(TAO::Register::Unpack(tx[nContract], TAO::Operation::OP::COINBASE))
                    {
                        /* Create a json contract object, manually add in contract ID. */
                        json::json contract = ContractToJSON(tx[nContract], nContract);

                        /* Push back in vector. */
                        contracts.push_back(contract);
                        continue;
                    }

                    /* Register address. */
                    uint256_t hashAddress;

                    /* Attempt to unpack a register script. */
                    TAO::Register::Object object;
                    if(!TAO::Register::Unpack(tx[nContract], object, hashAddress))
                        continue;

                    /* Check that it is an object. */
                    if(object.nType != TAO::Register::REGISTER::OBJECT)
                        continue;

                    /* Parse out the object register. */
                    if(!object.Parse())
                        continue;

                    /* Check that it is an account. */
                    if(object.Base() != TAO::Register::OBJECTS::ACCOUNT)
                        continue;

                    /* Get the token address. */
                    uint256_t hashToken;
                    if(!LLD::Register->ReadIdentifier(object.get<uint256_t>("token"), hashToken))
                        continue;

                    /* Push the token identifier to list to check. */
                    vRegisters.push_back(std::make_tuple(hashAddress, hashToken, object.get<uint64_t>("balance")));

                }

                /* Add in coinbase contracts, if any. */
                if(contracts.size())
                {
                    json::json obj;
                    obj["txid"] = tx.GetHash().ToString();
                    obj["contracts"] = contracts;
                    ret.push_back(obj);
                }

            }

            /* Start with sequence 0 (chronological order). */
            uint32_t nSequence = 0;

            /* Loop until genesis. */
            uint32_t nTotal = 0;

            /* Get notifications for foreign token registers. */
            for(const auto& hash : vRegisters)
            {
                /* Loop through all events for given token (split payments). */
                while(!config::fShutdown)
                {
                    uint256_t hashAddress = std::get<0>(hash);
                    uint256_t hashToken = std::get<1>(hash);

                    /* Get the current page. */
                    uint32_t nCurrentPage = nTotal / nLimit;

                    /* Get the transaction from disk. */
                    TAO::Ledger::Transaction tx;
                    if(!LLD::Ledger->ReadEvent(hashToken, nSequence, tx))
                        break;

                    ++nTotal;

                    /* Check the paged data. */
                    if(nCurrentPage < nPage)
                        continue;

                    if(nCurrentPage > nPage)
                        break;

                    if(nTotal - (nPage * nLimit) > nLimit)
                        break;

                    /* Read the object register. */
                    TAO::Register::Object object;
                    if(!LLD::Register->ReadState(hashToken, object))
                        continue;

                    /* Parse the object register. */
                    if(!object.Parse())
                        continue;

                    json::json obj;
                    obj["txid"] = tx.GetHash().ToString();

                    json::json contracts = json::json::array();
                    uint32_t nContracts = tx.Size();
                    for(uint32_t nContract = 0; nContract < nContracts; ++nContract)
                    {
                        /* Check claims against notifications. */
                        if(LLD::Ledger->HasProof(hashAddress, tx.GetHash(), nContract, TAO::Ledger::FLAGS::MEMPOOL))
                            continue;

                        /* Get the json object for the current contract in the transaction. */
                        json::json contract = ContractToJSON(tx[nContract], nContract);

                        if(contract["OP"] == "DEBIT")
                        {
                            uint256_t hashTo = uint256_t(contract["to"].get<std::string>());

                            TAO::Register::State stateTo;
                            if(!LLD::Register->ReadState(hashTo, stateTo))
                                continue;

                            if(stateTo.nType == TAO::Register::REGISTER::RAW
                            || stateTo.nType == TAO::Register::REGISTER::READONLY)
                            {
                                /* Calculate the partial debit amount (amount = amount * balance / supply). */
                                contract["amount"] = (contract["amount"].get<uint64_t>() * object.get<uint64_t>("balance")) / object.get<uint64_t>("supply");
                            }
                        }

                        /* Add the current contract to the json contracts array. */
                        contracts.push_back(contract);
                    }

                    /* Set the contract info on the json object. */
                    obj["contracts"] = contracts;


                    ret.push_back(obj);

                    /* Iterate sequence forward. */
                    ++nSequence;
                }
            }

            /*
            GetPartials()
            GetDebits()
            GetTransfers()
            GetCoinbases()
            */

            /* Get notifications for personal genesis indexes. */
            for(uint32_t nSequence = 0; ; ++nSequence)
            {
                /* Get the current page. */
                uint32_t nCurrentPage = nTotal / nLimit;

                /* Get the transaction from disk. */
                TAO::Ledger::Transaction tx;
                if(!LLD::Ledger->ReadEvent(hashGenesis, nSequence, tx))
                    break;

                /* Attempt to unpack a register script. */
                uint256_t hashAddress;
                uint32_t nContracts = tx.Size();
                bool fContinue = false;
                for(uint32_t nContract = 0; nContract < nContracts; ++nContract)
                {
                    if(!TAO::Register::Unpack(tx[nContract], hashAddress))
                    {
                        fContinue = true;
                        break;
                    }

                    /* Check claims against notifications. */
                    if(LLD::Ledger->HasProof(hashAddress, tx.GetHash(), nContract, TAO::Ledger::FLAGS::MEMPOOL))
                    {
                        fContinue = true;
                        break;
                    }
                }

                /* If any of the contracts failed, continue. */
                if(fContinue)
                    continue;


                ++nTotal;

                /* Check the paged data. */
                if(nCurrentPage < nPage)
                    continue;

                if(nCurrentPage > nPage)
                    break;

                if(nTotal - (nPage * nLimit) > nLimit)
                    break;

                json::json obj;
                /* Signatures and public keys are verbose level 2 and up. */
                obj["txid"]       = tx.GetHash().ToString();
                obj["contracts"]   = ContractsToJSON(tx);

                ret.push_back(obj);

            }

            /* Check for size. */
            if(ret.empty())
                throw APIException(-26, "No notifications available");

            return ret;
        }
    }
}
