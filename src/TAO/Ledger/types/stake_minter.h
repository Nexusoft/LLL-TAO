/*__________________________________________________________________________________________

(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

(c) Copyright The Nexus Developers 2014 - 2018

Distributed under the MIT software license, see the accompanying
file COPYING or http://www.opensource.org/licenses/mit-license.php.

"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_TAO_LEDGER_TYPES_BASE_MINTER_H
#define NEXUS_TAO_LEDGER_TYPES_BASE_MINTER_H


#include <LLC/types/uint1024.h>

#include <TAO/Ledger/include/stake_change.h>
#include <TAO/Ledger/types/sigchain.h>
#include <TAO/Ledger/types/state.h>
#include <TAO/Ledger/types/tritium.h>

#include <TAO/Register/types/object.h>

#include <Util/include/memory.h>
#include <Util/include/softfloat.h>

#include <atomic>


/* Global TAO namespace. */
namespace TAO
{

	/* Ledger namespace. */
	namespace Ledger
	{

	/** @class StakeMinter
	 *
	 * This class provides the base class and processes for mining blocks on the Proof of Stake channel.
	 * Implementations will provide the details for each specific mining type.
	 *
	 * Staking starts when Start() is called.
	 *
	 * Staking operations can be suspended by calling Stop and restarted by calling Start() again.
	 *
	 **/
	class StakeMinter
	{
	public:

		/** Destructor **/
		virtual ~StakeMinter() {}


		/** GetInstance
		 *
		 * Retrieves the stake minter instance to use for staking.
		 *
		 * @return reference to the TritiumMinter instance
		 *
		 **/
		static StakeMinter& GetInstance();


		/** IsStarted
		 *
		 * Tests whether or not a stake minter is currently running.
		 *
		 * @return true if a stake minter is started, false otherwise
		 *
		 **/
		bool IsStarted() const;


		/** GetBlockWeight
		 *
		 * Retrieves the current internal value for the block weight metric.
		 *
		 * @return value of current block weight
		 *
		 **/
		double GetBlockWeight() const;


		/** GetBlockWeightPercent
		 *
		 * Retrieves the block weight metric as a percentage of maximum.
		 *
		 * @return the current block weight percentage
		 *
		 **/
		double GetBlockWeightPercent() const;


		/** GetTrustWeight
		 *
		 * Retrieves the current internal value for the trust weight metric.
		 *
		 * @return value of current trust weight
		 *
		 **/
		double GetTrustWeight() const;


		/** GetTrustWeightPercent
		 *
		 * Retrieves the trust weight metric as a percentage of maximum.
		 *
		 * @return the current trust weight percentage
		 *
		 **/
		double GetTrustWeightPercent() const;


		/** GetStakeRate
		 *
		 * Retrieves the current staking reward rate.
		 *
		 * @return the current stake rate
		 *
		 **/
		double GetStakeRate() const;


		/** GetStakeRatePercent
		 *
		 * Retrieves the current staking reward rate as an annual percentage.
		 *
		 * @return the current stake rate percentage
		 *
		 **/
		double GetStakeRatePercent() const;


		/** IsWaitPeriod
		 *
		 * Checks whether the stake minter is waiting to begin staking.
		 *
		 * @return true if minter is waiting, false otherwise
		 *
		 **/
		bool IsWaitPeriod() const;


		/** GetWaitTime
		 *
		 * When IsWaitPeriod() is true, this method returns the remaining wait time before staking is active.
		 *
		 * @return number of seconds remaining for minimum age to stake
		 *
		 **/
		uint64_t GetWaitTime() const;


		/** Start
		 *
		 * Starts the stake minter.
		 *
		 * Call this method to start the stake minter thread and begin mining Proof of Stake, or
		 * to restart it after it was stopped.
		 *
		 * @return true if the stake minter was started, false if it was already running or not started
		 *
		 **/
		virtual bool Start() = 0;


		/** Stop
		 *
		 * Stops the stake minter.
		 *
		 * Call this method to signal the stake minter thread stop Proof of Stake mining and end.
		 * It can be restarted via a subsequent call to Start().
		 *
		 * @return true if the stake minter was stopped, false if it was already stopped
		 *
		 **/
		virtual bool Stop() = 0;


	protected:
        /** Set true when any stake miner thread starts and remains true while it is running **/
        static std::atomic<bool> fStarted;


        /** Flag to tell the stake minter thread to stop processing and exit. **/
        static std::atomic<bool> fStop;


		/** Hash of the best chain when the minter began attempting to mine its current candidate.
		 *  If current hashBestChain changes, the minter must start over with a new candidate.
		 **/
		uint1024_t hashLastBlock;


