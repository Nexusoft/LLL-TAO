/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_LEGACY_TYPES_MINTER_H
#define NEXUS_LEGACY_TYPES_MINTER_H


#include <thread>

#include <Legacy/wallet/reservekey.h>
#include <Legacy/wallet/wallet.h>

#include <LLC/types/uint1024.h>

#include <TAO/Ledger/types/trustkey.h>
#include <TAO/Ledger/types/tritium.h>

#include <Util/include/mutex.h>


namespace Legacy 
{
    /** @class StakeMinter
      *
      * This class performs all operations for mining on the Proof of Stake channel.
      * Initializing the StakeMinter by calling GetInstance() the first time will start
      * the stake minter thread, which uses the private methods of the StakeMinter to perform
      * Proof of Stake.
      *
      * Staking does not start, though, until StartStakeMinter() is called for the first time.
      * This retrieves a wallet reference and begins staking for the wallet.
      *
      * The trust key and balance from the wallet will be used for Proof of Stake.
      * Mined PoS blocks will add coinstake transactions to this wallet and move balance 
      * to the wallet's trust key. If the wallet does not have a trust key to start, one will be 
      * created from its key pool and the minter will mine for its Genesis transaction.
      *
      * Staking operations can be suspended by calling StopStakeMinter (for example, if the wallet is locked)
      * and restarted by calling StartStakeMinter() again.
      *
      **/
    class StakeMinter final
    {
    public:
        /** GetInstance
          *
          * Retrieves the StakeMinter.
          *
          * @return reference to the StakeMinter instance
          *
          **/
        static StakeMinter& GetInstance();


        /** IsStarted
          * 
          * Tests whether or not the stake minter is currently running.
          * 
          * @return true if the stake minter is started, false otherwise
          *
          */
        inline bool IsStarted() const;


        /** GetBlockWeight
          * 
          * Retrieves the current internal value for the block weight metric. 
          * 
          * @return value of current block weight
          *
          */
        double GetBlockWeight() const;


        /** GetBlockWeight
          * 
          * Retrieves the block weight metric as a percentage of maximum. 
          * 
          * @return the current block weight percentage
          *
          */
        double GetBlockWeightPercent() const;


        /** GetTrustWeight
          * 
          * Retrieves the current internal value for the trust weight metric. 
          * 
          * @return value of current trust weight
          *
          */
        double GetTrustWeight() const;


        /** GetTrustWeight
          * 
          * Retrieves the trust weight metric as a percentage of maximum. 
          * 
          * @return the current trust weight percentage
          *
          */
        double GetTrustWeightPercent() const;


        /** IsWaitPeriod
          * 
          * Checks whether the stake minter is waiting for average coin
          * age to reach the required minimum before staking Genesis.
          * 
          * @return true if minter is waiting on coin age, false otherwise
          *
          */
        bool IsWaitPeriod() const;


        /** StartStakeMinter
          * 
          * Start the stake minter.
          *
          * Call this method to start the stake minter and begin mining Proof of Statke, or 
          * to restart it after it was stopped.
          *
          * The first time this method is called, it will retrieve a reference to the wallet 
          * by calling CWallet::GetInstance(), so the wallet should be initialized before
          * starting the stake minter.
          *
          * In general, this method should be called when the wallet is unlocked.
          *
          * After calling this method, the StakeMinter may stay in suspended state if
          * the local node is synchronizing, or if it does not have any connections, yet.
          * In that case, it will automatically begin when sync is complete and connections 
          * are available.
          * 
          * @return true if the stake minter was started, false if it was already running
          *
          */
        bool StartStakeMinter();


        /** StopStakeMinter
          * 
          * Stops the stake minter. 
          *
          * Call this method to suspend the stake minter. This suspends Proof of Stake mining, 
          * though the stake minter thread remains operational, so mining and can be restarted
          * via a subsequent call to StartStakeMinter(). 
          *
          * Should be called whenever the wallet is locked.
          * 
          * @return true if the stake minter was stopped, false if it was already stopped
          *
          */
        bool StopStakeMinter();


        /** Destructor 
          *
          * Signals the stake minter thread to shut down and waits for it to join
          *
          **/
        ~StakeMinter();


    private:
        /** Mutex for stake minter thread synchronization **/
        static std::mutex cs_stakeMinter;


        /** Set true when stake miner starts processing and remains true while it is running **/
        static bool fisStarted;


        /** Flag to tell the stake minter thread to suspend processing. Will reset when stake minter is restarted.**/
        static bool fstopMinter;


        /** Flag to tell the stake minter thread that the minter is being destructed and it should end/join **/
        static bool fdestructMinter;


        /** Thread for operating the stake minter **/
        std::thread minterThread;


        /** The wallet where the stake minter will operate. **/
        CWallet* pStakingWallet;


        TrustKey trustKey;

        ReserveKey* pReservedTrustKey;

        uint1024_t hashLastBlock;

        TritiumBlock candidateBlock;

        uint64_t nSleepTime;

        bool fIsWaitPeriod;

        double nTrustWeight;

        double nBlockWeight;

        double nRequired;

        double nThreshhold;


        /** Default constructor **/
        StakeMinter() 
        : minterThread(StakeMinter::StakeMinterThread, this)
        , pStakingWallet(nullptr)
        , trustKey(TrustKey())
        , pReservedTrustKey(nullptr)
        , hashLastBlock(0)
        , pCandidateBlock(TritiumBlock())
        , nSleepTime(1000)
        , fIsWaitPeriod(false)
        , nTrustWeight(0.0)
        , nBlockWeight(0.0)
        , nRequired(0.0)
        , nThreshhold(0.0)
        {
        }


        /** FindTrustKey
          *
          * Gets the trust key for the current wallet. If none exists, retrieves a new
          * key from the key pool to use as the trust key for Genesis.
          *
          * @param[in,out] reservedTrustKey - ReserveKey for trust key, used to obtain a new key from key pool if needed
          *
          * @return true if is successfully retrieved a trust key or used a new one, false otherwise
          * 
          **/
        bool FindTrustKey(ReserveKey& reservedTrustKey);


        /** ResetMinter
          *
          * Resets the stake minter settings if the network finds a block
          * and the minter needs to start on a new one.
          *
          */
        void ResetMinter();


        bool CreateCandidateBlock();


        bool CalculateWeights();


        bool MineProofOfStake();


        bool ProcessMinedBlock();


        /** StakeMinterThread
          *
          * Method run on its own thread to oversee stake minter operation. The thread will
          * continue running after initialized, but operation can be stopped/restarted by
          * using the stake minter methods.
          *
          * On shutdown, the thread will cease operation and wait for the stake minter
          * destructor to tell it to exit/join.
          *
          * @param[in] pstakeMinter - the minter thread will use this instance to perform all the stake minter work
          * 
          **/
        static void StakeMinterThread(StakeMinter* pstakeMinter);

    }
}

#endif
