/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2017] ++

			(c) Copyright The Nexus Developers 2014 - 2017

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To The Voice of The People

____________________________________________________________________________________________*/

#ifndef NEXUS_CORE_INCLUDE_GLOBAL_H
#define NEXUS_CORE_INCLUDE_GLOBAL_H

#include <string>
#include <map>
#include <set>

#include "../../LLC/types/uint1024.h"

#include "../../Util/include/mutex.h"
#include "../../Util/templates/containers.h"

class CBigNum;
class CDataStream;

namespace Wallet
{
	class CWallet;
}

namespace Core
{
	class CBlock;
	class CBlockIndex;
	class CTrustPool;


	/* The Hash of the Genesis Block for the Network. Genesis Blocks are used as the root of a blockchain. */
	extern const uint1024 hashGenesisBlockOfficial;


	/* The Hash of the Genesis Block for the Testnet. Genesis Blocks are used as the root of a blockchain. */
	extern const uint1024 hashGenesisBlockTestNet;



	/* __________________________________________________ (Block Processing Constants) __________________________________________________  */




	/* The maximum size of a block in bytes. */
	extern const unsigned int MAX_BLOCK_SIZE;


	/* The maxmimum size a block can be filled while generating (mining). */
	extern const unsigned int MAX_BLOCK_SIZE_GEN;


	/* The maxmimum Operations each block can contain (prevents DoS exhaustion attacks). */
	extern const unsigned int MAX_BLOCK_SIGOPS;


	/* The maximum orphan transactions that arse kept around in memory. */
	extern const unsigned int MAX_ORPHAN_TRANSACTIONS;


	/* The minimum difficulty that any one channel can have. */
	extern CBigNum bnProofOfWorkLimit[];


	/* The starting difficulty of each channel for fair launch. */
	extern CBigNum bnProofOfWorkStart[];


	/* The minimum difficulty that any one channel can have (For Regression Tests) */
	extern CBigNum bnProofOfWorkLimitRegtest[];


	/* The starting difficulty of each channel (For Regression Tests) */
	extern CBigNum bnProofOfWorkStartRegtest[];



	/* __________________________________________________ (Time-Lock Activation Constants) __________________________________________________  */




	/* The Current Block Version being activated by NETWORK_VERSION_TIMELOCK. Sets the rules for new block versions. */
	extern const unsigned int NETWORK_BLOCK_CURRENT_VERSION;


	/* The Current Block Version being activated by TESTNET_VERSION_TIMELOCK. Sets the rules for new block versions. */
	extern const unsigned int TESTNET_BLOCK_CURRENT_VERSION;


	/* The UNIX Timestamp to set as the genesis time-lock (the Network activation timestamp). */
	extern const unsigned int NEXUS_NETWORK_TIMELOCK;


	/* The UNIX Timestamp to set as the genesis time-lcok (the Testnet activation timestamp). */
	extern const unsigned int NEXUS_TESTNET_TIMELOCK;


	/* The UNIX Timestamp to set as the activation timestamps for all Network block version updates (Consensus Updates). */
	extern const unsigned int NETWORK_VERSION_TIMELOCK[];


	/* The UNIX Timestamp to set as the activation timestamps for all Testnet block version updates (Consensus Updates). */
	extern const unsigned int TESTNET_VERSION_TIMELOCK[];


	/* The UNIX Timestamp to set the Network activation times of each of the block production channels (Prime, Hash, nPoS). */
	extern const unsigned int CHANNEL_NETWORK_TIMELOCK[];


	/* The UNIX Timestamp to set the activation times of each of the block production channels (Prime, Hash, nPoS). */
	extern const unsigned int CHANNEL_TESTNET_TIMELOCK[];



	/* __________________________________________________ (Transaction Constants) __________________________________________________  */


	/* The significcant digits a "COIN" can be broken into. */
	extern const int64 COIN;


	/* The significiant digits a "CENT" can be broken into. */
	extern const int64 CENT;


