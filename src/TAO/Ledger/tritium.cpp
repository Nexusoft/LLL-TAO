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
        return debug::strprintf(
			ANSI_COLOR_BRIGHT_WHITE "Block" ANSI_COLOR_RESET "("
			ANSI_COLOR_BRIGHT_WHITE "nVersion" ANSI_COLOR_RESET " = %u, "
			ANSI_COLOR_BRIGHT_WHITE "hashPrevBlock" ANSI_COLOR_RESET " = %s, "
			ANSI_COLOR_BRIGHT_WHITE "hashMerkleRoot" ANSI_COLOR_RESET " = %s, "
			ANSI_COLOR_BRIGHT_WHITE "nChannel" ANSI_COLOR_RESET " = %u, "
			ANSI_COLOR_BRIGHT_WHITE "nHeight" ANSI_COLOR_RESET " = %u, "
			ANSI_COLOR_BRIGHT_WHITE "nBits" ANSI_COLOR_RESET " = %u, "
			ANSI_COLOR_BRIGHT_WHITE "nNonce" ANSI_COLOR_RESET " = %" PRIu64 ", "
			ANSI_COLOR_BRIGHT_WHITE "nTime" ANSI_COLOR_RESET " = %u, "
			ANSI_COLOR_BRIGHT_WHITE "vchBlockSig" ANSI_COLOR_RESET " = %s, "
			ANSI_COLOR_BRIGHT_WHITE "vtx.size()" ANSI_COLOR_RESET " = %u)",
        nVersion, hashPrevBlock.ToString().substr(0, 20).c_str(), hashMerkleRoot.ToString().substr(0, 20).c_str(), nChannel, nBits, nNonce, nTime, HexStr(vchBlockSig.begin(), vchBlockSig.end()).c_str(), vtx.size());
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
		if (vtx.empty() || ::GetSerializeSize(*this, SER_NETWORK, LLP::PROTOCOL_VERSION) > MAX_BLOCK_SIZE)
			return debug::error(FUNCTION "size limits failed", __PRETTY_FUNCTION__);


		/** Make sure the Block was Created within Active Channel. **/
		if (GetChannel() > 2)
			return debug::error(FUNCTION "channel out of Range.", __PRETTY_FUNCTION__);


		/** Check that the time was within range. */
		if (GetBlockTime() > runtime::UnifiedTimestamp() + MAX_UNIFIED_DRIFT)
			return debug::error(FUNCTION "block timestamp too far in the future", __PRETTY_FUNCTION__);


		/** Do not allow blocks to be accepted above the current block version. */
		if(nVersion > (config::fTestNet ? TESTNET_BLOCK_CURRENT_VERSION : NETWORK_BLOCK_CURRENT_VERSION))
			return debug::error(FUNCTION "invalid block version", __PRETTY_FUNCTION__);


		/** Only allow POS blocks in Version 4. **/
		if(IsProofOfStake() && nVersion < 4)
			return debug::error(FUNCTION "proof-of-stake rejected until version 4", __PRETTY_FUNCTION__);


		/** Check the Proof of Work Claims. **/
		if (IsProofOfWork() && !VerifyWork())
			return debug::error(FUNCTION "invalid proof of work", __PRETTY_FUNCTION__);


		/** Check the Network Launch Time-Lock. **/
		if (nHeight > 0 && GetBlockTime() <= (config::fTestNet ? NEXUS_TESTNET_TIMELOCK : NEXUS_NETWORK_TIMELOCK))
			return debug::error(FUNCTION "block created before network time-lock", __PRETTY_FUNCTION__);


		/** Check the Current Channel Time-Lock. **/
		if (nHeight > 0 && GetBlockTime() < (config::fTestNet ? CHANNEL_TESTNET_TIMELOCK[GetChannel()] : CHANNEL_NETWORK_TIMELOCK[GetChannel()]))
			return debug::error(FUNCTION "block created before channel time-lock, please wait %" PRIu64 " seconds", __PRETTY_FUNCTION__, (config::fTestNet ? CHANNEL_TESTNET_TIMELOCK[GetChannel()] : CHANNEL_NETWORK_TIMELOCK[GetChannel()]) - runtime::UnifiedTimestamp());


		/** Check the Current Version Block Time-Lock. Allow Version (Current -1) Blocks for 1 Hour after Time Lock. **/
		if (nVersion > 1 && nVersion == (config::fTestNet ? TESTNET_BLOCK_CURRENT_VERSION - 1 : NETWORK_BLOCK_CURRENT_VERSION - 1) && (GetBlockTime() - 3600) > (config::fTestNet ? TESTNET_VERSION_TIMELOCK[TESTNET_BLOCK_CURRENT_VERSION - 2] : NETWORK_VERSION_TIMELOCK[NETWORK_BLOCK_CURRENT_VERSION - 2]))
			return debug::error(FUNCTION "version %u blocks have been obsolete for %" PRId64 " seconds", __PRETTY_FUNCTION__, nVersion, (runtime::UnifiedTimestamp() - (config::fTestNet ? TESTNET_VERSION_TIMELOCK[TESTNET_BLOCK_CURRENT_VERSION - 2] : NETWORK_VERSION_TIMELOCK[TESTNET_BLOCK_CURRENT_VERSION - 2])));


		/** Check the Current Version Block Time-Lock. **/
		if (nVersion >= (config::fTestNet ? TESTNET_BLOCK_CURRENT_VERSION : NETWORK_BLOCK_CURRENT_VERSION) && GetBlockTime() <= (config::fTestNet ? TESTNET_VERSION_TIMELOCK[TESTNET_BLOCK_CURRENT_VERSION - 2] : NETWORK_VERSION_TIMELOCK[NETWORK_BLOCK_CURRENT_VERSION - 2]))
			return debug::error(FUNCTION "version %u blocks are not accepted for %" PRId64 " seconds", __PRETTY_FUNCTION__, nVersion, (runtime::UnifiedTimestamp() - (config::fTestNet ? TESTNET_VERSION_TIMELOCK[TESTNET_BLOCK_CURRENT_VERSION - 2] : NETWORK_VERSION_TIMELOCK[NETWORK_BLOCK_CURRENT_VERSION - 2])));


		/* Check the producer transaction. */
		if(!producer.IsCoinBase())
			return debug::error(FUNCTION "producer transaction has to be coinbase.", __PRETTY_FUNCTION__);


		/* Check for duplicate txid's */
		std::set<uint512_t> uniqueTx;
		std::vector<uint512_t> vHashes;
		for(auto & tx : vtx)
		{
			uniqueTx.insert(tx.second);
			vHashes.push_back(tx.second);
		}
		if (uniqueTx.size() != vtx.size())
			return debug::error(FUNCTION "duplicate transaction", __PRETTY_FUNCTION__);


		/* Check the merkle root. */
		if (hashMerkleRoot != BuildMerkleTree(vHashes))
			return debug::error(FUNCTION "hashMerkleRoot mismatch", __PRETTY_FUNCTION__);


		/* Get the key from the producer. */
		LLC::ECKey key;
		key.SetPubKey(producer.vchPubKey);


		/* Check the Block Signature. */
		if (!VerifySignature(key))
			return debug::error(FUNCTION "bad block signature", __PRETTY_FUNCTION__);

		return true;
	}
}
