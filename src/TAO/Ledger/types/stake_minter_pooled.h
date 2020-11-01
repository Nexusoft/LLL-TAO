/*__________________________________________________________________________________________

(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

(c) Copyright The Nexus Developers 2014 - 2018

Distributed under the MIT software license, see the accompanying
file COPYING or http://www.opensource.org/licenses/mit-license.php.

"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_TAO_LEDGER_TYPES_STAKE_MINTER_POOLED_H
#define NEXUS_TAO_LEDGER_TYPES_STAKE_MINTER_POOLED_H

#include <TAO/Ledger/types/stake_minter.h>

#include <TAO/Ledger/types/transaction.h>


/* Global TAO namespace. */
namespace TAO
{

    /* Ledger namespace. */
    namespace Ledger
    {

    /** @class StakeMinterPooled
     *
     * This class supports pool mining blocks on the Proof of Stake channel. It should be created and managed
     * by an instance of StakeThread via a called to StakeMinter::GetInstance() in the parent class. The thread then
     * drives the staking process.
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
     * Committed stake may be changed by a StakeChange stored in the local database. Added stake is moved from the available
     * balance in the trust account to the committed stake balance upon successfully mining a stake block. Although the added
     * amount is not moved until a block is found, it is immediately included in stake minting calculations. Stake
     * removed (unstake) is moved from committed stake to available balance (with associated trust cost) upon successfully
     * mining a stake block. The unstake amount continues to be included in minting calculations until it is moved.
     *
     **/
    class StakeMinterPooled final : public TAO::Ledger::StakeMinter
    {
    public:
        /** Constructor **/
        StakeMinterPooled(uint256_t& sessionId);


        /** Destructor **/
        ~StakeMinterPooled() {}


        /** Copy constructor deleted **/
        StakeMinterPooled(const StakeMinterPooled&) = delete;

        /** Copy assignment deleted **/
        StakeMinterPooled& operator=(const StakeMinterPooled&) = delete;


    protected:
        /** CreateCandidateBlock
         *
         *  Creates a new pooled Proof of Stake block that the stake minter will attempt to mine via the Proof of Stake process.
         *  This includes creating the coinstake transaction and adding it as the block producer.
         *
         *  @return true if the candidate block was successfully created
         *
         **/
        bool CreateCandidateBlock() override;


        /** MintBlock
         *
         *  Initialize the staking process for a  pooled Proof of Stake block and call HashBlock() to perform block hashing.
         *
         *  @return true if the method performs hashing, false if not hashing
         *
         **/
        bool MintBlock() override;


        /** CheckStale
         *
         *  Check that producer coinstakes won't orphan any new transaction for the same hashGenesis received in mempool
         *  since starting work on the current block. This method checks all coinstakes in the pooled stake block.
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
        /** Producer transaction for local node as block finder in pooled stake.
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
         *  This method populates nPoolStake with the available balance total of added transactions that can be
         *  used for hashing calculations. nStake argument is used in determining this amount.
         *
         *  This method also populate nPoolFee with the net fee amount to be received from the pool.
         *
         *  @param[in] nStake Stake balance of local account.
         *
         *  @return true if process completed successfully
         *
         **/
        bool AddPoolCoinstakes(const uint64_t& nStake);

    };

    }
}

#endif
