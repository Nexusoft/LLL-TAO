/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

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

        /** The transactions of the block. **/
        std::vector<Transaction> vtx;


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
			if (!(nSerType & (SER_GETHASH | SER_BLOCKHEADERONLY)))
			{
				READWRITE(vtx);
				READWRITE(vchBlockSig);
			}
        )


        /** The default constructor. **/
        LegacyBlock()
        : Block()
        , vtx()
        {
            SetNull();
        }


        /** Copy Constructor. **/
        LegacyBlock(const LegacyBlock& block)
        : Block(block)
        , vtx(block.vtx)
        {

        }


        /** Check
         *
         *  Check a legacy block for consistency.
         *
         **/
        bool Check() const;


        /** Accept
         *
         *  Accept a legacy block with chain state parameters.
         *
         **/
        bool Accept();


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
        std::string ToString() const;


        /** print
         *
         *  For debugging purposes, printing the block to stdout
         *
         **/
        virtual void print() const;
    };
}

#endif
