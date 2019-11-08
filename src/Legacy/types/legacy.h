/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LEGACY_TYPES_LEGACY_H
#define NEXUS_LEGACY_TYPES_LEGACY_H

#include <TAO/Ledger/types/block.h>
#include <Legacy/types/transaction.h>

#include <Util/templates/serialize.h>


/* Global Legacy namespace. */
namespace Legacy
{

    /** Legacy Block
     *
     *  A legacy block contains the transactions in the blocks.
     *  Processing is all done when a block is received.
     *
     **/
    class LegacyBlock : public TAO::Ledger::Block
    {
    public:

        /** The Block's timestamp. This number is locked into the signature hash. **/
        uint32_t nTime; //NOTE: this is 32-bit for backwards compatability


        /** The transactions of the block. **/
        std::vector<Transaction> vtx;


        //serialize methods
        IMPLEMENT_SERIALIZE
        (
            READWRITE(nVersion);
            READWRITE(hashPrevBlock);
            READWRITE(hashMerkleRoot);
            READWRITE(nChannel);
            READWRITE(nHeight);
            READWRITE(nBits);
            READWRITE(nNonce);
            READWRITE(nTime);

            // ConnectBlock depends on vtx following header to generate CDiskTxPos
            if(!(nSerType & (SER_GETHASH | SER_BLOCKHEADERONLY)))
            {
                READWRITE(vtx);
                READWRITE(vchBlockSig);
            }

            /* Offsets are serialized to disk. */
            if(nSerType & SER_LLD)
                READWRITE(vOffsets);
        )


        /** The default constructor. **/
        LegacyBlock();


        /** Copy Constructor. **/
        LegacyBlock(const LegacyBlock& block);


        /** Move Constructor. **/
        LegacyBlock(LegacyBlock&& block) noexcept;


        /** Copy Assignment. **/
        LegacyBlock& operator=(const LegacyBlock& block);


        /** Move Assignment. **/
        LegacyBlock& operator=(LegacyBlock&& block) noexcept;


        /** Default Destructor **/
        virtual ~LegacyBlock();


        /** Clone
        *
        *  Allows polymorphic copying of blocks
        *  Overridden to return an instance of the LegacyBlock class.
        *  Return-type covariance allows us to return the more derived type whilst
        *  still overriding the virtual base-class method
        *
        *  @return A pointer to a copy of this LegacyBlock.
        *
        **/
        virtual LegacyBlock* Clone() const
        {
            return new LegacyBlock(*this);
        };


        /** Copy Constructor. **/
        LegacyBlock(const TAO::Ledger::Block& block);


        /** Copy Constructor. **/
        LegacyBlock(const TAO::Ledger::BlockState& state);


        /** Copy Constructor. **/
        LegacyBlock(const TAO::Ledger::SyncBlock& block);


        /** SetNull
         *
         *  Set the block to Null state.
         *
         **/
        virtual void SetNull();


        /** UpdateTime
         *
         *  Update the blocks timestamp.
         *
         **/
        void UpdateTime();


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
         *  Check a legacy block for consistency.
         *
         **/
        virtual bool Check() const override;


        /** Accept
         *
         *  Accept a legacy block with chain state parameters.
         *
         **/
        virtual bool Accept() const override;


        /** Verify Stake
         *
         *  Check the proof of stake calculations.
         *
         **/
        bool VerifyStake() const;


        /** Check Stake
         *
         *  Check the proof of stake calculations.
         *
         **/
        bool CheckStake() const;


        /** Block Age
         *
         *  Get the current block age of the trust key.
         *
         **/
        bool BlockAge(uint32_t& nAge) const;


        /** Trust Score
         *
         *  Get the score of the current trust block.
         *
         **/
        bool TrustScore(uint32_t& nScore) const;


        /** SignatureHash
         *
         *  Get the Signature Hash of the block. Used to verify work claims.
         *
         *  @return Returns a 1024-bit signature hash.
         *
         **/
        virtual uint1024_t SignatureHash() const override;


        /** Stake Hash
         *
         *  Prove that you staked a number of seconds based on weight
         *
         *  @return 1024-bit stake hash
         *
         **/
        uint1024_t StakeHash() const;


        /** ToString
         *
         *  For debugging Purposes seeing block state data dump
         *
         **/
        virtual std::string ToString() const;


    };
}

#endif
