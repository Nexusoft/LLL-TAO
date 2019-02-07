/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_TAO_LEDGER_TYPES_TRITIUM_H
#define NEXUS_TAO_LEDGER_TYPES_TRITIUM_H

#include <TAO/Ledger/types/block.h>
#include <TAO/Ledger/types/transaction.h>

#include <Util/templates/serialize.h>

/* Global TAO namespace. */
namespace TAO
{

    /* Ledger Layer namespace. */
    namespace Ledger
    {

        /** Defines the types of transaction hash stored in the TritiumBlock vtx **/
        enum TYPE
        {
            LEGACY_TX  = 0x00,
            TRITIUM_TX = 0x01,
            CHECKPOINT = 0x02, //for private chain checkpointing into mainnet blocks.
        };


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

            /** Verifier Transaction.
             *
             *  Transaction responsible for the block producer.
             *
             **/
            Transaction producer;


            /** The transaction history.
             *  uint8_t = TransactionType (per enum)
             *  uint512_t = Tx hash
             **/
            std::vector< std::pair<uint8_t, uint512_t> > vtx;


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
                READWRITE(vtx);
            )


            /** The default constructor. **/
            TritiumBlock()
            : Block()
            , producer()
            , vtx()
            {
                SetNull();
            }


            /** Copy Constructor. **/
            TritiumBlock(const TritiumBlock& block)
            : Block(block)
            , producer(block.producer)
            , vtx(block.vtx)
            {

            }


            /** Copy Constructor. **/
            TritiumBlock(const BlockState& state);

            /** Default Destructor **/
            virtual ~TritiumBlock()
            {
            }


            /** Check
             *
             *  Check a tritium block for consistency.
             *
             **/
            bool Check() const;


            /** Accept
             *
             *  Accept a tritium block with chain state parameters.
             *
             **/
            bool Accept();


            /** CheckStake
             *
             *  Check the proof of stake calculations.
             *
             **/
            bool CheckStake() const;


            /** CheckTrust
             *
             *  Check the calculated trust score meets published one.
             *
             **/
            bool CheckTrust() const;


            /** BlockAge
             *
             *  Get the current block age of the trust key.
             *
             **/
            bool BlockAge(uint32_t& nAge) const;


            /** TrustScore
             *
             *  Get the score of the current trust block.
             *
             **/
            bool TrustScore(uint32_t& nScore) const;


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
            std::string ToString() const;


            /** print
             *
             *  For debugging purposes, printing the block to stdout
             *
             **/
            virtual void print() const;
        };
    }
}

#endif
