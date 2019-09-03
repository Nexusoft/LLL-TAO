/*__________________________________________________________________________________________

(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

(c) Copyright The Nexus Developers 2014 - 2018

Distributed under the MIT software license, see the accompanying
file COPYING or http://www.opensource.org/licenses/mit-license.php.

"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_TAO_LEDGER_TYPES_TRITIUM_MINTER_H
#define NEXUS_TAO_LEDGER_TYPES_TRITIUM_MINTER_H

#include <TAO/Ledger/types/base_minter.h>
#include <TAO/Ledger/types/genesis.h>
#include <TAO/Ledger/types/sigchain.h>
#include <TAO/Ledger/types/state.h>
#include <TAO/Ledger/types/transaction.h>
#include <TAO/Ledger/types/tritium.h>

#include <TAO/Register/types/object.h>

#include <Util/include/allocators.h>
#include <Util/include/memory.h>

#include <atomic>
#include <thread>


/* Global TAO namespace. */
namespace TAO
{

    /* Ledger namespace. */
    namespace Ledger
    {

    /** @class TritiumMinter
     *
     * This class performs all operations for mining blocks on the Proof of Stake channel.
     * It is implemented as a Singleton instance retrieved by calling GetInstance().
     *
     * Staking does not start, though, until Start() is called for the first time.
     * It requires single user mode, with a user account unlocked for minting before it
     * will start successfully.
     *
     * The stake balance and trust score from the trust account will be used for Proof of Stake.
     * A new trust account register must be created for the active user account signature chain
     * before it can successfully stake.
     *
     * The initial stake transaction for a new trust account is the Genesis transaction using OP::GENESIS
     * in the block producer. All subsequent stake transactions are Trust transactions and use OP::TRUST.
     *
     * Staking operations can be suspended by calling Stop() (for example, when the account is locked)
     * and restarted by calling Start() again.
     *
     **/
    class TritiumMinter final : public TAO::Ledger::StakeMinter
    {
    public:
        /** Copy constructor deleted **/
        TritiumMinter(const TritiumMinter&) = delete;

        /** Copy assignment deleted **/
        TritiumMinter& operator=(const TritiumMinter&) = delete;


        /** Destructor **/
        ~TritiumMinter() {}


        /** GetInstance
         *
         * Retrieves the TritiumMinter.
         *
         * @return reference to the TritiumMinter instance
         *
         **/
        static TritiumMinter& GetInstance();


        /** IsStarted
         *
         * Tests whether or not the stake minter is currently running.
         *
         * @return true if the stake minter is started, false otherwise
         *
         */
        bool IsStarted() const override;


        /** Start
         *
         * Start the stake minter.
         *
         * Call this method to start the stake minter thread and begin mining Proof of Stake, or
         * to restart it after it was stopped.
         *
         * The first time this method is called, it will retrieve a reference to the wallet
         * by calling Wallet::GetInstance(), so the wallet must be initialized and loaded before
         * starting the stake minter.
         *
         * In general, this method should be called when the wallet is unlocked.
         *
         * If the system is configured not to run the TritiumMinter, this method will return false.
         * By default, the TritiumMinter will run for non-server, and won't run for server/daemon.
         * These defaults can be changed using the -stake setting.
         *
         * After calling this method, the TritiumMinter thread may stay in suspended state if
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


    private:
        /** Set true when stake miner thread starts and remains true while it is running **/
        static std::atomic<bool> fStarted;


        /** Flag to tell the stake minter thread to stop processing and exit. **/
        static std::atomic<bool> fStop;


        /** Thread for operating the stake minter **/
        static std::thread tritiumMinterThread;


        /** Register address for the trust account. Only used for staking Genesis. **/
        uint256_t hashAddress;


        /** Trust account to use for staking. **/
        TAO::Register::Object account;


        /** Last stake transaction for the current trust account. */
        TAO::Ledger::Transaction txLast;


        /** Last stake block found by current trust account. */
        TAO::Ledger::BlockState stateLast;


        /** The candidate block that the stake minter is currently attempting to mine **/
        TritiumBlock block;


        /** Flag to indicate whether staking for Genesis or Trust **/
        bool fGenesis;


        /** Trust score applied for the candidate block the stake minter is attempting to mine **/
        uint64_t nTrust;


        /** Block age (time since last stake for trust account) for the candidate block the stake minter is attempting to mine **/
        uint64_t nBlockAge;


        /** Default constructor **/
        TritiumMinter()
        : hashAddress(0)
        , account()
        , txLast()
        , stateLast()
        , block()
        , fGenesis(false)
        , nTrust(0)
        , nBlockAge(0)
        {
        }


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
         *  Gets the trust account for the current active signature chain and stores it into account.
         *
         *  @param[in] hashGenesis - genesis of user account signature chain
         *
         *  @return true if the trust account was successfully retrieved
         *
         **/
        bool FindTrustAccount(const Genesis& hashGenesis);


        /** FindLastStake
         *
         *  Retrieves the most recent stake transaction for a user account.
         *
         *  @param[in] hashGenesis - genesis of user account signature chain
         *  @param[out] tx - the most recent stake transaction
         *
         *  @return true if the last stake transaction was successfully retrieved
         *
         **/
        bool FindLastStake(const Genesis& hashGenesis, Transaction& tx);


        /** CreateCandidateBlock
         *
         *  Creates a new tritium block that the stake minter will attempt to mine via the Proof of Stake process.
         *
         *  @param[in] user - the currently active signature chain
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


        /** MintBlock
         *
         *  Attempt to solve the hashing algorithm at the current staking difficulty for the candidate block, while
         *  operating within the energy efficiency requirements. This process will continue to iterate until it either
         *  mines a new block or the hashBestChain changes and the minter must start over with a new candidate block.
         *
         *  @param[in] user - the currently active signature chain
         *  @param[in] strPIN - active pin corresponding to the sig chain
         *
         **/
        void MintBlock(const memory::encrypted_ptr<TAO::Ledger::SignatureChain>& user, const SecureString& strPIN);


        /** ProcessBlock
         *
         *  Processes a newly mined Proof of Stake block, adds transactions from the mempool, and submits it
         *  to the network
         *
         *  @param[in] user - the currently active signature chain
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
         *  @param[in] user - the currently active signature chain
         *  @param[in] strPIN - active pin corresponding to the sig chain
         *
         *  @return true if block successfully signed
         *
         **/
        bool SignBlock(const memory::encrypted_ptr<TAO::Ledger::SignatureChain>& user, const SecureString& strPIN);


        /** TritiumMinterThread
         *
         *  Method run on its own thread to oversee stake minter operation using the methods in the
         *  tritium minter instance. The thread will continue running after initialized, but operation can
         *  be stopped/restarted by using the stake minter methods.
         *
         *  On shutdown, the thread will cease operation and wait for the minter destructor to tell it to exit/join.
         *
         *  @param[in] pTritiumMinter - the minter thread will use this instance to perform all the tritium minter work
         *
         **/
        static void TritiumMinterThread(TritiumMinter* pTritiumMinter);

        };

    }
}

#endif
