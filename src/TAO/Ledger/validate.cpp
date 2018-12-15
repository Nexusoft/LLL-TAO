/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2018

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To The Voice of The People

____________________________________________________________________________________________*/

#include <TAO/Ledger/include/types/block.h>

namespace TAO::Ledger
{

    /* Checks if a block is valid if not connected to chain. */
    template<typename BlockType> bool CheckBlock(BlockType block) const
    {
        /* Check the Size limits of the Current Block. */
        if (vtx.empty() || vtx.size() > MAX_BLOCK_SIZE || ::GetSerializeSize(*this, SER_NETWORK, PROTOCOL_VERSION) > MAX_BLOCK_SIZE)
            return DoS(100, debug::error("CheckBlock() : size limits failed"));


        /** Make sure the Block was Created within Active Channel. **/
        if (GetChannel() > 2)
            return DoS(50, debug::error("CheckBlock() : Channel out of Range."));


        /** Check that the time was within range. */
        if (GetBlockTime() > runtime::UnifiedTimestamp() + MAX_UNIFIED_DRIFT)
            return debug::error("AcceptBlock() : block timestamp too far in the future");


        /** Do not allow blocks to be accepted above the Current Blo>::const_iterator it = mapWallet.begin(); it != mapWallet.end(); ++it)
            vCoins.push_back(&(*it).ck Version. **/
        if(nVersion > (fTestNet ? TESTNET_BLOCK_CURRENT_VERSION : NETWORK_BLOCK_CURRENT_VERSION))
            return DoS(50, debug::error("CheckBlock() : Invalid Block Version."));


        /** Only allow POS blocks in Version 4. **/
        if(IsProofOfStake() && nVersion < 4)
            return DoS(50, debug::error("CheckBlock() : Proof of Stake Blocks Rejected until Version 4."));


        /** Check the Proof of Work Claims. **/
        if (!IsInitialBlockDownload() && IsProofOfWork() && !VerifyWork())
            return DoS(50, debug::error("CheckBlock() : Invalid Proof of Work"));


        /** Check the Network Launch Time-Lock. **/
        if (nHeight > 0 && GetBlockTime() <= (fTestNet ? NEXUS_TESTNET_TIMELOCK : NEXUS_NETWORK_TIMELOCK))
            return debug::error("CheckBlock() : Block Created before Network Time-Lock");


        /** Check the Current Channel Time-Lock. **/
        if (nHeight > 0 && GetBlockTime() < (fTestNet ? CHANNEL_TESTNET_TIMELOCK[GetChannel()] : CHANNEL_NETWORK_TIMELOCK[GetChannel()]))
            return debug::error("CheckBlock() : Block Created before Channel Time-Lock. Channel Opens in %" PRId64 " Seconds", (fTestNet ? CHANNEL_TESTNET_TIMELOCK[GetChannel()] : CHANNEL_NETWORK_TIMELOCK[GetChannel()]) - runtime::UnifiedTimestamp());


        /** Check the Current Version Block Time-Lock. Allow Version (Current -1) Blocks for 1 Hour after Time Lock. **/
        if (nVersion > 1 && nVersion == (fTestNet ? TESTNET_BLOCK_CURRENT_VERSION - 1 : NETWORK_BLOCK_CURRENT_VERSION - 1) && (GetBlockTime() - 3600) > (fTestNet ? TESTNET_VERSION_TIMELOCK[TESTNET_BLOCK_CURRENT_VERSION - 2] : NETWORK_VERSION_TIMELOCK[NETWORK_BLOCK_CURRENT_VERSION - 2]))
            return debug::error("CheckBlock() : Version %u Blocks have been Obsolete for %" PRId64 " Seconds", nVersion, (runtime::UnifiedTimestamp() - (fTestNet ? TESTNET_VERSION_TIMELOCK[TESTNET_BLOCK_CURRENT_VERSION - 2] : NETWORK_VERSION_TIMELOCK[TESTNET_BLOCK_CURRENT_VERSION - 2])));


        /** Check the Current Version Block Time-Lock. **/
        if (nVersion >= (fTestNet ? TESTNET_BLOCK_CURRENT_VERSION : NETWORK_BLOCK_CURRENT_VERSION) && GetBlockTime() <= (fTestNet ? TESTNET_VERSION_TIMELOCK[TESTNET_BLOCK_CURRENT_VERSION - 2] : NETWORK_VERSION_TIMELOCK[NETWORK_BLOCK_CURRENT_VERSION - 2]))
            return debug::error("CheckBlock() : Version %u Blocks are not Accepted for %" PRId64 " Seconds", nVersion, (runtime::UnifiedTimestamp() - (fTestNet ? TESTNET_VERSION_TIMELOCK[TESTNET_BLOCK_CURRENT_VERSION - 2] : NETWORK_VERSION_TIMELOCK[NETWORK_BLOCK_CURRENT_VERSION - 2])));


        /** Check the Required Mining Outputs. **/
        if (IsProofOfWork() && nVersion >= 3) {
            unsigned int nSize = vtx[0].vout.size();

            /** Check the Coinbase Tx Size. **/
            if(nSize < 3)
                return debug::error("CheckBlock() : Coinbase Too Small.");

            if(!fTestNet)
            {
                if (!VerifyAddressList(vtx[0].vout[nSize - 2].scriptPubKey, (nVersion < 5) ? AMBASSADOR_SCRIPT_SIGNATURES : AMBASSADOR_SCRIPT_SIGNATURES_RECYCLED))
                    return debug::error("CheckBlock() : Block %u Ambassador Signature Not Verified.", nHeight);

                if (!VerifyAddressList(vtx[0].vout[nSize - 1].scriptPubKey, (nVersion < 5) ? DEVELOPER_SCRIPT_SIGNATURES : DEVELOPER_SCRIPT_SIGNATURES_RECYCLED))
                    return debug::error("CheckBlock() :  Block %u Developer Signature Not Verified.", nHeight);
            }

            else
            {
                if (!VerifyAddress(vtx[0].vout[nSize - 2].scriptPubKey, (nVersion < 5) ? TESTNET_DUMMY_SIGNATURE : TESTNET_DUMMY_SIGNATURE_AMBASSADOR_RECYCLED))
                    return debug::error("CheckBlock() :  Block %u Ambassador Signature Not Verified.", nHeight);

                if (!VerifyAddress(vtx[0].vout[nSize - 1].scriptPubKey, (nVersion < 5) ? TESTNET_DUMMY_SIGNATURE : TESTNET_DUMMY_SIGNATURE_DEVELOPER_RECYCLED))
                    return debug::error("CheckBlock() :  Block %u Developer Signature Not Verified.", nHeight);
            }
        }


        /** Check the Coinbase Transaction is First, with no repetitions. **/
        if (vtx.empty() || (!vtx[0].IsCoinBase() && nChannel > 0))
            return DoS(100, debug::error("CheckBlock() : first tx is not coinbase for Proof of Work Block"));


        /** Check the Coinstake Transaction is First, with no repetitions. **/
        if (vtx.empty() || (!vtx[0].IsCoinStake() && nChannel == 0))
            return DoS(100, debug::error("CheckBlock() : first tx is not coinstake for Proof of Stake Block"));


        /** Check for duplicate Coinbase / Coinstake Transactions. **/
        for (unsigned int i = 1; i < vtx.size(); i++)
            if (vtx[i].IsCoinBase() || vtx[i].IsCoinStake())
                return DoS(100, debug::error("CheckBlock() : more than one coinbase / coinstake"));


        /** Check coinbase/coinstake timestamp is at least 20 minutes before block time **/
        if (GetBlockTime() > (int64)vtx[0].nTime + ((nVersion < 4) ? 1200 : 3600))
            return DoS(50, debug::error("CheckBlock() : coinbase/coinstake timestamp is too early"));

        /* Ensure the Block is for Proof of Stake Only. */
        if(IsProofOfStake())
        {

            /* Check the Coinstake Time is before Unified Timestamp. */
            if(vtx[0].nTime > (runtime::UnifiedTimestamp() + MAX_UNIFIED_DRIFT))
                return debug::error("CheckBlock() : Coinstake Transaction too far in Future.");

            /* Make Sure Coinstake Transaction is First. */
            if (!vtx[0].IsCoinStake())
                return debug::error("CheckBlock() : First transaction non-coinstake %s", vtx[0].GetHash().ToString().c_str());

            /* Make Sure Coinstake Transaction Time is Before Block. */
            if (vtx[0].nTime > nTime)
                return debug::error("CheckBlock()  : Coinstake Timestamp to is ahead of block time");

        }

        /** Check the Transactions in the Block. **/
        BOOST_FOREACH(const CTransaction& tx, vtx)
        {
            if (!tx.CheckTransaction())
                return DoS(tx.nDoS, debug::error("CheckBlock() : CheckTransaction failed"));

            // Nexus: check transaction timestamp
            if (GetBlockTime() < (int64)tx.nTime)
                return DoS(50, debug::error("CheckBlock() : block timestamp earlier than transaction timestamp"));
        }


        // Check for duplicate txids. This is caught by ConnectInputs(),
        // but catching it earlier avoids a potential DoS attack:
        set<uint512> uniqueTx;
        BOOST_FOREACH(const CTransaction& tx, vtx)
        {
            uniqueTx.insert(tx.GetHash());
        }
        if (uniqueTx.size() != vtx.size())
            return DoS(100, debug::error("CheckBlock() : duplicate transaction"));

        unsigned int nSigOps = 0;
        BOOST_FOREACH(const CTransaction& tx, vtx)
        {
            nSigOps += tx.GetLegacySigOpCount();
        }

        if (nSigOps > MAX_BLOCK_SIGOPS)
            return DoS(100, debug::error("CheckBlock() : out-of-bounds SigOpCount"));

        // Check merkleroot
        if (hashMerkleRoot != BuildMerkleTree())
            return DoS(100, debug::error("CheckBlock() : hashMerkleRoot mismatch"));


        /* Check the Block Signature. */
        if (!CheckBlockSignature())
            return DoS(100, debug::error("CheckBlock() : bad block signature"));

        return true;
    }

}
