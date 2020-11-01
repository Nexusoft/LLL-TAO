/*__________________________________________________________________________________________

(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

(c) Copyright The Nexus Developers 2014 - 2018

Distributed under the MIT software license, see the accompanying
file COPYING or http://www.opensource.org/licenses/mit-license.php.

"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_TAO_LEDGER_TYPES_STAKE_MINTER_H
#define NEXUS_TAO_LEDGER_TYPES_STAKE_MINTER_H

#include <LLC/types/uint1024.h>

#include <TAO/API/include/session.h>

#include <TAO/Ledger/include/stake_change.h>
#include <TAO/Ledger/types/sigchain.h>
#include <TAO/Ledger/types/stake_thread.h>
#include <TAO/Ledger/types/state.h>
#include <TAO/Ledger/types/tritium.h>

#include <TAO/Register/types/object.h>

#include <Util/include/allocators.h>
#include <Util/include/memory.h>
#include <Util/include/softfloat.h>

#include <atomic>
#include <memory>


/* Global TAO namespace. */
namespace TAO
{

    /* Ledger namespace. */
    namespace Ledger
    {

    /** Mutex for coordinating stake minters on different threads. */
    extern std::mutex STAKING_MUTEX;

    /* forward declarations */
    class StakeThread;

    /** @class StakeMinter
     *
     * This class provides the base class and processes for mining blocks on the Proof of Stake channel.
     * Implementations will provide the details for each specific mining type.
     *
     * Staking minting proceeds in two phases: the Setup phase and the Hashing phase
     *
     * To perform setup, the following methods should be called:
     *  CheckUser
     *  FindTrustAccount
     *  CreateCandidateBlock
     *  CalculateWeights
     *
     * To perform hashing, call
     *  MintBlock
     *
     * These methods must be called from a StakeThread instance, which is declared as a friend of this class.
     *
     * The MintBlock() method returns control whenever it reaches a hashing threshold, so it must be called in
     * a loop until a new setup phase is required. This allows multisession staking to interweave the hashing phases
     * of multiple stake minters onto a single thread.
     *
     **/
    class StakeMinter
    {
    friend class TAO::Ledger::StakeThread;

    public:

        /** Destructor **/
        virtual ~StakeMinter() {}


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


	protected:
        /** true when the minter is waiting to begin staking.
         *  This occurs during the Genesis hold period after trust account receives new balance.
         **/
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


        /** Hash of the best chain when the minter began attempting to mine its current candidate.
         *  If current hashBestChain changes, the minter must start over with a new candidate.
         **/
        uint1024_t hashLastBlock;


        /** The candidate block that the stake minter is currently attempting to mine **/
        TritiumBlock block;


        /** Flag to indicate whether staking for Genesis or Trust **/
        bool fGenesis;


        /** Flag to indicate when this minter needs to restart with next setup phase (no further hashing current block) **/
        bool fRestart;


        /** Trust score applied for the current candidate block. Not used for Genesis. **/
        uint64_t nTrust;


        /** Block age (time since stateLast) for the current candidate block. Not used for Genesis. **/
        uint64_t nBlockAge;


        /** The login session associated with this stake minter. **/
        /* note - It is important that the StakeMinter not outlive this session.
         * To prevent this reference from becoming invalid, the minter must be ended before the session is terminated
         * via a call to StakeManager::Stop()
         */
        TAO::API::Session& session;


        /** Session Id of login session associated with this stake minter. **/
        uint256_t nSession;


        /** Timestamp when minter last logged a wait message. Used during wait periods to meter log entries **/
        uint64_t nLogTime;


        /** Reason the minter is logging wait messages, so it knows when to reset nLogTime */
        uint8_t nWaitReason;


        /** Constructor
         *
         *  @param[in] the session Id for which this minter will stake
         *
         **/
        StakeMinter(uint256_t& sessionId);


        /** CreateCandidateBlock
         *
         *  Creates a new tritium block that the stake minter will attempt to mine via the Proof of Stake process.
         *
         *  Each staking algorithm will implement its own block setup.
         *
         *  @return true if the candidate block was successfully created
         *
         **/
        virtual bool CreateCandidateBlock() = 0;


        /** MintBlock
         *
         *  Initialize the staking process for the candidate block and call HashBlock() to perform block hashing.
         *
         *  Each staking algorithm will implement its own setup for minting blocks.
         *
         *  @return true if the method performs hashing, false if not hashing
         *
         **/
        virtual bool MintBlock() = 0;


