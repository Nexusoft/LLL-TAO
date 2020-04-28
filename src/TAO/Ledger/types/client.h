/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "Doubt is the precursor to fear" - Alex Hannold

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_TAO_LEDGER_TYPES_CLIENT_H
#define NEXUS_TAO_LEDGER_TYPES_CLIENT_H

#include <TAO/Register/types/stream.h>

#include <TAO/Ledger/types/block.h>

/* Global TAO namespace. */
namespace TAO
{

    /* Ledger Layer namespace. */
    namespace Ledger
    {
        class BlockState;

        /** ClientBlock
         *
         *  This class is responsible for storing state variables
         *  that are chain specific for a specified block. These
         *  values are not recognized until this block is linked
         *  in the chain with all previous states before it.
         *
         **/
        class ClientBlock : public Block
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


            /** The Total NXS released to date */
            uint64_t nMoneySupply;


            /** The height of this channel. */
            uint32_t nChannelHeight;


            /** The weight of this channel. */
            uint128_t nChannelWeight[3];


            /** The reserves that are released. */
            int64_t nReleasedReserve[3];


            /** Used to Iterate forward in the chain */
            uint1024_t hashNextBlock;


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

                READWRITE(nMoneySupply);
                READWRITE(nChannelHeight);
                READWRITE(nChannelWeight[0]);
                READWRITE(nChannelWeight[1]);
                READWRITE(nChannelWeight[2]);
                READWRITE(nReleasedReserve[0]);
                READWRITE(nReleasedReserve[1]);
                READWRITE(nReleasedReserve[2]);

                READWRITE(ssSystem);
                READWRITE(vOffsets);
            )


            /** Default Constructor. **/
            ClientBlock();


            /** Copy constructor. **/
            ClientBlock(const ClientBlock& block);


            /** Move constructor. **/
            ClientBlock(ClientBlock&& block) noexcept;


            /** Copy assignment. **/
            ClientBlock& operator=(const ClientBlock& block);


            /** Move assignment. **/
            ClientBlock& operator=(ClientBlock&& block) noexcept;


            /** Copy Constructor from state. **/
            ClientBlock(const BlockState& block);


            /** Move Constructor from state. **/
            ClientBlock(BlockState&& block);


            /** Copy assignment from state. **/
            ClientBlock& operator=(const BlockState& block);


            /** Move assignment from state. **/
            ClientBlock& operator=(BlockState&& block) noexcept;


            /** Default Destructor **/
            virtual ~ClientBlock();


            /** Equivilence checking **/
            bool operator==(const ClientBlock& block) const;


            /** Equivilence checking **/
            bool operator!=(const ClientBlock& block) const;


            /** Not operator overloading. **/
            bool operator!(void) const;


            /** Clone
             *
             *  Allows polymorphic copying of blocks
             *  Overridden to return an instance of the BlockState class.
             *  Return-type covariance allows us to return the more derived type whilst
             *  still overriding the virtual base-class method
             *
             *  @return A pointer to a copy of this BlockState.
             *
             **/
            virtual ClientBlock* Clone() const override;


            /** Prev
             *
             *  Get the previous client block in chain.
             *
             *  @return The previous client block.
             *
             **/
            ClientBlock Prev() const;


            /** Next
             *
             *  Get the next client block in chain.
             *
             *  @return The next client block.
             *
             **/
            ClientBlock Next() const;


            /** GetBlockTime
             *
             *  Returns the current UNIX timestamp of the block.
             *
             *  @return 64-bit integer of timestamp.
             *
             **/
            uint64_t GetBlockTime() const;


            /** Check
             *
             *  Check a client block for consistency.
             *
             **/
            bool Check() const override;


            /** Accept
             *
             *  Accept a client block with chain state parameters.
             *
             **/
            bool Accept() const override;


            /** Index
             *
             *  Index a client block into chain.
             *
             *  @return true if accepted.
             *
             **/
            bool Index() const;


            /** Set Best
             *
             *  Set this state as the best chain.
             *
             *  @return true if accepted.
             *
             **/
            bool SetBest() const;


            /** Connect
             *
             *  Connect a client block into chain.
             *
             *  @return true if connected.
             *
             **/
            bool Connect() const;


            /** Disconnect
             *
             *  Remove a client block from the chain.
             *
             *  @return true if disconnected.
             *
             **/
            bool Disconnect() const;



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
             *  For debugging Purposes seeing client block data dump.
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

        };


        /** GetLastState
         *
         *  Gets a client block by channel from hash.
         *
         *  @param[in] state The block to search from.
         *  @param[in] nChannel The channel to search for.
         *
         *  @return The client block found.
         *
         **/
        bool GetLastState(ClientBlock& state, const uint32_t nChannel);
    }
}

#endif
