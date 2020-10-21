/*__________________________________________________________________________________________

(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

(c) Copyright The Nexus Developers 2014 - 2018

Distributed under the MIT software license, see the accompanying
file COPYING or http://www.opensource.org/licenses/mit-license.php.

"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_TAO_LEDGER_TYPES_TRITIUM_POOL_MINTER_H
#define NEXUS_TAO_LEDGER_TYPES_TRITIUM_POOL_MINTER_H

#include <TAO/Ledger/types/stake_minter.h>

#include <TAO/Ledger/types/sigchain.h>
#include <TAO/Ledger/types/transaction.h>

#include <Util/include/allocators.h>

#include <atomic>
#include <thread>


/* Global TAO namespace. */
namespace TAO
{

    /* Ledger namespace. */
    namespace Ledger
    {

    /** @class TritiumPoolMinter
     *
     * This class supports pool mining blocks on the Proof of Stake channel.
     *
     * It is implemented as a Singleton instance retrieved by calling GetInstance().
     *
     * Staking does not start, though, until Start() is called for the first time.
     * It requires single user mode, with a user account unlocked for staking before it will start successfully.
     *
     * A new trust account register must be created for the active user account signature chain and balance
     * sent to this account before it can successfully stake.
     *
     * The initial stake transaction for a new trust account is the Genesis transaction using OP::POOL_GENESIS
     * in the coinstake. Genesis commits the current available balance from the trust account to the stake balance.
     *
     * Accounts staking for Genesis cannot be the block finder for a pool stake block, so the minter will not attempt
     * to hash blocks until after the account has completed stake genesis. Before then, it will only submit a Genesis
     * coinstake to the stake pool.
     *
     * All subsequent stake transactions are Trust transactions and use OP::POOL_TRUST in the producer.
     * This operation will generete stake reward based on each individual account trust and stake balance, with a portion of
     * the overall reward going to the block finder.
     *
     * Pool staking also supports running in -client mode. In this mode, it will not hash blocks for either Genesis or Trust.
     * It will only create a coinstake and submit it to the pool to be mined by a full node that is pool staking.
     *
     * Committed stake may be changed by a StakeChange stored in the local database. Added stake is moved from the available
     * balance in the trust account to the committed stake balance upon successfully mining a stake block. Although the added
     * amount is not moved until a block is found, it is immediately included in stake minting calculations. Stake
     * removed (unstake) is moved from committed stake to available balance (with associated trust cost) upon successfully
     * mining a stake block. The unstake amount continues to be included in minting calculations until it is moved.
     *
     * Staking operations can be suspended by calling Stop() (for example, when the account is locked)
     * and restarted by calling Start() again.
     *
     **/
    class TritiumPoolMinter final : public TAO::Ledger::StakeMinter
    {
    public:
        /** Copy constructor deleted **/
        TritiumPoolMinter(const TritiumPoolMinter&) = delete;

        /** Copy assignment deleted **/
        TritiumPoolMinter& operator=(const TritiumPoolMinter&) = delete;


        /** Destructor **/
        ~TritiumPoolMinter() {}


        /** GetInstance
         *
         * Retrieves the TritiumPoolMinter.
         *
         * @return reference to the TritiumPoolMinter instance
         *
         **/
        static TritiumPoolMinter& GetInstance();


        /** Start
         *
         * Start the stake minter.
         *
         * Call this method to start the stake minter thread and begin mining Proof of Stake, or
         * to restart it after it was stopped.
         *
         * In general, this method should be called when a signature chain is unlocked for staking.
         *
         * If the system is configured not to run the TritiumPoolMinter, this method will return false.
         *
         * After calling this method, the TritiumPoolMinter thread may stay in suspended state if
         * the local node is synchronizing, or if it does not have any connections, yet.
         * In that case, it will automatically begin when sync is complete and connections
         * are available.
         *
         * @return true if the stake minter was started, false if it was already running or not started
         *
         */
        bool Start() override;