		/** Time (in milliseconds) to sleep between blocks. **/
		uint64_t nSleepTime;


		/** true when the minter is waiting to begin staking **/
		std::atomic<bool> fWait;


		/** When fWait is true, this field will store the remaining wait time before staking is active **/
		std::atomic<uint64_t> nWaitTime;


		/** The current internal trust weight value **/
		std::atomic<double> nTrustWeight;


		/** The current internal block weight value **/
		std::atomic<double> nBlockWeight;


		/** The current stake rate for calculating staking rewards. This is for display only, updated at the start of each
         *  mining round. The actual rate used for calculating reward paid is based on block time when the block is found.
         **/
		std::atomic<double> nStakeRate;


        /** Trust account to use for staking. **/
        TAO::Register::Object account;


        /** Last stake block found by current trust account. Not used for Genesis. **/
        TAO::Ledger::BlockState stateLast;


        /** Flag to indicate whether the user has a current stake change request **/
        bool fStakeChange;


        /** Stake change request for current user, when one is present **/
        TAO::Ledger::StakeChange stakeChange;


        /** The candidate block that the stake minter is currently attempting to mine **/
        TritiumBlock block;


        /** Flag to indicate whether staking for Genesis or Trust **/
        bool fGenesis;


        /** Trust score applied for the current candidate block. Not used for Genesis. **/
        uint64_t nTrust;


        /** Block age (time since stateLast) for the current candidate block. Not used for Genesis. **/
        uint64_t nBlockAge;


		/** Default constructor **/
		StakeMinter()
		: hashLastBlock(0)
		, nSleepTime(1000)
		, fWait(false)
		, nWaitTime(0)
		, nTrustWeight(0.0)
		, nBlockWeight(0.0)
		, nStakeRate(0.0)
        , account()
        , stateLast()
        , fStakeChange(false)
        , stakeChange()
        , block()
        , fGenesis(false)
        , nTrust(0)
        , nBlockAge(0)
		{
		}


        /** CreateCoinstake
         *
         *  Create the coinstake transaction and add it as the candidate block producer.
         *
         *  Each staking algorithm will implement its own approach for coinstake generation.
         *
         *  @param[in] user - the currently active signature chain
         *
         *  @return true if the coinstake was successfully created
         *
         **/
        virtual bool CreateCoinstake(const memory::encrypted_ptr<TAO::Ledger::SignatureChain>& user) = 0;


        /** MintBlock
         *
         *  Initialize the staking process for the candidate block and call HashBlock() to perform block hashing.
         *
         *  When HashBlock() returns because CheckBreak() is true, this method will alter the candidate block appropriate
         *  and then call HashBlock() again to continue hashing.
         *
         *  Each staking algorithm will implement its own setup for minting blocks.
         *
         *  @param[in] user - the currently active signature chain
         *  @param[in] strPIN - active pin corresponding to the sig chain
         *
         **/
        virtual void MintBlock(const memory::encrypted_ptr<TAO::Ledger::SignatureChain>& user, const SecureString& strPIN) = 0;


        /** CheckBreak
         *
         *  Check whether or not to stop hashing in HashBlock() to process block updates.
         *
         *  Each staking algorithm will implement its own process for checking when this is needed.
         *
         *  @return true if HashBLock() should break and return
         *
         **/
        virtual bool CheckBreak() = 0;


        /** CalculateCoinstakeReward
         *
         *  Calculate the coinstake reward for a newly mined Proof of Stake block.
         *
         *  Each staking algorithm will implement its own reward calculation.
         *
         *  @return the amount of reward paid by the block
         *
         **/
        virtual uint64_t CalculateCoinstakeReward() = 0;


		/** CheckUser
		 *
		 *  Verify user account unlocked for minting.
		 *
		 *  @return true if the user account can stake
		 *
		 **/
		bool CheckUser();


		/** FindTrust
		 *
		 *  Gets the trust account for the staking signature chain and stores it into account instance variable.
		 *
		 *  @param[in] hashGenesis - genesis of user account signature chain that is staking
		 *
		 *  @return true if the trust account was successfully retrieved
		 *
		 **/
		bool FindTrustAccount(const uint256_t& hashGenesis);


        /** FindLastStake
         *
         *  Retrieves the most recent stake transaction for a user account.
         *
         *  @param[in] hashGenesis - genesis of user account signature chain that is staking
         *  @param[out] hashLast - the most recent stake transaction hash
         *
         *  @return true if the last stake transaction was successfully retrieved
         *
         **/
        bool FindLastStake(const uint256_t& hashGenesis, uint512_t& hashLast);


