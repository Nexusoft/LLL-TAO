/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLP_INCLUDE_VALIDATION_THREAD_POOL_H
#define NEXUS_LLP_INCLUDE_VALIDATION_THREAD_POOL_H

#include <LLC/types/uint1024.h>

#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <vector>
#include <atomic>
#include <future>
#include <memory>

namespace LLP
{
    /** ValidationTask
     *
     *  Represents a block validation task to be processed by the thread pool.
     *
     **/
    struct ValidationTask
    {
        uint512_t hashMerkleRoot;           // Block identifier
        uint64_t nNonce;                    // Block nonce
        uint32_t nSessionId;                // Session that submitted
        std::function<bool()> fnValidate;   // Validation function
        std::promise<bool> promise;         // Result promise

        ValidationTask()
        : hashMerkleRoot(0)
        , nNonce(0)
        , nSessionId(0)
        , fnValidate(nullptr)
        {
        }

        ValidationTask(
            const uint512_t& merkle,
            uint64_t nonce,
            uint32_t session,
            std::function<bool()> fn)
        : hashMerkleRoot(merkle)
        , nNonce(nonce)
        , nSessionId(session)
        , fnValidate(std::move(fn))
        {
        }
    };


    /** ValidationThreadPool
     *
     *  Thread pool for parallel block validation to prevent bottlenecks
     *  during high miner traffic.
     *
     *  Design:
     *  - Fixed number of worker threads based on hardware
     *  - Queue-based task distribution for fair processing
     *  - Future-based result retrieval for async operation
     *
     **/
    class ValidationThreadPool
    {
    private:
        /** Worker threads **/
        std::vector<std::thread> vWorkers;

        /** Task queue **/
        std::queue<std::shared_ptr<ValidationTask>> queueTasks;

        /** Queue mutex **/
        std::mutex MUTEX;

        /** Condition variable for worker notification **/
        std::condition_variable CONDITION;

        /** Shutdown flag **/
        std::atomic<bool> fShutdown;

        /** Number of active workers **/
        std::atomic<uint32_t> nActiveWorkers;

        /** Total tasks processed **/
        std::atomic<uint64_t> nTasksProcessed;

        /** Total tasks pending **/
        std::atomic<uint64_t> nTasksPending;

    public:
        /** Constructor
         *
         *  Create thread pool with specified number of workers.
         *
         *  @param[in] nThreads Number of worker threads (0 = auto-detect)
         *
         **/
        explicit ValidationThreadPool(uint32_t nThreads = 0);

        /** Destructor **/
        ~ValidationThreadPool();

        /** Submit
         *
         *  Submit a validation task to the pool.
         *
         *  @param[in] hashMerkle Block merkle root
         *  @param[in] nNonce Block nonce
         *  @param[in] nSessionId Session identifier
         *  @param[in] fnValidate Validation function
         *
         *  @return Future for the validation result
         *
         **/
        std::future<bool> Submit(
            const uint512_t& hashMerkle,
            uint64_t nNonce,
            uint32_t nSessionId,
            std::function<bool()> fnValidate);

        /** GetPendingCount
         *
         *  Get number of pending tasks.
         *
         *  @return Number of tasks waiting
         *
         **/
        uint64_t GetPendingCount() const;

        /** GetProcessedCount
         *
         *  Get number of processed tasks.
         *
         *  @return Number of tasks completed
         *
         **/
        uint64_t GetProcessedCount() const;

        /** GetActiveWorkers
         *
         *  Get number of currently active workers.
         *
         *  @return Number of workers processing
         *
         **/
        uint32_t GetActiveWorkers() const;

        /** GetThreadCount
         *
         *  Get total number of worker threads.
         *
         *  @return Number of threads in pool
         *
         **/
        uint32_t GetThreadCount() const;

        /** Shutdown
         *
         *  Gracefully shutdown the thread pool.
         *
         **/
        void Shutdown();

        /** IsRunning
         *
         *  Check if the pool is running.
         *
         *  @return true if pool is active
         *
         **/
        bool IsRunning() const;

    private:
        /** WorkerLoop
         *
         *  Main worker thread loop.
         *
         **/
        void WorkerLoop();
    };


    /** GetValidationPool
     *
     *  Get the global validation thread pool instance.
     *
     *  @return Pointer to global pool
     *
     **/
    ValidationThreadPool* GetValidationPool();


    /** InitializeValidationPool
     *
     *  Initialize the global validation thread pool.
     *
     *  @param[in] nThreads Number of worker threads (0 = auto)
     *
     **/
    void InitializeValidationPool(uint32_t nThreads = 0);


    /** ShutdownValidationPool
     *
     *  Shutdown and cleanup the global validation thread pool.
     *
     **/
    void ShutdownValidationPool();

} // namespace LLP

#endif