        /** CheckStale
         *
         *  Check that block producer coinstake(s) won't orphan any new transaction for the same hashGenesis received in mempool
         *  since starting work on the current block.
         *
         *  Each staking algorithm will implement its own process for checking when this is needed.
         *
         *  @return true if current block is stale
         *
         **/
        virtual bool CheckStale() = 0;


        /** CalculateCoinstakeReward
         *
         *  Calculate the coinstake reward for a newly mined Proof of Stake block.
         *
         *  Each staking algorithm will implement its own reward calculation.
         *
         *  @param[in] nTime - the time for which the reward will be calculated, typically current timestamp
         *
         *  @return the amount of reward paid by the block
         *
         **/
        virtual uint64_t CalculateCoinstakeReward(uint64_t nTime) = 0;


        /** CheckUser
         *
         *  Verify minter's signature chain is unlocked for staking.
         *
         *  @return true if the user signature chain can stake
         *
         **/
        bool CheckUser();


        /** FindTrust
         *
         *  Gets the trust account for the minter's signature chain and stores it into account instance variable. The trust
         *  account will be updated if it receives Genesis or Trust, so this must be performed every minting round to assure
         *  we have any new data.
         *
         *  @return true if the trust account was successfully retrieved
         *
         **/
        bool FindTrustAccount();


        /** FindLastStake
         *
         *  Retrieves the most recent stake transaction for the minter's signature chain.
         *
         *  @param[out] hashLast - the most recent stake transaction hash
         *
         *  @return true if the last stake transaction was successfully retrieved
         *
         **/
        bool FindLastStake(uint512_t &hashLast);


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
         *  The signature on the change request is also verified against the crypto register for this minter's signature chain.
         *  If it does not match, the request is removed.
         *
         *  @param[in] hashLast - hash of last stake transaction for the user's trust account
         *
         *  @return true if processed successfully
         *
         **/
        bool FindStakeChange(const uint512_t& hashLast);


        /** CalculateWeights
         *
         *  Calculates the Trust Weight and Block Weight values for the current trust account and candidate block.
         *
         *  @return true if the weights were properly calculated
         *
         */
        bool CalculateWeights();


        /** SetLastBlock
         *
         *  Assigns the last block hash (best chain) for the current minting round. Must be set before calling MintBlock
         *  for the first time each round.
         *
         *  The driving thread should tell the minter what block hash it is using, rather than get it from ChainState.
         *  In the case it has changed already since the thread started the current round, the minter will know not to
         *  hash.
         *
         *  @param[out] hashBlock - current best block hash
         *
         **/
        void SetLastBlock(const uint1024_t& hashBlock);


        /** CheckInterval
         *
         *  Verify whether or not this minter's signature chain has met the interval requirement for
         *  minimum number of blocks between stake blocks it mines (Trust only).
         *
         *  @return true if sig chain has met interval requirement
         *
         **/
        bool CheckInterval();


        /** CheckMempool
         *
         *  Verify no transactions for the user signature chain associated with this minter in the mempool (Genesis only).
         *
         *  Genesis blocks do not include mempool transactions. Therefore, if the mempool already has any transactions
         *  for a sig chain, they would be orphaned if the chain generated stake Genesis. This method allows minter
         *  to skip mining rounds until mempool transactions are cleared.
         *
         *  @return true if no transactions pending for sig chain
         *
         **/
        bool CheckMempool();


        /** HashBlock
         *
         *  Attempt to solve the hashing algorithm at the current staking difficulty for the candidate block, while
         *  operating within the required threshold restriction.
         *
         *  This method returns whenever it hits the provided threshold. Call MintBlock() again to continue hashing
         *  the current block for this stake minter.
         *
         *  When a block solution is found, this method calls ProcessBlock() to process and relay the new block.
         *
         *  @param[in] nRequired - required theshold for stake block hashing
         *
         **/
        void HashBlock(const cv::softdouble& nRequired);


        /** ProcessBlock
         *
         *  Processes a newly mined Proof of Stake block, adds transactions from the mempool, and relays it
         *  to the network.
         *
         *  @return true if the block passed all process checks and was successfully submitted
         *
         **/
        bool ProcessBlock();


        /** SignBlock
         *
         *  Sign a candidate block after it is successfully mined.
         *
         *  @return true if block successfully signed
         *
         **/
        bool SignBlock();

    };


    }
}

#endif
