/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "Doubt is the precursor to fear" - Alex Hannold

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_TAO_LEDGER_TYPES_STATE_H
#define NEXUS_TAO_LEDGER_TYPES_STATE_H

#include <TAO/Ledger/types/tritium.h>

namespace Legacy
{
    class LegacyBlock;
}

/* Global TAO namespace. */
namespace TAO
{

    /* Ledger Layer namespace. */
    namespace Ledger
    {

        /** BlockState
         *
         *  This class is responsible for storing state variables
         *  that are chain specific for a specified block. These
         *  values are not recognized until this block is linked
         *  in the chain with all previous states before it.
         *
         **/
        class BlockState : public Block
        {
            /* Tell compiler we are overloading this virtual method. */
            using Block::ToString;

        public:
            static std::mutex STATE_MUTEX;

            /** The transaction history.
             *  uint8_t = TransactionType (per enum)
             *  uint512_t = Tx hash
             **/
            std::vector< std::pair<uint8_t, uint512_t> > vtx;


            /** The Trust of the Chain to this Block. */
            uint64_t nChainTrust;


            /** The Total NXS released to date */
            uint64_t nMoneySupply;


            /** The Total NXS mint. **/
            int32_t nMint;


            /** The height of this channel. */
            uint32_t nChannelHeight;


            /** The reserves that are released. */
            int64_t nReleasedReserve[3];


            /** Used to Iterate forward in the chain */
            uint1024_t hashNextBlock;


            /** Used to store the checkpoint. **/
            uint1024_t hashCheckpoint;


            /* Serialization Macros */
            IMPLEMENT_SERIALIZE
            (
                READWRITE(nVersion);
                READWRITE(hashPrevBlock);
                READWRITE(hashNextBlock);
                READWRITE(hashMerkleRoot);
                READWRITE(nChannel);
                READWRITE(nHeight);
                READWRITE(nBits);
                READWRITE(nNonce);
                READWRITE(nTime);

                READWRITE(nChainTrust);
                READWRITE(nMoneySupply);
                READWRITE(nMint);
                READWRITE(nChannelHeight);
                READWRITE(nReleasedReserve[0]);
                READWRITE(nReleasedReserve[1]);
                READWRITE(nReleasedReserve[2]);
                READWRITE(hashCheckpoint);

                READWRITE(vchBlockSig);
                READWRITE(vtx);
            )


            /** Default Constructor. **/
            BlockState()
            : Block()
            , vtx()
            , nChainTrust(0)
            , nMoneySupply(0)
            , nMint(0)
            , nChannelHeight(0)
            , nReleasedReserve{0, 0, 0}
            , hashNextBlock(0)
            , hashCheckpoint(0)
            {
                SetNull();
            }


            /** Default Constructor. **/
            BlockState(const TritiumBlock& block);


            /** Default Constructor. **/
            BlockState(const Legacy::LegacyBlock& block);


            /** Virtual Destructor. **/
            virtual ~BlockState() { }


            /** Copy Constructor. **/
            BlockState(const BlockState& state)
            : Block(state)
            {
                vtx                 = state.vtx;

                nChainTrust         = state.nChainTrust;
                nMoneySupply        = state.nMoneySupply;
                nMint               = state.nMint;
                nChannelHeight      = state.nChannelHeight;

                nReleasedReserve[0] = state.nReleasedReserve[0];
                nReleasedReserve[1] = state.nReleasedReserve[1];
                nReleasedReserve[2] = state.nReleasedReserve[2];

                hashNextBlock       = state.hashNextBlock;
                hashCheckpoint      = state.hashCheckpoint;
            }


            /** Copy Assignment Operator. **/
            BlockState& operator=(const BlockState& state)
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
                nMint               = state.nMint;
                nChannelHeight      = state.nChannelHeight;

                nReleasedReserve[0] = state.nReleasedReserve[0];
                nReleasedReserve[1] = state.nReleasedReserve[1];
                nReleasedReserve[2] = state.nReleasedReserve[2];

                hashNextBlock       = state.hashNextBlock;
                hashCheckpoint      = state.hashCheckpoint;

                return *this;
            }


            /** Equivilence checking **/
            bool operator==(const BlockState& state) const
            {
                return GetHash() == state.GetHash();
            }


            /** Equivilence checking **/
            bool operator!=(const BlockState& state) const
            {
                return GetHash() != state.GetHash();
            }


            /** Not operator overloading. **/
            bool operator !(void)
            {
                return IsNull();
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


            /** Index
             *
             *  Index a block state into chain.
             *
             *  @return true if accepted.
             *
             **/
            bool Index();


            /** Set Best
             *
             *  Set this state as the best chain.
             *
             *  @return true if accepted.
             *
             **/
            bool SetBest();


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


            /** GetBlockTrust
             *
             *  Get the trust of this block.
             *
             *  @return the current trust in the chain.
             *
             **/
            uint64_t GetBlockTrust() const;


            /** IsInMainChain
             *
             *  Function to determine if this block has been connected into the main chain.
             *
             *  @return true if in the main chain.
             *
             **/
            bool IsInMainChain() const;


            /** ToString
             *
             *  For debugging Purposes seeing block state data dump.
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
            virtual void print() const;


            /** StakeHash
             *
             *  Prove that you staked a number of seconds based on weight.
             *
             *  @return 1024-bit stake hash.
             *
             **/
            uint1024_t StakeHash() const;

        };


        /** GetLastState
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
}

#endif