        /** FindStakeChange
         *
         *  Identifies any pending stake change request, populates the stakeChange instance variable with it, and sets
         *  fStakeChange to the appropriate value.
         *
         *  A sig chain cannot have generated any stake blocks since the change request was created (example, if stake on
         *  a different machine). If the hashLast for the sig chain does not match the hashLast on the change request, then
         *  the request is stale and is removed.
         *
         *  If there is an expired stake change request, it will be removed.
         *
         *  If the change request adds more than the available balance, or removes more than the stake amount, then it
         *  will be modified to use current available balance or stake amount as appropriate.
         *
         *  The signature on the change request is also verified against the crypto register for the supplied hashGenesis.
         *  If it does not match, the request is removed.
         *
         *  @param[in] hashGenesis - genesis of user account signature chain that is staking
         *  @param[in] hashLast - hash of last stake transaction for the user's trust account
         *
         *  @return true if processed successfully
         *
         **/
        bool FindStakeChange(const uint256_t& hashGenesis, const uint512_t hashLast);


        /** CreateCandidateBlock
         *
         *  Creates a new tritium block that the stake minter will attempt to mine via the Proof of Stake process.
         *
         *  @param[in] user - the user account signature chain that is staking
         *  @param[in] strPIN - active pin corresponding to the sig chain
         *
         *  @return true if the candidate block was successfully created
         *
         **/
        bool CreateCandidateBlock(const memory::encrypted_ptr<TAO::Ledger::SignatureChain>& user, const SecureString& strPIN);


        /** CalculateWeights
         *
         *  Calculates the Trust Weight and Block Weight values for the current trust account and candidate block.
         *
         *  @return true if the weights were properly calculated
         *
         */
        bool CalculateWeights();


        /** CheckInterval
         *
         *  Verify whether or not a signature chain has met the interval requirement for minimum number of blocks
         *  between stake blocks it mines (Trust only).
         *
         *  @param[in] user - the user account signature chain that is staking
         *
         *  @return true if sig chain has met interval requirement
         *
         **/
        bool CheckInterval(const memory::encrypted_ptr<TAO::Ledger::SignatureChain>& user);


        /** CheckMempool
         *
         *  Verify no transactions for same signature chain in the mempool (Genesis only).
         *
         *  Genesis blocks do not include mempool transactions. Therefore, if the mempool already has any transactions
         *  for a sig chain, they would be orphaned if the chain generated stake Genesis. This method allows minter
         *  to skip mining rounds until mempool transactions are cleared.
         *
         *  @param[in] user - the user account signature chain that is staking
         *
         *  @return true if no transactions pending for sig chain
         *
         **/
        bool CheckMempool(const memory::encrypted_ptr<TAO::Ledger::SignatureChain>& user);


		/** HashBlock
		 *
         *  Attempt to solve the hashing algorithm at the current staking difficulty for the candidate block, while
         *  operating within the required threshold restriction.
         *
         *  This process periodically calls the algorithm-specific CheckBreak() method to determine whether or not it needs
         *  to stop iterating to alter the block setup. Otherwise, it will continue to iterate until it either
         *  mines a new block or the hashBestChain changes and the minter must start over with a new candidate block.
         *
         *  When a block solution is found, this method calls ProcessBlock() to process and relay the new block.
		 *
         *  @param[in] user - the user account signature chain that is staking
         *  @param[in] strPIN - active pin corresponding to the sig chain
         *  @param[in] nRequired - required theshold for stake block hashing
		 *
		 **/
		void HashBlock(const memory::encrypted_ptr<TAO::Ledger::SignatureChain>& user, const SecureString& strPIN,
			           const cv::softdouble nRequired);


        /** ProcessBlock
         *
         *  Processes a newly mined Proof of Stake block, adds transactions from the mempool, and relays it
         *  to the network.
         *
         *  @param[in] user - the user account signature chain that is staking
         *  @param[in] strPIN - active pin corresponding to the sig chain
         *
         *  @return true if the block passed all process checks and was successfully submitted
         *
         **/
        bool ProcessBlock(const memory::encrypted_ptr<TAO::Ledger::SignatureChain>& user, const SecureString& strPIN);


        /** SignBlock
         *
         *  Sign a candidate block after it is successfully mined.
         *
         *  @param[in] user - the user account signature chain that is staking
         *  @param[in] strPIN - active pin corresponding to the sig chain
         *
         *  @return true if block successfully signed
         *
         **/
        bool SignBlock(const memory::encrypted_ptr<TAO::Ledger::SignatureChain>& user, const SecureString& strPIN);

	};


	}
}

#endif
