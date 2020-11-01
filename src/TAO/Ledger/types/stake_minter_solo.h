/*__________________________________________________________________________________________

(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

(c) Copyright The Nexus Developers 2014 - 2018

Distributed under the MIT software license, see the accompanying
file COPYING or http://www.opensource.org/licenses/mit-license.php.

"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_TAO_LEDGER_TYPES_STAKE_MINTER_SOLO_H
#define NEXUS_TAO_LEDGER_TYPES_STAKE_MINTER_SOLO_H

#include <TAO/Ledger/types/stake_minter.h>


/* Global TAO namespace. */
namespace TAO
{

    /* Ledger namespace. */
    namespace Ledger
    {

    /** @class StakeMinterSolo
     *
     * This class supports solo mining blocks on the Proof of Stake channel. It should be created and managed
     * by an instance of StakeThread via a called to StakeMinter::GetInstance() in the parent class. The thread then
     * drives the staking process.
     *
     * The stake balance and trust score from the trust account will be used for Proof of Stake.
     * A new trust account register must be created for the active user account signature chain and balance
     * sent to this account before it can successfully stake.
     *
     * The initial stake transaction for a new trust account is the Genesis transaction using OP::GENESIS
     * in the block producer. Genesis commits the current available balance from the trust account to the stake balance.
     *
     * All subsequent stake transactions are Trust transactions and use OP::TRUST applied to the committed stake balance.
     *
     * Committed stake may be changed by a StakeChange stored in the local database. Added stake is moved from the available
     * balance in the trust account to the committed stake balance upon successfully mining a stake block. Although the added
     * amount is not moved until a block is found, it is immediately included in stake minting calculations. Stake
     * removed (unstake) is moved from committed stake to available balance (with associated trust cost) upon successfully
     * mining a stake block. The unstake amount continues to be included in minting calculations until it is moved.
     *
     **/
    class StakeMinterSolo final : public TAO::Ledger::StakeMinter
    {
    public:
        /** Constructor **/
        StakeMinterSolo(uint256_t& sessionId);


        /** Destructor **/
        ~StakeMinterSolo() {}


        /** Copy constructor deleted **/
        StakeMinterSolo(const StakeMinterSolo&) = delete;

        /** Copy assignment deleted **/
        StakeMinterSolo& operator=(const StakeMinterSolo&) = delete;


    protected:
        /** CreateCandidateBlock
         *
         *  Creates a new solo Proof of Stake block that the stake minter will attempt to mine via the Proof of Stake process.
         *  This includes creating the coinstake transaction and adding it as the block producer.
         *
         *  @return true if the candidate block was successfully created
         *
         **/
        bool CreateCandidateBlock() override;


        /** MintBlock
         *
         *  Initialize the staking process for the candidate block and call HashBlock() to perform block hashing.
         *
         *  @return true if the method performs hashing, false if not hashing
         *
         **/
        bool MintBlock() override;


        /** CheckStale
         *
         *  Check that producer coinstake won't orphan any new transaction for the same hashGenesis received in mempool
         *  since starting work on the current block.
         *
         *  @return true if current block is stale
         *
         **/
        bool CheckStale() override;


        /** CalculateCoinstakeReward
         *
         *  Calculate the coinstake reward for a solo mined Proof of Stake block.
         *
         *  @param[in] nTime - the time for which the reward will be calculated
         *
         *  @return the amount of reward paid by the block
         *
         **/
        uint64_t CalculateCoinstakeReward(uint64_t nTime) override;

    };

    }
}

#endif
