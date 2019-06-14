/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LEGACY_TYPES_MINTER_H
#define NEXUS_LEGACY_TYPES_MINTER_H

#include <Legacy/types/legacy.h>
#include <Legacy/types/trustkey.h>
#include <Legacy/wallet/reservekey.h>
#include <Legacy/wallet/wallet.h>

#include <LLC/types/uint1024.h>

#include <TAO/Ledger/types/base_minter.h>

#include <atomic>
#include <thread>


namespace Legacy
{
    /** @class LegacyMinter
      *
      * This class performs all operations for mining legacy blocks on the Proof of Stake channel.
      * Intialize the LegacyMinter by calling GetInstance() the first time.
      *
      * Staking does not start, though, until Start() is called for the first time.
      * This retrieves a wallet reference and begins staking for the current legacy wallet.
      *
      * The trust key and balance from the wallet will be used for Proof of Stake.
      * Mined PoS blocks will add coinstake transactions to this wallet and move balance
      * to the wallet's trust key. If the wallet does not have a trust key to start, one will be
      * created from its key pool and the minter will mine for its Genesis transaction.
      *
      * Staking operations can be suspended by calling Stop (for example, if the wallet is locked)
      * and restarted by calling Start() again.
      *
      **/
    class LegacyMinter final : public TAO::Ledger::StakeMinter
    {
    public:
        /** Copy constructor deleted **/
        LegacyMinter(const LegacyMinter&) = delete;

        /** Copy assignment deleted **/
        LegacyMinter& operator=(const LegacyMinter&) = delete;


        /** Destructor **/
        ~LegacyMinter();


        /** GetInstance
          *
          * Retrieves the LegacyMinter.
          *
          * @return reference to the LegacyMinter instance
          *
          **/
        static LegacyMinter& GetInstance();


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
          * If the system is configured not to run the LegacyMinter, this method will return false.
          * By default, the LegacyMinter will run for non-server, and won't run for server/daemon.
          * These defaults can be changed using the -stake setting.
          *
          * After calling this method, the LegacyMinter thread may stay in suspended state if
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
        static std::atomic<bool> fisStarted;


        /** Flag to tell the stake minter thread to stop processing and exit. **/
        static std::atomic<bool> fstopMinter;


        /** Thread for operating the stake minter **/
        static std::thread minterThread;


        /** The wallet where the stake minter will operate. **/
        Wallet* pStakingWallet;


        /** Trust key for staking. IsNull() is true when staking for Genesis. **/
        TrustKey trustKey;


        /** Reserved key to use for Genesis. nullptr when staking for Trust **/
        ReserveKey* pReservedTrustKey;


        /** The candidate block that the stake minter is attempting to mine */
        LegacyBlock block;


        /** Default constructor **/
        LegacyMinter()
        : StakeMinter()
        , pStakingWallet(nullptr)
        , trustKey(TrustKey())
        , pReservedTrustKey(nullptr)
        , block(LegacyBlock())
        {
        }


        /** FindTrustKey
         *
         *  Gets the trust key for the current wallet. If none exists, retrieves a new
         *  key from the key pool to use as the trust key for Genesis.
         *
         **/
        void FindTrustKey();


        /** CreateCandidateBlock
         *
         *  Creates a new legacy block that the stake minter will attempt to mine via the Proof of Stake process.
         *
         *  @return true if the candidate block was successfully created
         *
         **/
        bool CreateCandidateBlock();


        /** CalculateWeights
         *
         *  Calculates the Trust Weight and Block Weight values for the current trust key and candidate block.
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
         **/
        void MintBlock();


        /** ProcessBlock
         *
         *  Processes a newly mined Proof of Stake block, adds transactions from the mempool, and submits it
         *  to the network
         *
         *  @return true if the block passed all process checks and was successfully submitted
         *
         **/
        bool ProcessBlock();


        /** LegacyMinterThread
         *
         *  Method run on its own thread to oversee stake minter operation using the methods in the
         *  stake minter instance. The thread will continue running after initialized, but operation can
         *  be stopped/restarted by using the stake minter methods.
         *
         *  On shutdown, the thread will cease operation and wait for the stake minter
         *  destructor to tell it to exit/join.
         *
         *  @param[in] pLegacyMinter - the minter thread will use this instance to perform all the stake minter work
         *
         **/
        static void LegacyMinterThread(LegacyMinter* pLegacyMinter);

    };
}

#endif
