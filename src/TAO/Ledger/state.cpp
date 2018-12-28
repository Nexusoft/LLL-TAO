/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To The Voice of The People

____________________________________________________________________________________________*/

#include <string>

#include <LLD/include/global.h>

#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/types/state.h>

#include <TAO/Ledger/include/difficulty.h>
#include <TAO/Ledger/include/checkpoints.h>
#include <TAO/Ledger/include/supply.h>


namespace TAO::Ledger
{

    /* Get the block state object. */
    bool GetLastState(BlockState& state, uint32_t nChannel)
    {

    }


    /* Get the previous block state in chain. */
    BlockState BlockState::Prev() const
    {
        BlockState state;
        if(!LLD::legDB->ReadBlock(hashPrevBlock, state))
            return state;

        return state;
    }


    /* Get the next block state in chain. */
    BlockState BlockState::Next() const
    {
        BlockState state;
        if(hashNextBlock == 0)
            return state;

        if(!LLD::legDB->ReadBlock(hashNextBlock, state))
            return state;

        return state;
    }

    /* Accept a block state into chain. */
    bool BlockState::Accept()
    {
        /* Read leger DB for duplicate block. */
        BlockState state;
        if(LLD::legDB->ReadBlock(GetHash(), state))
            return debug::error(FUNCTION "block state already exists", __PRETTY_FUNCTION__);

        /* Read leger DB for previous block. */
        BlockState statePrev = Prev();
        if(statePrev.IsNull())
            return debug::error(FUNCTION "previous block state not found", __PRETTY_FUNCTION__);

        /* Check the Height of Block to Previous Block. */
        if(statePrev.nHeight + 1 != nHeight)
            return debug::error(FUNCTION "incorrect block height.", __PRETTY_FUNCTION__);

        /* Get the proof hash for this block. */
        uint1024_t hash = (nVersion < 5 ? GetHash() : GetChannel() == 0 ? StakeHash() : ProofHash());

        /* Get the target hash for this block. */
        uint1024_t hashTarget = LLC::CBigNum().SetCompact(nBits).getuint1024();

        /* Verbose logging of proof and target. */
        debug::log(2, "  proof:  %s", hash.ToString().substr(0, 30).c_str());

        /* Channel switched output. */
        if(GetChannel() == 1)
            debug::log(2, "  prime cluster verified of size %f", GetDifficulty(nBits, 1));
        else
            debug::log(2, "  target: %s", hashTarget.ToString().substr(0, 30).c_str());


        /* Check that the nBits match the current Difficulty. **/
        if (nBits != GetNextTargetRequired(statePrev, GetChannel()))
            return debug::error(FUNCTION "incorrect proof-of-work/proof-of-stake", __PRETTY_FUNCTION__);


        /* Check That Block timestamp is not before previous block. */
        if (GetBlockTime() <= statePrev.GetBlockTime())
            return debug::error(FUNCTION "block's timestamp too early Block: %" PRId64 " Prev: %" PRId64 "", __PRETTY_FUNCTION__,
            GetBlockTime(), statePrev.GetBlockTime());


        /* Check that Block is Descendant of Hardened Checkpoints. */
        if(!ChainState::Synchronizing() && !IsDescendant(statePrev))
            return debug::error(FUNCTION "not descendant of last checkpoint", __PRETTY_FUNCTION__);


        /* Check the block proof of work rewards. */
        if(IsProofOfWork() && nVersion >= 3)
        {
            /* Get the stream from coinbase. */
            DataStream ssData(producer.vchLedgerData, SER_OPERATIONS, producer.nVersion);
            ssData.SetPos(33); //set the read position to where reward will be.

            /* Read the mining reward. */
            uint64_t nMiningReward;
            ssData >> nMiningReward;

            /* Check that the Mining Reward Matches the Coinbase Calculations. */
            if (nMiningReward != GetCoinbaseReward(statePrev, GetChannel(), 0))
                return debug::error(FUNCTION "miner reward mismatch %" PRId64 " : %" PRId64 "",
                    __PRETTY_FUNCTION__, nMiningReward, GetCoinbaseReward(statePrev, GetChannel(), 0));
        }
        else if (IsProofOfStake())
        {
            /* Check that the Coinbase / CoinstakeTimstamp is after Previous Block. */
            if (producer.nTimestamp < statePrev.GetBlockTime())
                return debug::error(FUNCTION "coinstake transaction too early", __PRETTY_FUNCTION__);
        }


        //TODO: check legacy transactions for finality

        /* Compute the Chain Trust */
        nChainTrust = statePrev.nChainTrust + GetBlockTrust();


        /* Compute the Channel Height. */
        BlockState stateLast = statePrev;
        if(!GetLastState(stateLast, GetChannel()))
            nChannelHeight = 1;
        else
            nChannelHeight = stateLast.nChannelHeight + 1;


        /* Compute the Released Reserves. */
        for(int nType = 0; nType < 3; nType++)
        {
            if(IsProofOfWork())
            {
                /* Calculate the Reserves from the Previous Block in Channel's reserve and new Release. */
                uint64_t nReserve  = stateLast.nReleasedReserve[nType] +
                    GetReleasedReserve(*this, GetChannel(), nType);

                /* Get the coinbase rewards. */
                uint64_t nCoinbaseRewards = GetCoinbaseReward(statePrev, GetChannel(), nType);

                /* Block Version 3 Check. Disable Reserves from going below 0. */
                if(nVersion >= 3 && nCoinbaseRewards >= nReserve)
                    return debug::error(FUNCTION "out of reserve limits", __PRETTY_FUNCTION__);

                /* Check coinbase rewards. */
                nReleasedReserve[nType] =  nReserve - nCoinbaseRewards;

                debug::log(2, "Reserve Balance %i | %f Nexus | Released %f", nType, stateLast.nReleasedReserve[nType] / 1000000.0, (nReserve - stateLast.nReleasedReserve[nType]) / 1000000.0 );
            }
            else
                nReleasedReserve[nType] = 0;

        }

        /* Add the Pending Checkpoint into the Blockchain. */
        if(IsNewTimespan(Prev()))
        {
            hashCheckpoint = GetHash();

            debug::log(0, "===== New Pending Checkpoint Hash = %s", hashCheckpoint.ToString().substr(0, 15).c_str());
        }
        else
        {
            hashCheckpoint = stateLast.hashCheckpoint;

            debug::log(0, "===== Pending Checkpoint Hash = %s", hashCheckpoint.ToString().substr(0, 15).c_str());
        }

        return true;
    }


