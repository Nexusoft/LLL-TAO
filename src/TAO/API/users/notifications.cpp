/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <tuple>

#include <LLD/include/global.h>

#include <TAO/API/include/users.h>
#include <TAO/API/include/utils.h>

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

            /* Watch for destination genesis. If no specific genesis or username
             * have been provided then fall back to the active sigchain. */
            if(params.find("genesis") != params.end())
                hashGenesis.SetHex(params["genesis"].get<std::string>());
            else if(params.find("username") != params.end())
                hashGenesis = TAO::Ledger::SignatureChain::Genesis(params["username"].get<std::string>().c_str());
            else if(!config::fAPISessions.load() && mapSessions.count(0))
                hashGenesis = mapSessions[0]->Genesis();
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

            /* Get verbose levels. */
            uint32_t nVerbose = 3;
            if(params.find("verbose") != params.end())
                nVerbose = std::stoul(params["verbose"].get<std::string>());

            /* Get the last transaction. */
            uint512_t hashLast = 0;
            if(!LLD::legDB->ReadLast(hashGenesis, hashLast))
                throw APIException(-28, "No transactions found");

            /* Keep a running list of owned registers. */
            std::vector<std::tuple<uint256_t, uint256_t, uint64_t> > vRegisters;

            /* Loop until genesis. */
            while(hashLast != 0)
            {
                /* Get the transaction from disk. */
                TAO::Ledger::Transaction tx;
                if(!LLD::legDB->ReadTx(hashLast, tx))
                    throw APIException(-28, "Failed to read transaction");

                /* Set the next last. */
                hashLast = tx.hashPrevTx;

                /* Register address. */
                uint256_t hashAddress;

                /* Attempt to unpack a register script. */
                TAO::Register::Object object;
                if(!TAO::Register::Unpack(tx, object, hashAddress))
                    continue;

                /* Check that it is an account. */
                if(object.nType != TAO::Register::REGISTER::OBJECT)
                    continue;

                /* Parse out the object register. */
                if(!object.Parse())
                    continue;

                /* Get the token address. */
                uint256_t hashToken;
                if(!LLD::regDB->ReadIdentifier(object.get<uint256_t>("identifier"), hashToken))
                    continue;

                /* Push the token identifier to list to check. */
                vRegisters.push_back(std::make_tuple(hashAddress, hashToken, object.get<uint64_t>("balance")));
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
                    /* Get the current page. */
                    uint32_t nCurrentPage = nTotal / nLimit;

                    /* Get the transaction from disk. */
                    TAO::Ledger::Transaction tx;
                    if(!LLD::legDB->ReadEvent(std::get<1>(hash), nSequence, tx))
                        break;

                    /* Check claims against notifications. */
                    if(LLD::legDB->HasProof(std::get<0>(hash), tx.GetHash(), TAO::Ledger::FLAGS::BLOCK | TAO::Ledger::FLAGS::MEMPOOL))
                        continue;

                    ++nTotal;

                    /* Check the paged data. */
                    if(nCurrentPage < nPage)
                        continue;

                    if(nCurrentPage > nPage)
                        break;

                    if(nTotal > nLimit)
                        break;

                    /* Read the object register. */
                    TAO::Register::Object object;
                    if(!LLD::regDB->ReadState(std::get<1>(hash), object))
                        continue;

                    json::json obj;
                    obj["version"]   = tx.nVersion;
                    obj["sequence"]  = tx.nSequence;
                    obj["timestamp"] = tx.nTimestamp;

                    /* Genesis and hashes are verbose 1 and up. */
                    if(nVerbose >= 1)
                    {
                        obj["genesis"]   = tx.hashGenesis.ToString();
                        obj["nexthash"]  = tx.hashNext.ToString();
                        obj["prevhash"]  = tx.hashPrevTx.ToString();
                    }

                    /* Signatures and public keys are verbose level 2 and up. */
                    if(nVerbose >= 2)
                    {
                        obj["pubkey"]    = HexStr(tx.vchPubKey.begin(), tx.vchPubKey.end());
                        obj["signature"] = HexStr(tx.vchSig.begin(),    tx.vchSig.end());
                    }

                    obj["hash"]          = tx.GetHash().ToString();
                    obj["operation"]     = OperationToJSON(tx.ssOperation);

                    if(obj["operation"]["OP"] == "DEBIT")
                    {
                        uint256_t hashTo = uint256_t(obj["operation"]["transfer"].get<std::string>());

                        TAO::Register::State stateTo;
                        if(!LLD::regDB->ReadState(hashTo, stateTo))
                            continue;

                        if(stateTo.nType == TAO::Register::REGISTER::RAW
                        || stateTo.nType == TAO::Register::REGISTER::READONLY)
                        {
                            /* Parse the object register. */
                            if(!object.Parse())
                                continue;

                            /* Calculate the partial debit amount. */
                            obj["operation"]["amount"] = (obj["operation"]["amount"].get<uint64_t>() * std::get<2>(hash)) / object.get<uint64_t>("supply");
                        }
                    }

                    ret.push_back(obj);

                    /* Iterate sequence forward. */
                    ++nSequence;
                }
            }


            /* Get notifications for personal genesis indexes. */
            for(uint32_t nSequence = 0; ; ++nSequence)
            {
                /* Get the current page. */
                uint32_t nCurrentPage = nTotal / nLimit;

                /* Get the transaction from disk. */
                TAO::Ledger::Transaction tx;
                if(!LLD::legDB->ReadEvent(hashGenesis, nSequence, tx))
                    break;

                /* Attempt to unpack a register script. */
                uint256_t hashAddress;
                if(!TAO::Register::Unpack(tx, hashAddress))
                    continue;

                /* Check claims against notifications. */
                if(LLD::legDB->HasProof(hashAddress, tx.GetHash(), TAO::Ledger::FLAGS::BLOCK | TAO::Ledger::FLAGS::MEMPOOL))
                    continue;

                ++nTotal;

                /* Check the paged data. */
                if(nCurrentPage < nPage)
                    continue;

                if(nCurrentPage > nPage)
                    break;

                if(nTotal > nLimit)
                        break;

                json::json obj;
                obj["version"]   = tx.nVersion;
                obj["sequence"]  = tx.nSequence;
                obj["timestamp"] = tx.nTimestamp;

                /* Genesis and hashes are verbose 1 and up. */
                if(nVerbose >= 1)
                {
                    obj["genesis"]   = tx.hashGenesis.ToString();
                    obj["nexthash"]  = tx.hashNext.ToString();
                    obj["prevhash"]  = tx.hashPrevTx.ToString();
                }

                /* Signatures and public keys are verbose level 2 and up. */
                if(nVerbose >= 2)
                {
                    obj["pubkey"]    = HexStr(tx.vchPubKey.begin(), tx.vchPubKey.end());
                    obj["signature"] = HexStr(tx.vchSig.begin(),    tx.vchSig.end());
                }

                obj["hash"]       = tx.GetHash().ToString();
                obj["operation"]  = OperationToJSON(tx.ssOperation);

                ret.push_back(obj);

            }

            /* Check for size. */
            if(ret.empty())
                throw APIException(-26, "No notifications available");

            return ret;
        }
    }
}
