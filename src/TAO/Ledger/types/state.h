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

            /** The Block's timestamp. This number is locked into the signature hash. **/
            uint64_t nTime;


            /** System Script
             *
             *  The critical system level pre-states and post-states.
             *
             **/
            TAO::Register::Stream  ssSystem;


            /** The transaction history.
             *  uint8_t = TransactionType (per enum)
             *  uint512_t = Tx hash
             **/
            std::vector<std::pair<uint8_t, uint512_t> > vtx;


            /** The Trust of the Chain to this Block. */
            uint64_t nChainTrust;


            /** The Total NXS released to date */
            uint64_t nMoneySupply;


            /** The Total NXS mint. **/
            int32_t nMint;


            /** The Total Fees in block. **/
            uint64_t nFees;


            /** The height of this channel. */
            uint32_t nChannelHeight;


            /** The weight of this channel. */
            uint128_t nChannelWeight[3];


            /** The reserves that are released. */
            int64_t nReleasedReserve[3];


            /** The reserves that are released. */
            uint64_t nFeeReserve;


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

                /* Tritium Block States. */
                READWRITE(nFees);
                READWRITE(nChannelWeight[0]);
                READWRITE(nChannelWeight[1]);
                READWRITE(nChannelWeight[2]);
                READWRITE(nFeeReserve);

                /* Reserves. */
                READWRITE(nReleasedReserve[0]);
                READWRITE(nReleasedReserve[1]);
                READWRITE(nReleasedReserve[2]);
                READWRITE(hashCheckpoint);

                if(!(nSerType & SER_BLOCKHEADERONLY))
                {
                    READWRITE(vchBlockSig);
                    READWRITE(ssSystem);
                }

                READWRITE(vOffsets);

                if(!(nSerType & SER_BLOCKHEADERONLY))
                {
                    READWRITE(vtx);
                }
            )


            /** Default Constructor. **/
            BlockState();


            /** Copy constructor. **/
            BlockState(const BlockState& block);


            /** Move constructor. **/
            BlockState(BlockState&& block) noexcept;


            /** Copy assignment. **/
            BlockState& operator=(const BlockState& block);


            /** Move assignment. **/
            BlockState& operator=(BlockState&& block) noexcept;


            /** Default Destructor **/
            virtual ~BlockState();


            /** Default Constructor. **/
            BlockState(const TritiumBlock& block);


            /** Default Constructor. **/
            BlockState(const Legacy::LegacyBlock& block);


            /** Equivilence checking **/
            bool operator==(const BlockState& state) const;


            /** Equivilence checking **/
            bool operator!=(const BlockState& state) const;


            /** Not operator overloading. **/
            bool operator!(void) const;


            /** GetBlockTime
             *
             *  Returns the current UNIX timestamp of the block.
             *
             *  @return 64-bit integer of timestamp.
             *
             **/
            uint64_t GetBlockTime() const;


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


            /** Trust
             *
             *  Get the trust of this block.
             *
             *  @return the current trust in the chain.
             *
             **/
            uint64_t Trust() const;


            /** Weight
             *
             *  Get the weight of this block.
             *
             *  @return the current weight for this block.
             *
             **/
            uint64_t Weight() const;


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


            /** SignatureHash
             *
             *  Get the Signature Hash of the block. Used to verify work claims.
             *
             *  @return Returns a 1024-bit signature hash.
             *
             **/
            uint1024_t SignatureHash() const;


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