	/* The Minimum fee Amount for a Transaction (TODO: Going to be eliminated post Tritium). */
	extern const int64 MIN_TX_FEE;


	/* The Minimum fee Amount for a Transaction (TODO: Going to be eliminated post Tritium). */
	extern const int64 MIN_RELAY_TX_FEE;


	/* The Maximum amount that can be sent in a single transaction. */
	extern const int64 MAX_TXOUT_AMOUNT;


	/* The Minimum amount that can be sent in a single transaction. */
	extern const int64 MIN_TXOUT_AMOUNT;


	/* The total blocks for any generated (minted) transaction to confirm. */
	extern const int COINBASE_MATURITY;


	/* Threshold to distinguish difference between block height and unix timestamp. (TODO: Remoe Post Tritium) */
	extern const int LOCKTIME_THRESHOLD;



	/* __________________________________________________ (Trust Keys / nPoS Constants) __________________________________________________  */




	/* The targeted time between stake blocks. */
	extern const int STAKE_TARGET_SPACING;


	/* NOTE: Pre-Tritium Rule. The minimum amount of blocks that have to be between a trust key's produced blocks. */
	extern const int TRUST_KEY_MIN_INTERVAL;


	/* NOTE: Pre-Tritium Rule. The amount that is needed to stake to get a fully weighted Energy Efficiency Threshold. */
	extern const uint64 MAX_STAKE_WEIGHT;


	/* Tritium ++ Rules for Trust Keys. The timespan in which positive and negative trust are calculated against. */
	extern const int TRUST_KEY_MAX_TIMESPAN;


	/* Tritium ++ Rules for Trust Keys. Consistency History shows how consistent a node's block production times are. */
	extern double TRUST_KEY_DIFFICULTY_THRESHOLD;



	/* __________________________________________________ (Developer / Ambassador Constants) __________________________________________________  */




	/* Standard non-spendable testnet address for outputs and signature verification. */
	extern const std::string TESTNET_DUMMY_ADDRESS;


	/* Standard list of Ambassador Addresses sent as second to last input on every transaction */
	extern const std::string AMBASSADOR_ADDRESSES[];


	/* Standard list of Developer Addresses sent as second to last input on every transaction */
	extern const std::string DEVELOPER_ADDRESSES[];


	/* Byte Level data of each Developer Script Signature for verifying addresses in coinbase inputs */
	extern const unsigned char DEVELOPER_SCRIPT_SIGNATURES[][37];


	/* Byte Level data of each Developer Script Signature for verifying addresses in coinbase inputs  */
	extern const unsigned char AMBASSADOR_SCRIPT_SIGNATURES[][37];


	/* Byte Level data of Testnet non-spendable address for Testing purposes only */
	extern const unsigned char TESTNET_DUMMY_SIGNATURE[];



	/* __________________________________________________ (Unified Time Data Externals) __________________________________________________  */



	/* The Maximum Seconds that a Clock can be Off. This is set to account
		for Network Propogation Times and Normal Hardware level clock drifting. */
	extern int MAX_UNIFIED_DRIFT;


	/* The Maximum Samples to be stored on a moving average for this node. */
	extern int MAX_UNIFIED_SAMPLES;


	/* The Maximum Samples to be stored on a moving average per node. */
	extern int MAX_PER_NODE_SAMPLES;



	/* __________________________________________________ (Mutable Block Chain Level References) __________________________________________________  */




	/* The number calculated on a per block basis to determine the most trusted chain */
	extern uint64 nBestChainTrust;


	/* Memory Only Map of Blocks that are in orphan holding until their dependent blocks arrive to this node. */
	extern std::map<uint1024, CBlock> mapOrphanBlocks;


	/* Memory Only Map of Orphan Blocks indexed by their previous block for easier reassembly of long or short block orphan chains. */
	extern std::multimap<uint1024, CBlock> mapOrphanBlocksByPrev;


