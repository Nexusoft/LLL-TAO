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



namespace TAO::Ledger
{

    /* Get the block state object. */
    bool GetLastState(BlockState& state, uint32_t nChannel)
    {

    }


    /* Get the previous block state in chain. */
    BlockState BlockState::Prev()
    {
        BlockState state;
        if(!LLD::legDB->ReadBlock(hashPrevBlock, state))
            return state;

        return state;
    }


    /* Get the next block state in chain. */
    BlockState BlockState::Next()
    {
        BlockState state;
        if(hashNextBlock == 0)
            return state;

        if(!LLD::legDB->ReadBlock(hashNextBlock, state))
            return state;

        return state;
    }

    /** Function to determine if this block has been connected into the main chain. **/
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
            nHeight, GetDifficulty(nBits, nChannel), nNonce, nTime, HexStr(vchBlockSig.begin(), vchBlockSig.end()));
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