    /* USed to determine the trust of a block in the chain. */
    uint64_t BlockState::GetBlockTrust() const
    {
        /** Give higher block trust if last block was of different channel **/
        BlockState prev = Prev();
        if(!prev.IsNull() && prev.GetChannel() != GetChannel())
            return 3;

        return 1;
    }


    /* Function to determine if this block has been connected into the main chain. */
    bool BlockState::IsInMainChain() const
    {
        return (hashNextBlock != 0 || GetHash() == ChainState::hashBestChain);
    }


    /* For debugging Purposes seeing block state data dump */
    std::string BlockState::ToString() const
    {
        return std::string(""); //TODO - replace placeholder
    }


    /* For debugging purposes, printing the block to stdout */
    void BlockState::print(uint8_t nState) const
    {
        std::string strDebug = "";

        /* Handle verbose output for just header. */
        if(nState & debug::flags::header)
        {
            strDebug += debug::strprintf("Block("
            ANSI_COLOR_BRIGHT_WHITE "nVersion" ANSI_COLOR_RESET " = %u, "
            ANSI_COLOR_BRIGHT_WHITE "hashPrevBlock" ANSI_COLOR_RESET " = %s, "
            ANSI_COLOR_BRIGHT_WHITE "hashMerkleRoot" ANSI_COLOR_RESET " = %s, "
            ANSI_COLOR_BRIGHT_WHITE "nChannel" ANSI_COLOR_RESET " = %u, "
            ANSI_COLOR_BRIGHT_WHITE "nHeight" ANSI_COLOR_RESET " = %u, "
            ANSI_COLOR_BRIGHT_WHITE "nDiff" ANSI_COLOR_RESET " = %f, "
            ANSI_COLOR_BRIGHT_WHITE "nNonce" ANSI_COLOR_RESET " = %" PRIu64 ", "
            ANSI_COLOR_BRIGHT_WHITE "nTime" ANSI_COLOR_RESET " = %u, "
            ANSI_COLOR_BRIGHT_WHITE "blockSig" ANSI_COLOR_RESET " = %s",
            nVersion, hashPrevBlock.ToString().substr(0, 20).c_str(),
            hashMerkleRoot.ToString().substr(0, 20).c_str(), nChannel,
            nHeight, GetDifficulty(nBits, nChannel), nNonce, nTime, HexStr(vchBlockSig.begin(), vchBlockSig.end()).c_str());
        }

        /* Handle the verbose output for chain state. */
        if(nState & debug::flags::chain)
        {
            strDebug += debug::strprintf(", "
            ANSI_COLOR_BRIGHT_WHITE "nChainTrust" ANSI_COLOR_RESET " = %" PRIu64 ", "
            ANSI_COLOR_BRIGHT_WHITE "nMoneySupply" ANSI_COLOR_RESET " = %" PRIu64 ", "
            ANSI_COLOR_BRIGHT_WHITE "nChannelHeight" ANSI_COLOR_RESET " = %" PRIu32 ", "
            ANSI_COLOR_BRIGHT_WHITE "nMinerReserve" ANSI_COLOR_RESET " = %" PRIu32 ", "
            ANSI_COLOR_BRIGHT_WHITE "nAmbassadorReserve" ANSI_COLOR_RESET " = %" PRIu32 ", "
            ANSI_COLOR_BRIGHT_WHITE "nDeveloperReserve" ANSI_COLOR_RESET " = %" PRIu32 ", "
            ANSI_COLOR_BRIGHT_WHITE "hashNextBlock" ANSI_COLOR_RESET " = %s, "
            ANSI_COLOR_BRIGHT_WHITE "hashCheckpoint" ANSI_COLOR_RESET " = %s",
            nChainTrust, nMoneySupply, nChannelHeight, nReleasedReserve[0], nReleasedReserve[1],
            nReleasedReserve[2], hashNextBlock.ToString().substr(0, 20).c_str(),
            hashCheckpoint.ToString().substr(0, 20).c_str());
        }
        strDebug += ")\n";

        /* Handle the verbose output for transactions. */
        if(nState & debug::flags::tx)
        {
            for(auto tx : vtx)
                strDebug += debug::strprintf("Proof(nType = %u, hash = %s)\n", tx.first, tx.second);
        }

        debug::log(0, strDebug.c_str());
    }

}
