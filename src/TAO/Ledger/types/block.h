/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_TAO_LEDGER_TYPES_BLOCK_H
#define NEXUS_TAO_LEDGER_TYPES_BLOCK_H

#include <LLC/types/uint1024.h>

#include <set>

//forward declerations for BigNum
namespace LLC
{
    class ECKey;
    class FLKey;
}

/* Global TAO namespace. */
namespace TAO
{

    /* Ledger Layer namespace. */
    namespace Ledger
    {

        /** Block
         *
         *  Nodes collect new transactions into a block, hash them into a hash tree,
         *  and scan through nonce values to make the block's hash satisfy validation
         *  requirements.
         *
         */
        class Block
        {
        public:

            /** The blocks version for. Useful for changing rules. **/
            uint32_t nVersion;


            /** The previous blocks hash. Used to chain blocks together. **/
            uint1024_t hashPrevBlock;


            /** The Merkle Root. A merkle tree of transaction hashes included in header. **/
            uint512_t hashMerkleRoot;


            /** The Block Channel. This number designates what validation algorithm is required. **/
            uint32_t nChannel;


            /** The Block's Height. This number tells what block number this is in the chain. **/
            uint32_t nHeight;


            /** The Block's Bits. This number is a compact representation of the required difficulty. **/
            uint32_t nBits;


            /** The Block's nOnce. This number is used to find the "winning" hash. **/
            uint64_t nNonce;


            /** The prime origin offsets. **/
            std::vector<uint8_t> vOffsets;


            /** The bytes holding the blocks signature. Signed by the block creator before broadcast. **/
            std::vector<uint8_t> vchBlockSig;


            /** MEMORY ONLY: list of missing transactions if processing failed. **/
            mutable std::vector<std::pair<uint8_t, uint512_t> > vMissing;



            /** The default constructor. Sets block state to Null. **/
            Block();


            /** A base constructor.
             *
             *  @param[in] nVersionIn The version to set block to
             *  @param[in] hashPrevBlockIn The previous block being linked to
             *  @param[in] nChannelIn The channel this block is being created for
             *  @param[in] nHeightIn The height this block is being created at.
             *
             **/
            Block(uint32_t nVersionIn, uint1024_t hashPrevBlockIn, uint32_t nChannelIn, uint32_t nHeightIn);


            /** Copy constructor. **/
            Block(const Block& block);


            /** Default Destructor **/
            virtual ~Block();


            /** Clone
             *
             *  Allows polymorphic copying of blocks
             *  Derived classes should override this and return an instance of the derived type.
             *
             *  @return A pointer to a copy of this Block.
             *
             **/
            virtual Block* Clone() const;


            /** SetNull
             *
             *  Set the block to Null state.
             *
             **/
            virtual void SetNull();


            /** Check
             *
             *  Check a block for consistency.
             *
             **/
            virtual bool Check() const;


            /** Accept
             *
             *  Accept a block with chain state parameters.
             *
             **/
            virtual bool Accept() const;


            /** SetChannel
             *
             *  Sets the channel for the block.
             *
             *  @param[in] nNewChannel The channel to set.
             *
             **/
            void SetChannel(uint32_t nNewChannel);


            /** GetChannel
             *
             *  Gets the channel the block belongs to.
             *
             *  @return The channel assigned. (uint32_t)
             *
             **/
            uint32_t GetChannel() const;


            /** IsNull
             *
             *  Checks the Null state of the block.
             *
             *  @return True if null, false otherwise.
             *
             **/
            bool IsNull() const;


            /** GetPrime
             *
             *  Get the Prime number for the block (hash + nNonce).
             *
             *  @return Returns a 1024-bit prime number.
             *
             **/
            uint1024_t GetPrime() const;


            /** ProofHash
             *
             *  Get the Proof Hash of the block. Used to verify work claims.
             *
             *  @return Returns a 1024-bit proof hash.
             *
             **/
            uint1024_t ProofHash() const;


            /** SignatureHash
             *
             *  Get the Signature Hash of the block. Used to verify work claims.
             *
             *  @return Returns a 1024-bit signature hash.
             *
             **/
            virtual uint1024_t SignatureHash() const;


            /** GetHash
             *
             *  Get the Hash of the block.
             *
             *  @return Returns a 1024-bit block hash.
             *
             **/
            uint1024_t GetHash() const;


            /** IsProofOfStake
             *
             *  @return True if the block is proof of stake, false otherwise.
             *
             **/
            bool IsProofOfStake() const;


            /** IsProofOfWork
             *
             *  @return True if the block is proof of work, false otherwise.
             *
             **/
            bool IsProofOfWork() const;


            /** IsPrivate
             *
             *  @return True if the block is private block.
             *
             **/
            bool IsPrivate() const;


            /** BuildMerkleTree
             *
             *  Build the merkle tree from the transaction list.
             *
             *  @param[in] vMerkleTree The list of hashes to build merkle tree with.
             *
             *  @return The 512-bit merkle root
             *
             **/
            uint512_t BuildMerkleTree(std::vector<uint512_t> vMerkleTree) const;


            /** ToString
             *
             *  For debugging Purposes seeing block state data dump
             *
             **/
            virtual std::string ToString() const ;

            /** print
             *
             *  Dump to the log file the raw block data
             *
             **/
            void print() const;


            /** VerifyWork
             *
             *  Verify the work was completed by miners as advertised.
             *
             *  @return True if work is valid, false otherwise.
             *
             **/
            virtual bool VerifyWork() const;


            /** Generate Signature
             *
             *  Sign the block with the key that found the block.
             *
             */
            bool GenerateSignature(const LLC::FLKey& key);


            /** Verify Signature
             *
             *  Check that the block signature is a valid signature.
             *
             **/
            bool VerifySignature(const LLC::FLKey& key) const;


            /** GenerateSignature
             *
             *  Generate the signature as the block finder.
             *
             *  @param[in] key The key object containing private key to make Signature.
             *
             *  @return True if the signature was made successfully, false otherwise.
             *
             **/
            bool GenerateSignature(const LLC::ECKey& key);


            /** VerifySignature
             *
             *  Verify that the signature included in block is valid.
             *
             *  @return True if signature is valid, false otherwise.
             *
             **/
            bool VerifySignature(const LLC::ECKey& key) const;


            /** Serialize
             *
             *  Convert the Header of a Block into a Byte Stream for
             *  Reading and Writing Across Sockets.
             *
             *  @return Returns a vector of serialized byte information.
             *
             **/
            std::vector<uint8_t> Serialize() const;


            /** Deserialize
             *
             *  Convert Byte Stream into Block Header.
             *
             *  @param[in] vData The byte stream containing block info.
             *
             **/
            void Deserialize(const std::vector<uint8_t>& vData);

        protected:


            /** StakeHash
             *
             *  Generates the StakeHash for this block from a uint256_t hashGenesis
             *
             *  @return 1024-bit stake hash
             *
             **/
            uint1024_t StakeHash(bool fIsGenesis, const uint256_t &hashGenesis) const;


            /** StakeHash
             *
             *  Generates the StakeHash for this block from a uint576_t trust key
             *
             *  @return 1024-bit stake hash
             *
             **/
            uint1024_t StakeHash(bool fIsGenesis, const uint576_t &trustKey) const;
        };
    }
}

#endif
