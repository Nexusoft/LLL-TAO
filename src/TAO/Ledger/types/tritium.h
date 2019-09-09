/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_TAO_LEDGER_TYPES_TRITIUM_H
#define NEXUS_TAO_LEDGER_TYPES_TRITIUM_H

#include <LLC/include/flkey.h>

#include <TAO/Register/types/stream.h>

#include <TAO/Ledger/types/block.h>
#include <TAO/Ledger/types/transaction.h>

#include <Util/templates/serialize.h>

/* Global TAO namespace. */
namespace TAO
{

    /* Ledger Layer namespace. */
    namespace Ledger
    {
        class BlockState;
        class SyncBlock;


        /** TritiumBlock
         *
         *  A tritium block contains references to the transactions in blocks.
         *  These are used to build the merkle tree for checking.
         *  Transactions are processed before block is recieved, and commit
         *  When a block is recieved to break up processing requirements.
         *
         **/
        class TritiumBlock : public Block
        {
        public:

            /** The Block's timestamp. This number is locked into the signature hash. **/
            uint64_t nTime;


            /** Verifier Transaction.
             *
             *  Transaction responsible for the block producer.
             *
             **/
            Transaction producer;


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


            /** Serialization **/
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
                READWRITE(vchBlockSig);

                READWRITE(producer);
                READWRITE(ssSystem);
                READWRITE(vOffsets);
                READWRITE(vtx);
            )


            /** The default constructor. **/
            TritiumBlock();


            /** Copy constructor from base block. **/
            TritiumBlock(const Block& block);


            /** Copy Constructor. **/
            TritiumBlock(const TritiumBlock& block);


            /** Copy Constructor. **/
            TritiumBlock(const BlockState& state);


            /** Copy Constructor. **/
            TritiumBlock(const SyncBlock& block);


            /** Default Destructor **/
            virtual ~TritiumBlock();


            /** Clone
             *
             *  Allows polymorphic copying of blocks
             *  Overridden to return an instance of the TritiumBlock class.
             *  Return-type covariance allows us to return the more derived type whilst
             *  still overriding the virtual base-class method
             *
             *  @return A pointer to a copy of this TritiumBlock.
             *
             **/
            virtual TritiumBlock* Clone() const override;


            /** SetNull
             *
             *  Set the block to Null state.
             *
             **/
            void SetNull() override;


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
             *  Check a tritium block for consistency.
             *
             **/
            bool Check() const override;


            /** Accept
             *
             *  Accept a tritium block with chain state parameters.
             *
             **/
            bool Accept() const override;


            /** CheckStake
             *
             *  Check the proof of stake calculations.
             *
             **/
            bool CheckStake() const;


            /** VerifyWork
             *
             *  Verify the work was completed by miners as advertised.
             *
             *  @return True if work is valid, false otherwise.
             *
             **/
            bool VerifyWork() const override;


            /** SignatureHash
             *
             *  Get the Signature Hash of the block. Used to verify work claims.
             *
             *  @return Returns a 1024-bit signature hash.
             *
             **/
            uint1024_t SignatureHash() const override;


            /** StakeHash
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
            std::string ToString() const override;


        };
    }
}

#endif
