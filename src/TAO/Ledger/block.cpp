/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/hash/SK.h>
#include <LLC/hash/macro.h>
#include <LLC/include/key.h>
#include <LLC/types/bignum.h>

#include <Util/templates/serialize.h>
#include <Util/include/hex.h>
#include <Util/include/args.h>
#include <Util/include/runtime.h>

#include <TAO/Ledger/types/block.h>
#include <TAO/Ledger/include/prime.h>
#include <TAO/Ledger/include/constants.h>
#include <TAO/Ledger/include/timelocks.h>

#include <ios>
#include <iomanip>

/* Global TAO namespace. */
namespace TAO
{

    /* Ledger Layer namespace. */
    namespace Ledger
    {

        /* Set the block state to null. */
        void Block::SetNull()
        {
            nVersion = config::fTestNet ? TESTNET_BLOCK_CURRENT_VERSION : NETWORK_BLOCK_CURRENT_VERSION;
            hashPrevBlock = 0;
            hashMerkleRoot = 0;
            nChannel = 0;
            nHeight = 0;
            nBits = 0;
            nNonce = 0;
            nTime = 0;
            vchBlockSig.clear();
        }


        /* Set the channel of the block. */
        void Block::SetChannel(uint32_t nNewChannel)
        {
            nChannel = nNewChannel;
        }


        /* Get the Channel block is produced from. */
        uint32_t Block::GetChannel() const
        {
            return nChannel;
        }


        /* Check the nullptr state of the block. */
        bool Block::IsNull() const
        {
            return (nBits == 0);
        }


        /* Return the Block's current UNIX timestamp. */
        uint64_t Block::GetBlockTime() const
        {
            return (uint64_t)nTime;
        }


        /* Get the prime number of the block. */
        LLC::CBigNum Block::GetPrime() const
        {
            return LLC::CBigNum(ProofHash() + nNonce);
        }


        /* GGet the Proof Hash of the block. Used to verify work claims. */
        uint1024_t Block::ProofHash() const
        {
            /** Hashing template for CPU miners uses nVersion to nBits **/
            if(GetChannel() == 1)
                return LLC::SK1024(BEGIN(nVersion), END(nBits));

            /** Hashing template for GPU uses nVersion to nNonce **/
            return LLC::SK1024(BEGIN(nVersion), END(nNonce));
        }


        /* Get the Proof Hash of the block. Used to verify work claims. */
        uint1024_t Block::SignatureHash() const
        {
            return LLC::SK1024(BEGIN(nVersion), END(nTime));
        }


        /* Generate a Hash For the Block from the Header. */
        uint1024_t Block::GetHash() const
        {
            /* Pre-Version 5 rule of being block hash. */
            if(nVersion < 5)
                return ProofHash();

            return LLC::SK1024(BEGIN(nVersion), END(nTime));
        }


        /* Update the nTime of the current block. */
        void Block::UpdateTime()
        {
            nTime = runtime::unifiedtimestamp();
        }


        /* Check flags for nPoS block. */
        bool Block::IsProofOfStake() const
        {
            return (nChannel == 0);
        }


        /* Check flags for PoW block. */
        bool Block::IsProofOfWork() const
        {
            return (nChannel == 1 || nChannel == 2);
        }


        /* Generate the Merkle Tree from uint512_t hashes. */
        uint512_t Block::BuildMerkleTree(std::vector<uint512_t> vMerkleTree) const
        {
            int j = 0;
            for (int nSize = (int)vMerkleTree.size(); nSize > 1; nSize = (nSize + 1) / 2)
            {
                for (int i = 0; i < nSize; i += 2)
                {
                    int i2 = std::min(i+1, nSize-1);
                    vMerkleTree.push_back(LLC::SK512(BEGIN(vMerkleTree[j+i]),  END(vMerkleTree[j+i]),
                                                BEGIN(vMerkleTree[j+i2]), END(vMerkleTree[j+i2])));
                }
                j += nSize;
            }
            return (vMerkleTree.empty() ? 0 : vMerkleTree.back());
        }

        /* Dump the Block data to Console / Debug.log. */
        void Block::print() const
        {
            debug::log(0,
                "Block(hash=", GetHash().ToString().substr(0,20),
                ", ver=", nVersion,
                ", hashPrevBlock=", hashPrevBlock.ToString().substr(0,20),
                ", hashMerkleRoot=", hashMerkleRoot.ToString().substr(0,10),
                ", nTime=", nTime,
                std::hex, std::setfill('0'), std::setw(8), ", nBits=", nBits,
                std::dec, std::setfill(' '), std::setw(0), ", nChannel = ", nChannel,
                ", nHeight= ", nHeight,
                ", nNonce=",  nNonce,
                ", vchBlockSig=", HexStr(vchBlockSig.begin(), vchBlockSig.end()), ")");
        }


        /* Verify the Proof of Work satisfies network requirements. */
        bool Block::VerifyWork() const
        {
            /* Check the Prime Number Proof of Work for the Prime Channel. */
            if(GetChannel() == 1)
            {
                /* Check prime minimum origins. */
                if(nVersion < 5 && ProofHash() < bnPrimeMinOrigins.getuint1024())
                    return debug::error(FUNCTION, "prime origins below 1016-bits");

                /* Check proof of work limits. */
                uint32_t nPrimeBits = GetPrimeBits(GetPrime());
                if (nPrimeBits < bnProofOfWorkLimit[1])
                    return debug::error(FUNCTION, "prime-cluster below minimum work");

                /* Check the prime difficulty target. */
                if(nBits > nPrimeBits)
                    return debug::error(FUNCTION, "prime-cluster below target");

                return true;
            }

            /* Get the hash target. */
            LLC::CBigNum bnTarget;
            bnTarget.SetCompact(nBits);

            /* Check that the hash is within range. */
            if (bnTarget <= 0 || bnTarget > bnProofOfWorkLimit[2])
                return debug::error(FUNCTION, "proof-of-work hash not in range");


            /* Check that the that enough work was done on this block. */
            if (ProofHash() > bnTarget.getuint1024())
                return debug::error(FUNCTION, "proof-of-work hash below target");

            return true;
        }


        /* Sign the block with the key that found the block. */
        bool Block::GenerateSignature(LLC::ECKey key)
        {
            return key.Sign((nVersion == 4) ? SignatureHash() : GetHash(), vchBlockSig, 1024);
        }


        /* Check that the block signature is a valid signature. */
        bool Block::VerifySignature(LLC::ECKey key) const
        {
            if (vchBlockSig.empty())
                return false;

            return key.Verify((nVersion == 4) ? SignatureHash() : GetHash(), vchBlockSig, 1024);
        }
    }
}