        /** Stop
         *
         * Stops the stake minter.
         *
         * Call this method to signal the stake minter thread stop Proof of Stake mining and end.
         * It can be restarted via a subsequent call to Start().
         *
         * Should be called whenever the wallet is locked, and on system shutdown.
         *
         * @return true if the stake minter was stopped, false if it was already stopped
         *
         */
        bool Stop() override;


    protected:
        /** CreateCoinstake
         *
         *  Create the coinstake transaction for a pool Proof of Stake block and add it as the candidate block producer.
         *
         *  @param[in] user - the currently active signature chain
         *  @param[in] strPIN - active pin corresponding to the sig chain
         *
         *  @return true if the coinstake was successfully created
         *
         **/
        bool CreateCoinstake(const memory::encrypted_ptr<TAO::Ledger::SignatureChain>& user, const SecureString& strPIN) override;


        /** MintBlock
         *
         *  Initialize the staking process for pool Proof of Stake and call HashBlock() to perform block hashing.
         *
         *  @param[in] user - the user account signature chain that is staking
         *  @param[in] strPIN - active pin corresponding to the sig chain
         *
         **/
        void MintBlock(const memory::encrypted_ptr<TAO::Ledger::SignatureChain>& user, const SecureString& strPIN) override;


        /** CheckBreak
         *
         *  Check whether or not to stop hashing in HashBlock() to process block updates.
         *
         *  @return true if block requires update
         *
         **/
        bool CheckBreak() override;


        /** CheckStale
         *
         *  Check that producer coinstakes won't orphan any new transaction for the same hashGenesis received in mempool
         *  since starting work on the current block.
         *
         *  @return true if current block is stale
         *
         **/
        bool CheckStale() override;


        /** CalculateCoinstakeReward
         *
         *  Calculate the block finder coinstake reward for a pool mined Proof of Stake block.
         *
         *  Block finder receives stake reward + net fees paid by pooled coinstakes
         *
         *  @param[in] nTime - the time for which the reward will be calculated
         *
         *  @return the amount of reward paid by the block
         *
         **/
        uint64_t CalculateCoinstakeReward(uint64_t nTime) override;


    private:
        /** Set true when pool minter thread starts and remains true while it is running **/
        static std::atomic<bool> fStarted;


        /** Thread for operating the stake minter **/
        static std::thread stakeMinterThread;


        /** Producer transaction for this node as block finder in pooled stake.
         *  This is kept separate from block so it can be added as last producer in block when it is built from stakepool.
         **/
        TAO::Ledger::Transaction txProducer;

        /** Maximum number of coinstakes to include in candidate block **/
        uint32_t nProducerSize;

        /** Last recorded size of the stake pool. Comparing to current pool size lets us know when pool has received more tx. **/
        uint32_t nPoolSize;

        /** Net balance of coinstakes from pool to be used for hashing the current block **/
        uint64_t nPoolStake;

        /** Net fees paid by coinstakes from pool included in the current block **/
        uint64_t nPoolFee;


        /** Constructor **/
        TritiumPoolMinter();


        /** SetupPool
         *
         *  Setup pooled stake parameters.
         *
         *  @return true if process completed successfully
         *
         **/
        bool SetupPool();


        /** AddPoolCoinstakes
         *
         *  Add coinstake transactions from the stake pool to the current candidate block.
         *
         *  This method will also populate nPooledStake with the available balance total of added transactions that can be
         *  used for hashing calculations. nStake is used in determining this amount.
         *
         *  @param[in] nStake Stake balance of local account.
         *
         *  @return true if process completed successfully
         *
         **/
        bool AddPoolCoinstakes(const uint64_t& nStake);


        /** StakeMinterThread
         *
         *  Method run on its own thread to oversee stake minter operation using the methods in the
         *  tritium minter instance. The thread will continue running after initialized, but operation can
         *  be stopped/restarted by using the stake minter methods.
         *
         *  On shutdown, the thread will cease operation and wait for the minter destructor to tell it to exit/join.
         *
         *  @param[in] pTritiumPoolMinter - the minter thread will use this instance to perform all the tritium minter work
         *
         **/
        static void StakeMinterThread(TritiumPoolMinter* pTritiumPoolMinter);

    };

    }
}

#endif
