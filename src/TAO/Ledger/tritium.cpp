/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/include/key.h>

#include <TAO/Ledger/types/tritium.h>

#include <TAO/Ledger/include/constants.h>
#include <TAO/Ledger/include/timelocks.h>

#include <Util/include/args.h>
#include <Util/include/hex.h>

namespace TAO::Ledger
{

    /* For debugging Purposes seeing block state data dump */
    std::string TritiumBlock::ToString() const
    {
        return debug::strprintf("Block("
            VALUE("hash")     " = %s "
            VALUE("nVersion") " = %u, "
            VALUE("hashPrevBlock") " = %s, "
            VALUE("hashMerkleRoot") " = %s, "
            VALUE("nChannel") " = %u, "
            VALUE("nHeight") " = %u, "
            VALUE("nBits") " = %u, "
            VALUE("nNonce") " = %" PRIu64 ", "
            VALUE("nTime") " = %u, "
            VALUE("vchBlockSig") " = %s, "
            VALUE("vtx.size()") " = %u)",
        GetHash().ToString().substr(0, 20).c_str(), nVersion, hashPrevBlock.ToString().substr(0, 20).c_str(), hashMerkleRoot.ToString().substr(0, 20).c_str(), nChannel, nHeight, nBits, nNonce, nTime, HexStr(vchBlockSig.begin(), vchBlockSig.end()).c_str(), vtx.size());
    }


    /* For debugging purposes, printing the block to stdout */
    void TritiumBlock::print() const
    {
        debug::log(0, ToString().c_str());
    }