	/* Memory Only Map of Transactions that are in orphan holding since they don't have any known previous transactions to spend the UTXO. */
	extern std::map<uint512, CDataStream*> mapOrphanTransactions;


	/* Memory Only Map referencing orphan transactions by their previous inputs for easier reassembling of a transaction chain that is broken into orphans. */
	extern std::map<uint512, std::map<uint512, CDataStream*> > mapOrphanTransactionsByPrev;


	/* Memory Only Map of the blocks that have been stored as an Index Object. Used for quick reading / writing of block data. */
	extern std::map<uint1024, CBlockIndex*> mapBlockIndex;


	/* Main CWallet registry (wallet.dat) for Core dispatching functions to keep the wallet.dat up to date with what the rest of the transaction history is on the network. */
	extern std::set<Wallet::CWallet*> setpwalletRegistered;


	/* Mutable Number of blocks for node maturity. For switching TESTNET constants */
	extern int nCoinbaseMaturity;


	/* Mutable Transaction Fee inherited by constants and mutated by TESTNET switch */
	extern int64 nTransactionFee;


	/* The Last time a block was recieved. Used for internal node processing. */
	extern int64 nTimeBestReceived;


	/* The last Transaction that was in a block. */
	extern uint64 nLastBlockTx;


	/* The Size of the Latest block accepted into the Block Chain. */
	extern uint64 nLastBlockSize;


	/* The Official Genesis Block switched by the TESTNET flag. */
	extern uint1024 hashGenesisBlock;


	/* Reference of the last checkpoint. */
	extern uint1024 hashLastCheckpoint;


	/* The Official Hash of the Best Chain. This is set in the SetBestChain method in block.h/block.cpp. */
	extern uint1024 hashBestChain;


	/* The current file that is being used to append new data to the block database (blk0001.dat). */
	extern unsigned int nCurrentBlockFile;


	/* The height of the best block in the chain. Used to know what the current block chain height is. */
	extern unsigned int nBestHeight;


	/* The index pointer of the genesis block for easy referencing and reading / writing from disk. */
	extern CBlockIndex* pindexGenesisBlock;


	/* The index point of the best block currently written in memory and on disk. Used to reference the top of the block chain. */
	extern CBlockIndex* pindexBest;



	/* __________________________________________________ (GUI Reporting / Mutexs) __________________________________________________
     NOTE: To be deprecated into the new*/




	/* The total trust weight of a given node reported by stake minter into the GUI (The bottom right hand corner clock / question mark). */
	extern double dTrustWeight;


	/* The total weight that a trust key has gained based on block time which means the longer it has been since a block, the higher the weight. */
	extern double dBlockWeight;


	/* The total interest rate of the given trust key. Ranges fro 0.5% - 3% Annually based on contribution time and trust. */
	extern double dInterestRate;


	/* Main Critical Section for Core Level Functions */
	extern Mutex_t cs_main;


	/* Main Critical Section for the Wallet Registry Functions */
	extern Mutex_t cs_setpwalletRegistered;


	/** Filter to gain average block count among peers **/
	extern CMajority<int> cPeerBlockCounts;


	/** Small function to ensure the global supply remains within reasonable bounds **/
	inline bool MoneyRange(int64 nValue) { return (nValue >= 0 && nValue <= MAX_TXOUT_AMOUNT); }



	/* __________________________________________________ (TODO: Post Tritium Deprecation) __________________________________________________  */


	/* TODO: Very simple and ineffective way to track address balances. Build a new LLD Database for such Post Tritium. */
	extern std::map<uint256, uint64> mapAddressTransactions;


	/* TODO: Left-over from old blockchain architecture. No longer needed. Remove pre / post Tritium. */
	extern const std::string strMessageMagic;


	/* NOTE: Pre-Tritium Rule. One day expiration. Tritium uses positive and negative trust. (TODO: Remove Post Tritium) */
	extern int TRUST_KEY_EXPIRE;

}


#endif
