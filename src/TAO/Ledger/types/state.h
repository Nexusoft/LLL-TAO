/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_TAO_LEDGER_TYPES_STATE_H
#define NEXUS_TAO_LEDGER_TYPES_STATE_H

#include <TAO/Ledger/types/tritium.h>

namespace TAO::Ledger
{
    /** Block State Class
     *
     *  This class is responsible for storing state variables
     *  that are chain specific for a specified block. These
     *  values are not recognized until this block is linked
     *  in the chain with all previous states before it.
     *
     **/
    class BlockState : public TritiumBlock
    {
    public:


        /** The Trust of the Chain to this Block. */
        uint64_t nChainTrust;


        /** The Total NXS released to date */
        uint64_t nMoneySupply;


        /** The height of this channel. */
        uint32_t nChannelHeight;


        /** The reserves that are released. */
        uint32_t nReleasedReserve[3];


        /** Used to Iterate forward in the chain */
        uint1024_t hashNextBlock;


        /** Used to store the checkpoint. **/
        uint1024_t hashCheckpoint;


        /* Serialization Macros */
        IMPLEMENT_SERIALIZE
        (
            BlockState* pthis = const_cast<BlockState*>(this);
            TritiumBlock* block = (TritiumBlock*)pthis;

            READWRITE(*block);
            READWRITE(nChainTrust);
            READWRITE(nMoneySupply);
            READWRITE(nChannelHeight);
            READWRITE(nReleasedReserve[0]);
            READWRITE(nReleasedReserve[1]);
            READWRITE(nReleasedReserve[2]);

            //for disk operations only
            READWRITE(hashNextBlock);
            READWRITE(hashCheckpoint);
        )


        BlockState()
        : TritiumBlock()
        , nChainTrust(0)
        , nMoneySupply(0)
        , nChannelHeight(0)
        , nReleasedReserve{0, 0, 0}
        , hashNextBlock(0)
        , hashCheckpoint(0)
        {
        }


        BlockState(TritiumBlock blockIn)
        : TritiumBlock(blockIn)
        , nChainTrust(0)
        , nMoneySupply(0)
        , nChannelHeight(0)
        , nReleasedReserve{0, 0, 0}
        , hashNextBlock(0)
        , hashCheckpoint(0)
        {
        }


        /** Copy Constructor. **/
        BlockState(const BlockState& state)
        {
            nVersion            = state.nVersion;
            hashPrevBlock       = state.hashPrevBlock;
            hashMerkleRoot      = state.hashMerkleRoot;
            nChannel            = state.nChannel;
            nHeight             = state.nHeight;
            nBits               = state.nBits;
            nNonce              = state.nNonce;
            nTime               = state.nTime;
            vchBlockSig         = state.vchBlockSig;
            vtx                 = state.vtx;

            nChainTrust         = state.nChainTrust;
            nMoneySupply        = state.nMoneySupply;
            nChannelHeight      = state.nChannelHeight;

            nReleasedReserve[0] = state.nReleasedReserve[0];
            nReleasedReserve[1] = state.nReleasedReserve[1];
            nReleasedReserve[2] = state.nReleasedReserve[2];

            hashNextBlock       = state.hashNextBlock;
            hashCheckpoint      = state.hashCheckpoint;
        }


        /** Equivilence checking **/
        bool operator==(const BlockState state)
        {
            return     state.GetHash() == GetHash();
        }

        /** Equivilence checking **/
        bool operator!=(const BlockState state)
        {
            return     state.GetHash() != GetHash();
        }


        /** Not operator overloading. **/
        bool const operator ! (void)
        {
            return !IsNull();
        }


        /** Prev
         *
         *  Get the previous block state in chain.
         *
         *  @return The previous block state.
         *
         **/
        BlockState Prev() const;


        /** Next
         *
         *  Get the next block state in chain.
         *
         *  @return The next block state.
         *
         **/
        BlockState Next() const;


        /** Accept
         *
         *  Accept a block state into chain.
         *
         *  @return true if accepted.
         *
         **/
        bool Accept();


        /** Connect
         *
         *  Connect a block state into chain.
         *
         *  @return true if connected.
         *
         **/
        bool Connect();


        /** Disconnect
         *
         *  Remove a block state from the chain.
         *
         *  @return true if disconnected.
         *
         **/
        bool Disconnect();


        /** Get Block Trust
         *
         *  Get the trust of this block.
         *
         *  @return the current trust in the chain.
         *
         **/
        uint64_t GetBlockTrust() const;


        /** Is In Main Chain
         *
         *  Function to determine if this block has been connected into the main chain.
         *
         *  @return true if in the main chain.
         *
         **/
        bool IsInMainChain() const;


        /** To String
         *
         *  For debugging Purposes seeing block state data dump
         *
         *  @param[in] nState The states to output.
         *
         *  @return the string with output data.
         *
         **/
        std::string ToString(uint8_t nState = debug::flags::header) const;


        /** print
         *
         *  For debugging to dump state to console.
         *
         *  @param[in] nState The states to output.
         *
         **/
        void print(uint8_t nState = debug::flags::header) const;
    };


    /** Get Last State
     *
     *  Gets a block state by channel from hash.
     *
     *  @param[in] state The block to search from.
     *  @param[in] nChannel The channel to search for.
     *
     *  @return The block state found.
     *
     **/
    bool GetLastState(BlockState& state, uint32_t nChannel);

}

#endif
