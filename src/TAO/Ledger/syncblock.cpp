/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/Register/types/stream.h>

#include <TAO/Ledger/types/syncblock.h>
#include <TAO/Ledger/types/state.h>

#include <Util/templates/datastream.h>

/* Global TAO namespace. */
namespace TAO
{

    /* Ledger Layer namespace. */
    namespace Ledger
    {

        /* Default constructor. */
        SyncBlock::SyncBlock()
        : Block    ( )
        , nTime    (runtime::unifiedtimestamp())
        , ssSystem ( )
        , vtx      ( )
        {
        }


        /* Copy constructor. */
        SyncBlock::SyncBlock(const SyncBlock& block)
        : Block    (block)
        , nTime    (block.nTime)
        , ssSystem (block.ssSystem)
        , vtx      (block.vtx)
        {
        }


        /* Move constructor. */
        SyncBlock::SyncBlock(SyncBlock&& block) noexcept
        : Block    (std::move(block))
        , nTime    (std::move(block.nTime))
        , ssSystem (std::move(block.ssSystem))
        , vtx      (std::move(block.vtx))
        {
        }


        /* Copy assignment. */
        SyncBlock& SyncBlock::operator=(const SyncBlock& block)
        {
            nVersion       = block.nVersion;
            hashPrevBlock  = block.hashPrevBlock;
            hashMerkleRoot = block.hashMerkleRoot;
            nChannel       = block.nChannel;
            nHeight        = block.nHeight;
            nBits          = block.nBits;
            nNonce         = block.nNonce;
            vOffsets       = block.vOffsets;
            vchBlockSig    = block.vchBlockSig;
            vMissing       = block.vMissing;
            hashMissing    = block.hashMissing;
            fConflicted    = block.fConflicted;

            nTime          = block.nTime;
            ssSystem       = block.ssSystem;
            vtx            = block.vtx;

            return *this;
        }


        /* Move assignment. */
        SyncBlock& SyncBlock::operator=(SyncBlock&& block) noexcept
        {
            nVersion       = std::move(block.nVersion);
            hashPrevBlock  = std::move(block.hashPrevBlock);
            hashMerkleRoot = std::move(block.hashMerkleRoot);
            nChannel       = std::move(block.nChannel);
            nHeight        = std::move(block.nHeight);
            nBits          = std::move(block.nBits);
            nNonce         = std::move(block.nNonce);
            vOffsets       = std::move(block.vOffsets);
            vchBlockSig    = std::move(block.vchBlockSig);
            vMissing       = std::move(block.vMissing);
            hashMissing    = std::move(block.hashMissing);
            fConflicted    = std::move(block.fConflicted);

            nTime          = std::move(block.nTime);
            ssSystem       = std::move(block.ssSystem);
            vtx            = std::move(block.vtx);

            return *this;
        }


        /* Destructor. */
        SyncBlock::~SyncBlock()
        {
        }


        /* Copy Constructor. */
        SyncBlock::SyncBlock(const BlockState& state)
        : Block    (state)
        , nTime    (state.nTime)
        , ssSystem (state.ssSystem)
        , vtx      ( )
        {
            /* Loop through transactions in state block. */
            for(const auto& proof : state.vtx)
            {
                /* Switch for type. */
                switch(proof.first)
                {
                    /* Check for tritium. */
                    case TRANSACTION::TRITIUM:
                    {
                        /* Read the tritium transaction from disk. */
                        Transaction tx;
                        if(!LLD::Ledger->ReadTx(proof.second, tx, FLAGS::MEMPOOL)) //check mempool too
                            throw debug::exception(FUNCTION, "failed to read tx ", proof.second.SubString());

                        /* Serialize stream. */
                        DataStream ssData(SER_DISK, LLD::DATABASE_VERSION);
                        ssData << tx;

                        /* Add transaction to binary data. */
                        vtx.push_back(std::make_pair(proof.first, ssData.Bytes()));

                        break;
                    }

                    /* Check for legacy. */
                    case TRANSACTION::LEGACY:
                    {
                        /* Read the tritium transaction from disk. */
                        Legacy::Transaction tx;
                        if(!LLD::Legacy->ReadTx(proof.second, tx, FLAGS::MEMPOOL)) //check mempool too
                            throw debug::exception(FUNCTION, "failed to read tx ", proof.second.SubString());

                        /* Serialize stream. */
                        DataStream ssData(SER_DISK, LLD::DATABASE_VERSION);
                        ssData << tx;

                        /* Add transaction to binary data. */
                        vtx.push_back(std::make_pair(proof.first, ssData.Bytes()));

                        break;
                    }

                    /* Check for checkpoint. */
                    case TRANSACTION::CHECKPOINT:
                    {
                        /* Serialize stream. */
                        DataStream ssData(SER_DISK, LLD::DATABASE_VERSION);
                        ssData << proof.second;

                        /* Add transaction to binary data. */
                        vtx.push_back(std::make_pair(proof.first, ssData.Bytes()));

                        break;
                    }
                }
            }
        }

        /* Get the Signature Hash of the block. Used to verify work claims. */
        uint1024_t SyncBlock::SignatureHash() const
        {
            /* Signature hash for version 7 blocks. */
            if(nVersion >= 7)
            {
                /* Create a data stream to get the hash. */
                DataStream ss(SER_GETHASH, LLP::PROTOCOL_VERSION);
                ss.reserve(256);

                /* Serialize the data to hash into a stream. */
                ss << nVersion << hashPrevBlock << hashMerkleRoot << nChannel << nHeight << nBits << nNonce << nTime << vOffsets;

                return LLC::SK1024(ss.begin(), ss.end());
            }

            /* Create a data stream to get the hash. */
            DataStream ss(SER_GETHASH, LLP::PROTOCOL_VERSION);
            ss.reserve(256);

            /* Serialize the data to hash into a stream. */
            ss << nVersion << hashPrevBlock << hashMerkleRoot << nChannel << nHeight << nBits << nNonce << uint32_t(nTime);

            return LLC::SK1024(ss.begin(), ss.end());
        }
    }
}
