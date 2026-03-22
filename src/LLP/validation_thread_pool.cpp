/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLP/include/validation_thread_pool.h>
#include <Util/include/debug.h>

#include <algorithm>

namespace LLP
{
    /** Configuration constants for validation thread pool **/
    static const uint32_t MIN_VALIDATION_THREADS = 2;       /* Minimum worker threads */
    static const uint32_t DEFAULT_VALIDATION_THREADS = 4;   /* Default if detection fails */
    static const uint32_t THREAD_RATIO_DIVISOR = 2;         /* Use 1/N of available cores */

    /** Global validation pool instance **/
    static std::unique_ptr<ValidationThreadPool> pValidationPool = nullptr;
    static std::mutex POOL_MUTEX;


    /** Constructor **/
    ValidationThreadPool::ValidationThreadPool(uint32_t nThreads)
    : vWorkers()
    , queueTasks()
    , MUTEX()
    , CONDITION()
    , fShutdown(false)
    , nActiveWorkers(0)
    , nTasksProcessed(0)
    , nTasksPending(0)
    {
        /* Auto-detect thread count if not specified */
        if(nThreads == 0)
        {
            nThreads = std::thread::hardware_concurrency();
            if(nThreads == 0)
                nThreads = DEFAULT_VALIDATION_THREADS;

            /* Use fraction of available cores for validation */
            nThreads = std::max(MIN_VALIDATION_THREADS, nThreads / THREAD_RATIO_DIVISOR);
        }

        debug::log(0, FUNCTION, "Starting validation thread pool with ", nThreads, " workers");

        /* Create worker threads */
        vWorkers.reserve(nThreads);
        for(uint32_t i = 0; i < nThreads; ++i)
        {
            vWorkers.emplace_back(&ValidationThreadPool::WorkerLoop, this);
        }
    }


    /** Destructor **/
    ValidationThreadPool::~ValidationThreadPool()
    {
        Shutdown();
    }


    /** Submit a validation task **/
    std::future<bool> ValidationThreadPool::Submit(
        const uint512_t& hashMerkle,
        uint64_t nNonce,
        uint32_t nSessionId,
        std::function<bool()> fnValidate)
    {
        auto pTask = std::make_shared<ValidationTask>(hashMerkle, nNonce, nSessionId, std::move(fnValidate));
        std::future<bool> result = pTask->promise.get_future();

        {
            std::unique_lock<std::mutex> lock(MUTEX);

            if(fShutdown)
            {
                pTask->promise.set_value(false);
                return result;
            }

            queueTasks.push(pTask);
            ++nTasksPending;
        }

        CONDITION.notify_one();

        debug::log(3, FUNCTION, "Queued validation task for merkle=", hashMerkle.SubString(),
                   " nonce=", nNonce, " session=", nSessionId,
                   " pending=", nTasksPending.load());

        return result;
    }


    /** Get pending count **/
    uint64_t ValidationThreadPool::GetPendingCount() const
    {
        return nTasksPending.load();
    }


    /** Get processed count **/
    uint64_t ValidationThreadPool::GetProcessedCount() const
    {
        return nTasksProcessed.load();
    }


    /** Get active workers **/
    uint32_t ValidationThreadPool::GetActiveWorkers() const
    {
        return nActiveWorkers.load();
    }


    /** Get thread count **/
    uint32_t ValidationThreadPool::GetThreadCount() const
    {
        return static_cast<uint32_t>(vWorkers.size());
    }


    /** Shutdown **/
    void ValidationThreadPool::Shutdown()
    {
        {
            std::unique_lock<std::mutex> lock(MUTEX);
            if(fShutdown)
                return;
            fShutdown = true;
        }

        debug::log(0, FUNCTION, "Shutting down validation thread pool...");

        /* Wake up all workers */
        CONDITION.notify_all();

        /* Wait for workers to finish */
        for(auto& worker : vWorkers)
        {
            if(worker.joinable())
                worker.join();
        }

        debug::log(0, FUNCTION, "Validation thread pool shutdown complete. Processed ",
                   nTasksProcessed.load(), " tasks.");
    }


    /** Check if running **/
    bool ValidationThreadPool::IsRunning() const
    {
        return !fShutdown.load();
    }


    /** Worker loop **/
    void ValidationThreadPool::WorkerLoop()
    {
        while(true)
        {
            std::shared_ptr<ValidationTask> pTask;

            {
                std::unique_lock<std::mutex> lock(MUTEX);

                /* Wait for task or shutdown */
                CONDITION.wait(lock, [this] {
                    return fShutdown || !queueTasks.empty();
                });

                if(fShutdown && queueTasks.empty())
                    return;

                /* Get next task */
                pTask = queueTasks.front();
                queueTasks.pop();
                --nTasksPending;
            }

            /* Process task */
            ++nActiveWorkers;

            bool fResult = false;
            try
            {
                debug::log(3, FUNCTION, "Processing validation for merkle=",
                           pTask->hashMerkleRoot.SubString());

                if(pTask->fnValidate)
                    fResult = pTask->fnValidate();
            }
            catch(const std::exception& e)
            {
                debug::error(FUNCTION, "Validation exception: ", e.what());
                fResult = false;
            }

            /* Set result */
            try
            {
                pTask->promise.set_value(fResult);
            }
            catch(...)
            {
                /* Promise may already be fulfilled */
            }

            ++nTasksProcessed;
            --nActiveWorkers;

            debug::log(3, FUNCTION, "Validation complete for merkle=",
                       pTask->hashMerkleRoot.SubString(), " result=", fResult ? "accepted" : "rejected");
        }
    }


    /** Get global pool **/
    ValidationThreadPool* GetValidationPool()
    {
        std::lock_guard<std::mutex> lock(POOL_MUTEX);
        return pValidationPool.get();
    }


    /** Initialize global pool **/
    void InitializeValidationPool(uint32_t nThreads)
    {
        std::lock_guard<std::mutex> lock(POOL_MUTEX);
        if(!pValidationPool)
        {
            pValidationPool = std::make_unique<ValidationThreadPool>(nThreads);
        }
    }


    /** Shutdown global pool **/
    void ShutdownValidationPool()
    {
        std::lock_guard<std::mutex> lock(POOL_MUTEX);
        if(pValidationPool)
        {
            pValidationPool->Shutdown();
            pValidationPool.reset();
        }
    }

} // namespace LLP
