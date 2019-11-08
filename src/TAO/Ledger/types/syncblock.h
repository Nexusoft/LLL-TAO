/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_TAO_LEDGER_TYPES_SYNCBLOCK_H
#define NEXUS_TAO_LEDGER_TYPES_SYNCBLOCK_H

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


        /** TritiumBlock
         *
         *  A tritium block contains references to the transactions in blocks.
         *  These are used to build the merkle tree for checking.
         *  Transactions are processed before block is recieved, and commit
         *  When a block is recieved to break up processing requirements.
         *
         **/
        class SyncBlock : public Block
        {
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
             *  std::vector = Serialized byte level data
             **/
            std::vector<std::pair<uint8_t, std::vector<uint8_t> > > vtx;


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

                READWRITE(ssSystem);
                READWRITE(vOffsets);
                READWRITE(vtx);
            )


            /** The default constructor. **/
            SyncBlock();


            /** Copy constructor. **/
            SyncBlock(const SyncBlock& block);


            /** Move constructor. **/
            SyncBlock(SyncBlock&& block) noexcept;


            /** Copy assignment. **/
            SyncBlock& operator=(const SyncBlock& block);


            /** Move assignment. **/
            SyncBlock& operator=(SyncBlock&& block) noexcept;


            /** Default Destructor **/
            virtual ~SyncBlock();


            /** Copy Constructor. **/
            SyncBlock(const BlockState& state);


            /** SignatureHash
             *
             *  Get the Signature Hash of the block. Used to verify work claims.
             *
             *  @return Returns a 1024-bit signature hash.
             *
             **/
            virtual uint1024_t SignatureHash() const override;

        };
    }
}

#endif
