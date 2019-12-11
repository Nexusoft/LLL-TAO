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

#include <TAO/Ledger/types/mempool.h>
#include <TAO/Ledger/types/sigchain.h>
#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/include/constants.h>

#include <TAO/Operation/include/enum.h>

#include <Util/include/hex.h>

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        /* Returns a list of all votes made to an account. */
        json::json Voting::List(const json::json& params, bool fHelp)
        {
            /* JSON return value. */
            json::json ret = json::json::array();

            /* The register address of the account to get the vote count for. */
            TAO::Register::Address hashAccount ;

            /* If name is provided then use this to deduce the register address,
             * otherwise try to find the raw hex encoded address. */
            if(params.find("name") != params.end())
                hashAccount = Names::ResolveAddress(params, params["name"].get<std::string>());
            else if(params.find("address") != params.end())
                hashAccount.SetBase58(params["address"].get<std::string>());
            else
                throw APIException(-33, "Missing name or address");

            /* Get the account . */
            TAO::Register::Object account;
            if(!LLD::Register->ReadState(hashAccount, account))
                throw APIException(-13, "Account not found");

            /* Get the Genesis ID of the account owner. */
            uint256_t hashGenesis = account.hashOwner;

            /* Parse the object register. */
            if(!account.Parse())
                throw APIException(-14, "Object failed to parse");

            /* Check that the object is an account. */
            if(account.Base() != TAO::Register::OBJECTS::ACCOUNT)
                throw APIException(-65, "Object is not an account");

            /* Check for paged parameter. */
            uint32_t nPage = 0;
            if(params.find("page") != params.end())
                nPage = std::stoul(params["page"].get<std::string>());

            /* Check for limit parameter. */
            uint32_t nLimit = 100;
            if(params.find("limit") != params.end())
                nLimit = std::stoul(params["limit"].get<std::string>());

            /* Get the last transaction. */
            uint512_t hashLast = 0;
            if(!LLD::Ledger->ReadLast(hashGenesis, hashLast))
                throw APIException(-144, "No transactions found");

            /* vector of votes by sig chain so we can detect dupes */
            std::vector<uint256_t> vVotes;

            /* Loop until genesis. */
            uint32_t nTotal = 0;
            while(hashLast != 0)
            {
                /* Get the current page. */
                uint32_t nCurrentPage = nTotal / nLimit;

                /* Get the transaction from disk. */
                TAO::Ledger::Transaction tx;
                if(!LLD::Ledger->ReadTx(hashLast, tx))
                    throw APIException(-108, "Failed to read transaction");

                /* Check all contracts in the transaction to see if any of them relates to the requested vote account. */
                uint32_t nContracts = tx.Size();
                for(uint32_t nContract = 0; nContract < nContracts; ++nContract)
                {
                    
                    /* Retrieve the contract from the transaction for easier processing */
                    const TAO::Operation::Contract& contract = tx[nContract];

                    /* Start the stream at the beginning. */
                    contract.Reset();

                    /* Get the operation byte. */
                    uint8_t nType = 0;
                    contract >> nType;

                    /* Check type. Only credits can be counted */
                    if(nType != TAO::Operation::OP::CREDIT)
                        continue;

                    /* Extract the transaction from contract. */
                    uint512_t hashTx = 0;
                    contract >> hashTx;

                    /* Extract the contract-id. */
                    uint32_t nID = 0;
                    contract >> nID;

                    /* The register address that this contract relates to, if any  */
                    TAO::Register::Address hashAddress;
                    contract >> hashAddress;
                    
                    /* Check that the credit was made to the requested vote account */
                    if(hashAddress != hashAccount)
                        continue;

                    /* The account address of the sending account */
                    TAO::Register::Address hashFrom;
                    contract >> hashFrom;

                    /* Retrieve the account that the debit was made from as we need to look at the originating sig chain */
                    TAO::Register::Object from;
                    if(!LLD::Register->ReadState(hashFrom, from))
                        continue;

                    /* Get the sending account owner */
                    uint256_t hashSender = from.hashOwner;

                    /* Check to see if a vote for this sig chain has already been counted */
                    if(std::find(vVotes.begin(), vVotes.end(), hashSender) != vVotes.end())
                        continue;

                    /* The weighted vote amount for this contract */
                    uint64_t nVote = TAO::Ledger::NXS_COIN;

                    /* Get the trust account for the sender */
                    TAO::Register::Address hashTrust = TAO::Register::Address(std::string("trust"), hashSender, TAO::Register::Address::TRUST);

                    /* Get trust account. Any trust account that has completed Genesis will be indexed. */
                    TAO::Register::Object trust;

                    /* Check for trust index. */
                    if(!LLD::Register->ReadTrust(hashSender, trust)
                    && !LLD::Register->ReadState(hashTrust, trust))
                        continue;

                    /* Parse the object. */
                    if(!trust.Parse())
                        throw APIException(-71, "Unable to parse trust account.");

                    /* The amount staked */
                    uint64_t nStake = trust.get<uint64_t>("stake") ;

                    /* The sender's trust score as a %*/
                    double dTrustScore = (double)trust.get<uint64_t>("trust") / 10000000.0;

                    /* Only apply the trust weighting if they are staking */
                    if(nStake > 0 && dTrustScore > 0.5)
                    {
                        /* Cap the amount of NXS used in the vote to 10k NXS so that large accounts cannot dominate */
                        uint64_t nStakeCapped = std::min(nStake, 10000 * TAO::Ledger::NXS_COIN);

                        nVote = TAO::Ledger::NXS_COIN + ((nStakeCapped / 10000) * dTrustScore);  

                        // 1,000 NXS    @ 0.6%      = 1.06 votes
                        // 10,000 NXS   @ 0.6%      = 1.6 votes
                        // 10,000 NXS   @ 1%        = 2 votes
                        // 10,000 NXS   @ 3%        = 4 votes

                    }

                    ++nTotal;

                    /* Check the paged data. */
                    if(nCurrentPage < nPage)
                        continue;

                    if(nCurrentPage > nPage)
                        break;

                    if(nTotal - (nPage * nLimit) > nLimit)
                        break;

                    /* Build the transaction JSON. */
                    json::json jsonTx;

                    /* Read the block state from the the ledger DB using the transaction hash index */
                    TAO::Ledger::BlockState blockState;
                    LLD::Ledger->ReadBlock(tx.GetHash(), blockState);

                    /* Always add the transaction hash */
                    jsonTx["txid"] = tx.GetHash().GetHex();
                    jsonTx["timestamp"] = tx.nTimestamp;
                    jsonTx["genesis"] = hashSender.ToString();
                    jsonTx["vote"] = (double) nVote / TAO::Ledger::NXS_COIN;
                    jsonTx["confirmations"] = blockState.IsNull() ? 0 : TAO::Ledger::ChainState::nBestHeight.load() - blockState.nHeight + 1;

                    ret.push_back(jsonTx);

                    /* record the vote so that we don't get dupes */
                    vVotes.push_back(hashSender);
                }

                /* Set the next last. */
                hashLast = !tx.IsFirst() ? tx.hashPrevTx : 0;

            }

            return ret;
        }
    }
}