    /* Checks if a block is valid if not connected to chain. */
    bool TritiumBlock::Check() const
    {
        /* Check the Size limits of the Current Block. */
        if (::GetSerializeSize(*this, SER_NETWORK, LLP::PROTOCOL_VERSION) > MAX_BLOCK_SIZE)
            return debug::error(FUNCTION "size limits failed");


        /* Make sure the Block was Created within Active Channel. */
        if (GetChannel() > 2)
            return debug::error(FUNCTION "channel out of Range.");


        /* Check that the time was within range. */
        if (GetBlockTime() > runtime::unifiedtimestamp() + MAX_UNIFIED_DRIFT)
            return debug::error(FUNCTION "block timestamp too far in the future");


        /* Do not allow blocks to be accepted above the current block version. */
        if(nVersion == 0 || nVersion > (config::fTestNet ? TESTNET_BLOCK_CURRENT_VERSION : NETWORK_BLOCK_CURRENT_VERSION))
            return debug::error(FUNCTION "invalid block version");


        /* Only allow POS blocks in Version 4. */
        if(IsProofOfStake() && nVersion < 4)
            return debug::error(FUNCTION "proof-of-stake rejected until version 4");


        /* Check the Proof of Work Claims. */
        //if (IsProofOfWork() && !VerifyWork())
        //    return debug::error(FUNCTION "invalid proof of work");


        /* Check the Network Launch Time-Lock. */
        if (nHeight > 0 && GetBlockTime() <= (config::fTestNet ? NEXUS_TESTNET_TIMELOCK : NEXUS_NETWORK_TIMELOCK))
            return debug::error(FUNCTION "block created before network time-lock");


        /* Check the Current Channel Time-Lock. */
        if (nHeight > 0 && GetBlockTime() < (config::fTestNet ? CHANNEL_TESTNET_TIMELOCK[GetChannel()] : CHANNEL_NETWORK_TIMELOCK[GetChannel()]))
            return debug::error(FUNCTION "block created before channel time-lock, please wait %" PRIu64 " seconds", (config::fTestNet ? CHANNEL_TESTNET_TIMELOCK[GetChannel()] : CHANNEL_NETWORK_TIMELOCK[GetChannel()]) - runtime::unifiedtimestamp());


        /* Check the Current Version Block Time-Lock. Allow Version (Current -1) Blocks for 1 Hour after Time Lock. */
        if (nVersion > 1 && nVersion == (config::fTestNet ? TESTNET_BLOCK_CURRENT_VERSION - 1 : NETWORK_BLOCK_CURRENT_VERSION - 1) && (GetBlockTime() - 3600) > (config::fTestNet ? TESTNET_VERSION_TIMELOCK[TESTNET_BLOCK_CURRENT_VERSION - 2] : NETWORK_VERSION_TIMELOCK[NETWORK_BLOCK_CURRENT_VERSION - 2]))
            return debug::error(FUNCTION "version %u blocks have been obsolete for %" PRId64 " seconds", nVersion, (runtime::unifiedtimestamp() - (config::fTestNet ? TESTNET_VERSION_TIMELOCK[TESTNET_BLOCK_CURRENT_VERSION - 2] : NETWORK_VERSION_TIMELOCK[TESTNET_BLOCK_CURRENT_VERSION - 2])));


        /* Check the Current Version Block Time-Lock. */
        if (nVersion >= (config::fTestNet ? TESTNET_BLOCK_CURRENT_VERSION : NETWORK_BLOCK_CURRENT_VERSION) && GetBlockTime() <= (config::fTestNet ? TESTNET_VERSION_TIMELOCK[TESTNET_BLOCK_CURRENT_VERSION - 2] : NETWORK_VERSION_TIMELOCK[NETWORK_BLOCK_CURRENT_VERSION - 2]))
            return debug::error(FUNCTION "version %u blocks are not accepted for %" PRId64 " seconds", nVersion, (runtime::unifiedtimestamp() - (config::fTestNet ? TESTNET_VERSION_TIMELOCK[TESTNET_BLOCK_CURRENT_VERSION - 2] : NETWORK_VERSION_TIMELOCK[NETWORK_BLOCK_CURRENT_VERSION - 2])));


        /* Check the producer transaction. */
        if(nHeight > 0)
        {
            /* Check the coinbase if not genesis. */
            if(GetChannel() > 0 && !producer.IsCoinbase())
                return debug::error(FUNCTION "producer transaction has to be coinbase for proof of work");

            /* Check that the producer is a valid transactions. */
            if(!producer.IsValid())
                return debug::error(FUNCTION "producer transaction is invalid");
        }


        /* Check the producer transaction. */
        if(GetChannel() == 0 && !producer.IsTrust())
            return debug::error(FUNCTION "producer transaction has to be trust for proof of stake");


        /* Check coinbase/coinstake timestamp is at least 20 minutes before block time */
        if (GetBlockTime() > (uint64_t)producer.nTimestamp + ((nVersion < 4) ? 1200 : 3600))
            return debug::error(FUNCTION "producer transaction timestamp is too early");


        /* Proof of stake specific checks. */
        if(IsProofOfStake())
        {
            /* Check for nonce of zero values. */
            if(nNonce == 0)
                return debug::error(FUNCTION "proof of stake can't have Nonce value of zero");

            /* Check the trust time is before Unified timestamp. */
            if(producer.nTimestamp > (runtime::unifiedtimestamp() + MAX_UNIFIED_DRIFT))
                return debug::error(FUNCTION "trust timestamp too far in the future");

            /* Make Sure Trust Transaction Time is Before Block. */
            if (producer.nTimestamp > GetBlockTime())
                return debug::error(FUNCTION "trust timestamp is ahead of block timestamp");
        }


        /* Check for duplicate txid's */
        std::set<uint512_t> uniqueTx;


        /* Get the hashes for the merkle root. */
        std::vector<uint512_t> vHashes;


        /* Only do producer transaction on non genesis. */
        if(nHeight > 0)
            vHashes.push_back(producer.GetHash());


        /* Get the signature operations for legacy tx's. */
        uint32_t nSigOps = 0;


        /* Check all the transactions. */
        for(auto & tx : vtx)
        {
            /* Insert txid into set to check for duplicates. */
            uniqueTx.insert(tx.second);

            /* Push back this hash for merkle root. */
            vHashes.push_back(tx.second);

            /* Basic checks for legacy transactions. */
            if(tx.first == TYPE::LEGACY_TX)
            {
                //check the legacy memory pool.

                //if (!tx.CheckTransaction())
                //    return DoS(tx.nDoS, error("CheckBlock() : CheckTransaction failed"));

                // Nexus: check transaction timestamp
                //if (GetBlockTime() < (int64)tx.nTime)
                //    return DoS(50, error("CheckBlock() : block timestamp earlier than transaction timestamp"));

                //nSigOps += tx.GetLegacySigOpCount();
            }

            /* Basic checks for tritium transactions. */
            else if(tx.first == TYPE::TRITIUM_TX)
            {
                //check the tritium memory pool.
            }
            else if(tx.first != TYPE::CHECKPOINT)
                return debug::error(FUNCTION "unknown transaction type");
        }


        /* Check for duplicate txid's. */
        if (uniqueTx.size() != vtx.size())
            return debug::error(FUNCTION "duplicate transaction");


        /* Check the signature operations for legacy. */
        if (nSigOps > MAX_BLOCK_SIGOPS)
            return debug::error(FUNCTION "out-of-bounds SigOpCount");


        /* Check the merkle root. */
        if (hashMerkleRoot != BuildMerkleTree(vHashes))
            return debug::error(FUNCTION "hashMerkleRoot mismatch");

        /* Get the key from the producer. */
        if(nHeight > 0)
        {
            /* Create the key to check. */
            LLC::ECKey key(NID_brainpoolP512t1, 64);
            key.SetPubKey(producer.vchPubKey);

            /* Check the Block Signature. */
            if (!VerifySignature(key))
                return debug::error(FUNCTION "bad block signature");
        }

        return true;
    }
}
