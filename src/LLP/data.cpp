/*__________________________________________________________________________________________

			Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

			(c) Copyright The Nexus Developers 2014 - 2025

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLP/include/base_address.h>
#include <LLP/templates/data.h>
#include <LLP/templates/static.h>
#include <LLP/templates/socket.h>

#include <chrono>

#include <LLP/types/tritium.h>
#include <LLP/types/time.h>
#include <LLP/types/apinode.h>
#include <LLP/types/filenode.h>
#include <LLP/types/rpcnode.h>
#include <LLP/types/miner.h>
#include <LLP/types/lookup.h>
#include <LLP/types/stateless_miner_connection.h>

#include <Util/include/hex.h>

#include <type_traits>


namespace LLP
{
    namespace
    {
        static constexpr uint32_t MINING_POLL_EMPTY_TIMEOUT_MS = 5000;

        /** Maximum time (milliseconds) a partial packet (header read but data
         *  incomplete) may remain stuck before the connection is killed.
         *  This catches the case where a miner sends a header + length but the
         *  remaining bytes never arrive, jamming the read pipeline while the
         *  write pipeline (PUSH notifications) continues to work — the
         *  "shadow ban" scenario. 30 seconds is generous for any legitimate
         *  frame over any network link. */
        static constexpr uint32_t PARTIAL_PACKET_TIMEOUT_MS = 30000;
    }

    /** Default Constructor **/
    template <class ProtocolType>
    DataThread<ProtocolType>::DataThread(const uint32_t nID, const bool ffDDOSIn,
                                         const uint32_t rScore, const uint32_t cScore,
                                         const uint32_t nTimeout, const bool fMeter)
    : fDDOS           (ffDDOSIn)
    , fMETER          (fMeter)
    , fDestruct       (false)
    , nIncoming       (0)
    , nOutbound       (0)
    , ID              (nID)
    , TIMEOUT         (nTimeout)
    , DDOS_rSCORE     (rScore)
    , DDOS_cSCORE     (cScore)
    , CONNECTIONS     (util::atomic::lock_unique_ptr<std::vector<std::shared_ptr<ProtocolType>> >
        (new std::vector<std::shared_ptr<ProtocolType>>()))
    , RELAY           (util::atomic::lock_unique_ptr<std::queue<std::pair<typename ProtocolType::message_t, DataStream>> >
        (new std::queue<std::pair<typename ProtocolType::message_t, DataStream>>()))
    , CONDITION       ( )
    , DATA_THREAD     (std::bind(&DataThread::Thread, this))
    , FLUSH_CONDITION ( )
    , FLUSH_THREAD    (std::bind(&DataThread::Flush, this))
    {
    }


    /** Default Destructor **/
    template <class ProtocolType>
    DataThread<ProtocolType>::~DataThread()
    {
        debug::log(2, FUNCTION, "Shutting down data thread ", ID);

        fDestruct = true;
        CONDITION.notify_all();
        FLUSH_CONDITION.notify_all();

        /* ── Timeout-guarded join ─────────────────────────────────────────────
         * Use a timed join instead of a blocking join() to prevent infinite
         * stall when a connected miner's socket is stuck in OS-level cleanup
         * (TCP TIME_WAIT). poll() has a 100ms timeout per iteration, but with
         * multiple connections and slow kernel teardown, join() can block for
         * seconds or indefinitely.
         *
         * If the thread does not exit within SHUTDOWN_JOIN_TIMEOUT_MS, we
         * detach it and let the OS clean up on process exit. The hard-exit
         * watchdog in signals.cpp provides the final safety net.
         */
        constexpr uint32_t SHUTDOWN_JOIN_TIMEOUT_MS = 500;

        auto join_with_timeout = [&](std::thread& t, const char* name)
        {
            if(!t.joinable())
                return;

            /* Move the thread handle and joined flag into shared_ptrs so the
             * waiter thread holds shared ownership — no dangling references if
             * join_with_timeout returns before the waiter finishes. */
            auto sp_thread = std::make_shared<std::thread>(std::move(t));
            auto sp_joined = std::make_shared<std::atomic<bool>>(false);

            /* Spawn a waiter thread to perform the actual join. */
            std::thread waiter([sp_thread, sp_joined]()
            {
                if(sp_thread->joinable())
                    sp_thread->join();
                sp_joined->store(true);
            });

            /* Poll until joined or deadline reached. */
            const auto deadline = std::chrono::steady_clock::now() +
                                  std::chrono::milliseconds(SHUTDOWN_JOIN_TIMEOUT_MS);

            while(!sp_joined->load() && std::chrono::steady_clock::now() < deadline)
                runtime::sleep(10);

            if(sp_joined->load())
            {
                waiter.join();
                debug::log(2, FUNCTION, "  ", name, " joined cleanly for thread ", ID);
            }
            else
            {
                /* Thread did not exit in time — detach the waiter and let it
                 * run to completion independently. The shared_ptrs keep the
                 * thread handle and flag alive until the waiter exits.
                 * The hard-exit watchdog in signals.cpp will force-terminate
                 * the process if graceful shutdown does not complete within
                 * 8 seconds. */
                debug::error(FUNCTION, "  ", name, " (thread ", ID, ") did not exit within ",
                             SHUTDOWN_JOIN_TIMEOUT_MS, "ms — detaching (watchdog will force exit)");
                waiter.detach();
            }
        };

        join_with_timeout(DATA_THREAD,  "DATA_THREAD");
        join_with_timeout(FLUSH_THREAD, "FLUSH_THREAD");

        debug::log(2, FUNCTION, "Data thread ", ID, " shutdown complete");
    }


    /* Establishes a new connection and adds it to current Data Thread and returns the active connection pointer. */
    template <class ProtocolType>
    bool DataThread<ProtocolType>::NewConnection(const BaseAddress &addr, DDOS_Filter* DDOS, const bool& fSSL, std::shared_ptr<ProtocolType> &pNodeRet)
    {
        try
        {
            /* Create a new pointer on the heap. */
            ProtocolType* pnode = new ProtocolType(nullptr, false); //turn off DDOS for outgoing connections

            /* Set the SSL flag */
            pnode->SetSSL(fSSL);

            /* Attempt to make the connection. */
            if(!pnode->Connect(addr))
            {
                delete pnode;
                return false;
            }

            /* Update the indexes. */
            pnode->nDataThread     = ID;
            pnode->FLUSH_CONDITION = &FLUSH_CONDITION;

            /* Check for inbound socket. */
            if(pnode->Incoming())
                ++nIncoming;
            else
                ++nOutbound;

            /* Find an avilable data thread slot. */
            const uint32_t nSlot = find_slot();

            /* Fire the connected event. */
            pnode->nDataIndex = nSlot;
            pnode->Event(EVENTS::CONNECT);

            /* Find a slot that is empty. */
            if(nSlot == CONNECTIONS->size())
                CONNECTIONS->push_back(std::shared_ptr<ProtocolType>(pnode));
            else
                CONNECTIONS->at(nSlot) = std::shared_ptr<ProtocolType>(pnode);

            /* Notify data thread to wake up. */
            CONDITION.notify_all();

        }
        catch(const std::runtime_error& e)
        {
            debug::error(FUNCTION, e.what()); //catch any atomic_ptr exceptions

            return false;
        }

        return true;
    }


    /*  Disconnects all connections by issuing a DISCONNECT::FORCE event message
     *  and then removes the connection from this data thread. */
    template <class ProtocolType>
    void DataThread<ProtocolType>::DisconnectAll()
    {
        /* Iterate through connections to remove.*/
        const uint32_t nSize = CONNECTIONS->size();
        for(uint32_t nIndex = 0; nIndex < nSize; ++nIndex)
        {
            /* When on destruct or shutdown, remove the connection without events. */
            if(fDestruct.load() || config::fShutdown.load())
                remove_connection(nIndex);

            /* Otherwise, remove with events to inform the address manager so it knows to re-attempt this connection. */
            else
                remove_connection_with_event(nIndex, DISCONNECT::FORCE);
        }
    }

    /* Release all pending triggers from BlockingMessages */
    template <class ProtocolType>
    void DataThread<ProtocolType>::NotifyTriggers()
    {
        /* Iterate through connections to remove.*/
        const uint32_t nSize = CONNECTIONS->size();
        for(uint32_t nIndex = 0; nIndex < nSize; ++nIndex)
        {
            /* Skip over inactive connections. */
            if(!CONNECTIONS->at(nIndex))
                continue;

            /* Notify connection triggers. */
            CONNECTIONS->at(nIndex)->NotifyTriggers();
        }
    }


    /*  Thread that handles all the Reading / Writing of Data from Sockets.
     *  Creates a Packet QUEUE on this connection to be processed by an
     *  LLP Messaging Thread. */
    template <class ProtocolType>
    void DataThread<ProtocolType>::Thread()
    {
        /* Cache sleep time if applicable. */
        const uint32_t nSleep = config::GetArg("-llpsleep", 0);
        const uint32_t nWait  = config::GetArg("-llpwait", 1);

        /* The mutex for the condition. */
        std::mutex CONDITION_MUTEX;

        /* This mirrors CONNECTIONS with pollfd settings for passing to poll methods.
         * Windows throws SOCKET_ERROR intermittently if pass CONNECTIONS directly.
         */
        std::vector<pollfd> POLLFDS;

        /* The main connection handler loop. */
        while(!fDestruct.load() && !config::fShutdown.load())
        {
            /* Check for data thread sleep (helps with cpu usage). */
            if(nSleep > 0)
                runtime::sleep(nSleep);

            /* Keep data threads waiting for work.
             * Will wait until have one or more connections, DataThread is disposed, or system shutdown
             * While loop catches potential for spurious wakeups. Also has the effect of skipping the wait() call after connections established.
             */
            std::unique_lock<std::mutex> CONDITION_LOCK(CONDITION_MUTEX);
            CONDITION.wait(CONDITION_LOCK,
            [this]
            {
                /* CRITICAL: Check for shutdown FIRST, before checking suspend state.
                 * This ensures the thread wakes up during shutdown even if protocol is suspended.
                 * Bug fix: Previously checked fSuspendProtocol first, causing shutdown hang. */
                if(fDestruct.load() || config::fShutdown.load())
                    return true;

                /* Check for suspended state. */
                if(config::fSuspendProtocol.load())
                    return false;

                /* Wake up if there are active connections to process. */
                return nIncoming.load() > 0 || nOutbound.load() > 0;
            });

            /* Check for close. */
            if(fDestruct.load() || config::fShutdown.load())
            {
                debug::log(2, FUNCTION, "DATA_THREAD ", ID, " exiting: shutdown requested");
                return;
            }

            /* Check if we are suspended. */
            if(config::fSuspendProtocol.load())
            {
                runtime::sleep(100);
                continue;
            }

            /* Wrapped mutex lock. */
            const uint32_t nSize = static_cast<uint32_t>(CONNECTIONS->size());

            /* Check the pollfd's size. */
            if(POLLFDS.size() != nSize)
                POLLFDS.resize(nSize);

            /* Initialize the revents for all connection pollfd structures.
            * One connection must be live, so verify that and skip if none
            */
            for(uint32_t nIndex = 0; nIndex < nSize; ++nIndex)
            {
                try
                {
                    /* Set the proper POLLIN flags. */
                    POLLFDS.at(nIndex).events  = POLLIN;// | POLLRDHUP;
                    POLLFDS.at(nIndex).revents = 0; //reset return events

                    /* Set to invalid socket if connection is inactive. */
                    if(!CONNECTIONS->at(nIndex))
                    {
                        POLLFDS.at(nIndex).fd = INVALID_SOCKET;

                        continue;
                    }

                    /* Set the correct file descriptor. */
                    POLLFDS.at(nIndex).fd = CONNECTIONS->at(nIndex)->fd;
                }
                catch(const std::exception& e)
                {
                    debug::error(FUNCTION, e.what());
                }
            }

            /* Poll the sockets. */
#ifdef WIN32
            int32_t nPoll = WSAPoll((pollfd*)&POLLFDS[0], nSize, 100);
#else
            int32_t nPoll = poll((pollfd*)&POLLFDS[0], nSize, 100);
#endif

            /* Check poll for available sockets. */
            if(nPoll < 0)
            {
                runtime::sleep(1);
                continue;
            }


            /* Check all connections for data and packets. */
            for(uint32_t nIndex = 0; nIndex < nSize; ++nIndex)
            {
                /* Break early if shutdown signaled mid-iteration. */
                if(fDestruct.load() || config::fShutdown.load())
                    break;

                /* Access the shared pointer. */
                std::shared_ptr<ProtocolType> CONNECTION = CONNECTIONS->at(nIndex);
                try
                {
                    /* Skip over Inactive Connections. */
                    if(!CONNECTION || !CONNECTION->Connected())
                        continue;
                    
                    /* Detect stateless Miner connections for special handling.
                     * All Miner connections now use stateless protocol (no session required).
                     * Resolved at compile time since ProtocolType is a template parameter. */
                    constexpr bool fStatelessMiner = std::is_same<ProtocolType, Miner>::value;
                    
                    /* Log data thread connection assignment at verbose level 3. */
                    if(config::nVerbose.load() >= 3 && CONNECTION->PacketComplete())
                    {
                        debug::log(3, FUNCTION, "DataThread[", ID, "]: Processing connection id=", nIndex, 
                                   " type=", ProtocolType::Name(), 
                                   " from ", CONNECTION->GetAddress().ToStringIP(), ":", CONNECTION->GetAddress().GetPort(),
                                   " stateless=", (fStatelessMiner ? "true" : "false"));
                    }

                    /* Disconnect if there was a polling error */
                    if(POLLFDS.at(nIndex).revents & POLLERR)
                    {
                         remove_connection_with_event(nIndex, DISCONNECT::POLL_ERROR);
                         continue;
                    }

                    /* Disconnect if the socket was disconnected by peer (need for Windows) */
                    if(POLLFDS.at(nIndex).revents & POLLHUP)
                    {
                        remove_connection_with_event(nIndex, DISCONNECT::PEER);
                        continue;
                    }

                    /* Remove Connection if it has Timed out or had any read/write Errors. */
                    if(CONNECTION->Errors())
                    {
                        remove_connection_with_event(nIndex, DISCONNECT::ERRORS);
                        continue;
                    }

                    /* Remove Connection if it has Timed out or had any Errors.
                     * Non-exempt connections use the DataThread TIMEOUT (server-configured).
                     * Authenticated mining connections (IsTimeoutExempt == true) use the
                     * virtual GetReadTimeout() which returns a longer but finite value
                     * (default 600s / 10 minutes, configurable via -miningreadtimeout).
                     * This prevents a stalled read pipeline from persisting indefinitely
                     * while server-initiated PUSH notifications continue to work
                     * (the "shadow ban" scenario). */
                    {
                        const uint32_t nCustom = CONNECTION->GetReadTimeout();
                        const uint32_t nReadTimeout = (CONNECTION->IsTimeoutExempt() && nCustom > 0)
                            ? nCustom
                            : TIMEOUT * 1000;

                        if(CONNECTION->Timeout(nReadTimeout, Socket::READ))
                        {
                            remove_connection_with_event(nIndex, DISCONNECT::TIMEOUT);
                            continue;
                        }
                    }

                    /* Disconnect if pollin signaled with no data for 1ms consistently (This happens on Linux).
                     * Authenticated mining connections are exempt — a spurious POLLIN with
                     * Available()==0 on a 1 ms window is too aggressive for high-value Falcon-
                     * authenticated sessions.  The 24-hour session timeout and TCP keepalive
                     * probes will catch genuinely dead connections instead. */
                    const bool fHasPartialPacket =
                        !CONNECTION->INCOMING.IsNull() && !CONNECTION->PacketComplete();
                    const bool fMiningConnection =
                        std::is_same<ProtocolType, Miner>::value
                        || std::is_same<ProtocolType, StatelessMinerConnection>::value;
                    const bool fTimeoutExempt = CONNECTION->IsTimeoutExempt();
                    const uint32_t nPollEmptyTimeout =
                        fMiningConnection ? MINING_POLL_EMPTY_TIMEOUT_MS : nWait;
                    /* A partial packet already buffered in INCOMING means progress was made on
                     * a real frame, so treat the empty poll as transport jitter and never as a
                     * dead-socket signal.  We still apply the same timeout window first so the
                     * check only runs after the miner-specific grace period has elapsed. */
                    const bool fPollEmptyCandidate =
                        (POLLFDS.at(nIndex).revents & POLLIN)
                        && CONNECTION->Timeout(nPollEmptyTimeout, Socket::READ)
                        && CONNECTION->Available() == 0;
                    if(fPollEmptyCandidate && !fHasPartialPacket)
                    {
                        if(fTimeoutExempt)
                        {
                            /* Log near-miss for authenticated miners — this would have killed
                             * the connection prior to the IsTimeoutExempt() bypass.  Useful
                             * for diagnosing spurious POLLIN events from TCP keepalive, etc. */
                            debug::log(0, FUNCTION, "DataThread[", ID, "]: POLL_EMPTY near-miss for authenticated ",
                                ProtocolType::Name(), " from ", CONNECTION->GetAddress().ToStringIP(),
                                " revents=", POLLFDS.at(nIndex).revents,
                                " Available()=0 timeout=", nPollEmptyTimeout,
                                "ms — bypassed via IsTimeoutExempt()");
                        }
                        else
                        {
                            remove_connection_with_event(nIndex, DISCONNECT::POLL_EMPTY);
                            continue;
                        }
                    }

                    /* Disconnect if buffer is full and remote host isn't reading at all.
                     * Authenticated miners bypass this check via IsTimeoutExempt() because
                     * their receive window can temporarily close during CPU-intensive proof-
                     * of-work computation.  They use the virtual GetWriteTimeout() which
                     * returns a longer grace period (default 30s) vs the 5s P2P default. */
                    if(CONNECTION->Buffered()
                    && CONNECTION->Timeout(CONNECTION->GetWriteTimeout(), Socket::WRITE))
                    {
                        if(CONNECTION->IsTimeoutExempt())
                        {
                            /* Log near-miss for authenticated miners. */
                            debug::log(0, FUNCTION, "DataThread[", ID, "]: TIMEOUT_WRITE near-miss for authenticated ",
                                ProtocolType::Name(), " from ", CONNECTION->GetAddress().ToStringIP(),
                                " Buffered()=", CONNECTION->Buffered(),
                                " WriteTimeout=", CONNECTION->GetWriteTimeout(), "ms",
                                " — bypassed via IsTimeoutExempt()");
                        }
                        else
                        {
                            remove_connection_with_event(nIndex, DISCONNECT::TIMEOUT_WRITE);
                            continue;
                        }
                    }

                    /* Check that write buffers aren't overflowed. */
                    if(CONNECTION->Buffered() > CONNECTION->GetMaxSendBuffer())
                    {
                        /* Log at verbosity 0 for any connection whose buffer overflows. */
                        debug::log(0, FUNCTION, "DataThread[", ID, "]: BUFFER overflow for ",
                            ProtocolType::Name(), " from ", CONNECTION->GetAddress().ToStringIP(),
                            " Buffered()=", CONNECTION->Buffered(),
                            " MaxSendBuffer=", CONNECTION->GetMaxSendBuffer(),
                            " IsTimeoutExempt=", CONNECTION->IsTimeoutExempt());

                        remove_connection_with_event(nIndex, DISCONNECT::BUFFER);
                        continue;
                    }

                    /* PARTIAL-PACKET WATCHDOG (Option A)
                     * If a partial packet (header read, data incomplete) has been stuck
                     * for longer than PARTIAL_PACKET_TIMEOUT_MS, disconnect.  This is
                     * NOT gated by IsTimeoutExempt() — even authenticated miners get
                     * disconnected if a frame is stuck mid-read.
                     *
                     * The read-idle timer (nLastRecv / Socket::READ) tracks when the
                     * last bytes arrived on the socket.  If the partial packet's header
                     * was read but the remaining data bytes have not arrived within the
                     * timeout window, the frame is considered stalled — a condition
                     * that can leave the connection alive indefinitely while the write
                     * pipeline (PUSH) continues to work (shadow ban). */
                    if(fHasPartialPacket
                    && CONNECTION->Timeout(PARTIAL_PACKET_TIMEOUT_MS, Socket::READ))
                    {
                        debug::log(0, FUNCTION, "DataThread[", ID, "]: PARTIAL_STALL for ",
                            ProtocolType::Name(), " from ", CONNECTION->GetAddress().ToStringIP(),
                            " — incomplete frame stuck >", PARTIAL_PACKET_TIMEOUT_MS, "ms, disconnecting");

                        remove_connection_with_event(nIndex, DISCONNECT::PARTIAL_STALL);
                        continue;
                    }

                    /* Generic event for Connection. */
                    CONNECTION->Event(EVENTS::GENERIC);

                    /* Work on Reading a Packet. **/
                    CONNECTION->ReadPacket();

                    /* Handle any DDOS Filters. */
                    if(fDDOS.load() && CONNECTION->DDOS && !CONNECTION->addr.IsLocal())
                    {
                        /* Ban a node if it has too many Requests per Second. **/
                        if(CONNECTION->DDOS->rSCORE.Score() > DDOS_rSCORE)
                            CONNECTION->DDOS->Ban();

                        /* Remove a connection if it was banned by DDOS Protection. */
                        if(!CONNECTION->GetAddress().IsLocal() && CONNECTION->DDOS->Banned())
                        {
                            debug::log(0, ProtocolType::Name(), " BANNED: ", CONNECTION->GetAddress().ToString());
                            remove_connection_with_event(nIndex, DISCONNECT::DDOS);
                            continue;
                        }
                    }

                    /* If a Packet was received successfully, increment request count [and DDOS count if enabled]. */
                    if(CONNECTION->PacketComplete())
                    {
                        /* Debug dump of message type. */
                        if(config::nVerbose.load() >= 4)
                            debug::log(4, FUNCTION, "Received Message (", CONNECTION->INCOMING.GetBytes().size(), " bytes)");

                        /* Debug dump of packet data. */
                        if(config::nVerbose.load() >= 5)
                            PrintHex(CONNECTION->INCOMING.GetBytes());

                        /* Handle Meters and DDOS. */
                        if(fMETER)
                            ++ProtocolType::REQUESTS;

                        /* Packet Process return value of False will flag Data Thread to Disconnect. */
                        if(!CONNECTION->ProcessPacket())
                        {
                            remove_connection_with_event(nIndex, DISCONNECT::FORCE);
                            continue;
                        }

                        /* Increment rScore. */
                        if(fDDOS.load() && CONNECTION->DDOS)
                            CONNECTION->DDOS->rSCORE += 1;

                        /* Run procssed event for connection triggers. */
                        CONNECTION->Event(EVENTS::PROCESSED);
                        CONNECTION->ResetPacket();
                    }
                }
                catch(const std::exception& e)
                {
                    /* Get the connection for detailed logging. */
                    std::shared_ptr<ProtocolType> CONNECTION = CONNECTIONS->at(nIndex);
                    
                    /* Check if this is a "Session not found" error */
                    std::string strError = e.what();
                    bool fSessionError = (strError.find("Session not found") != std::string::npos);
                    
                    /* Check if this is a mining connection (stateless protocol doesn't use TAO API sessions).
                     * CRITICAL: Stateless mining can happen on two server types:
                     * 1. MINING_SERVER (port 9325) - uses Miner class (thin wrapper to StatelessMiner)
                     * 2. STATELESS_MINER_SERVER (port 8323) - uses StatelessMinerConnection class
                     * Both should be allowed to continue even if legacy code tries to access sessions. */
                    std::string strProtocol = ProtocolType::Name();
                    bool fMiningConnection = (strProtocol == "Miner" || strProtocol == "StatelessMiner");
                    
                    /* Allow mining connections to proceed even without TAO API session.
                     * The stateless mining protocol uses MiningContext state tracked by
                     * StatelessMinerManager, not TAO API sessions. */
                    if(fSessionError && fMiningConnection)
                    {
                        /* Log at level 0 so operators can see the session health issue.
                         * ProcessPacket should have already sent TEMPLATE_SOURCE_UNAVAILABLE
                         * to the miner; this is a safety-net log for unexpected code paths. */
                        debug::error(FUNCTION, "DataThread[", ID, "]: SESSION::DEFAULT not available"
                                     " for mining (", strProtocol, ") from ",
                                     CONNECTION->GetAddress().ToStringIP(), ":",
                                     CONNECTION->GetAddress().GetPort(),
                                     " — node requires -autologin=user:pass or manual"
                                     " unlock. Miner should receive TEMPLATE_SOURCE_UNAVAILABLE.");

                        /* Continue to next connection without disconnecting.
                         * The session may become available on the next request. */
                        continue;
                    }
                    else
                    {
                        /* For all other cases, maintain existing behavior: log error and disconnect. */
                        if(CONNECTION)
                        {
                            debug::log(1, FUNCTION, "DataThread[", ID, "]: Exception for connection id=", nIndex, 
                                       " type=", strProtocol, 
                                       " from ", CONNECTION->GetAddress().ToStringIP(), ":", CONNECTION->GetAddress().GetPort(),
                                       " - ", e.what());
                        }
                        
                        debug::error(FUNCTION, "Data Connection: ", e.what());
                        remove_connection_with_event(nIndex, DISCONNECT::ERRORS);
                    }
                }
            }
        }
    }


    /*  Thread that handles all the Reading / Writing of Data from Sockets.
     *  Creates a Packet QUEUE on this connection to be processed by an
     *  LLP Messaging Thread. */
    template <class ProtocolType>
    void DataThread<ProtocolType>::Flush()
    {
        /* The mutex for the condition. */
        std::mutex CONDITION_MUTEX;

        /* The main connection handler loop. */
        while(!fDestruct.load() && !config::fShutdown.load())
        {
            /* Keep data threads waiting for work.
             * Will wait until have one or more connections, DataThread is disposed, or system shutdown
             * While loop catches potential for spurious wakeups. Also has the effect of skipping the wait() call after connections established.
             */
            std::unique_lock<std::mutex> CONDITION_LOCK(CONDITION_MUTEX);
            FLUSH_CONDITION.wait(CONDITION_LOCK,
            [this]
            {
                /* Break on shutdown or destructor. */
                if(fDestruct.load() || config::fShutdown.load())
                    return true;

                /* Check for suspended state. */
                if(config::fSuspendProtocol.load())
                    return false;

                /* Check for data in the queue. */
                if(!RELAY->empty())
                    return true;

                /* Check for buffered connection. */
                const uint32_t nSize = CONNECTIONS->size();
                for(uint32_t nIndex = 0; nIndex < nSize; ++nIndex)
                {
                    try
                    {
                        /* Get shared pointer to prevent race condition on the internal connection pointer. */
                        std::shared_ptr<ProtocolType> CONNECTION = CONNECTIONS->at(nIndex);

                        /* Skip over Inactive Connections. */
                        if(!CONNECTION || !CONNECTION->Connected())
                            continue;

                        /* Check for buffered connection. */
                        if(CONNECTION->Buffered())
                            return true;
                    }
                    catch(const std::exception& e) { }
                }

                return false;
            });

            /* Check for close. */
            if(fDestruct.load() || config::fShutdown.load())
                return;

            /* Check if we are suspended. */
            if(config::fSuspendProtocol.load())
            {
                runtime::sleep(100);
                continue;
            }

            /* Pair to store the relay from the queue. */
            std::pair<typename ProtocolType::message_t, DataStream> qRelay =
                std::make_pair(typename ProtocolType::message_t(), DataStream(SER_NETWORK, MIN_PROTO_VERSION));

            /* Grab data from queue. */
            if(!RELAY->empty())
            {
                /* Make a copy of the relay data. */
                qRelay = RELAY->front();
                RELAY->pop();
            }

            /* Check all connections for data and packets. */
            uint32_t nSize = CONNECTIONS->size();
            for(uint32_t nIndex = 0; nIndex < nSize; ++nIndex)
            {
                /* Break early if shutdown signaled mid-iteration. */
                if(fDestruct.load() || config::fShutdown.load())
                    break;

                try
                {
                    /* Reset stream read position. */
                    qRelay.second.Reset();

                    /* Get shared pointer to prevent race condition on the internal connection pointer. */
                    std::shared_ptr<ProtocolType> CONNECTION = CONNECTIONS->at(nIndex);

                    /* Skip over Inactive Connections. */
                    if(!CONNECTION || !CONNECTION->Connected())
                        continue;

                    /* Relay if there are active subscriptions. */
                    const DataStream ssRelay = CONNECTION->RelayFilter(qRelay.first, qRelay.second);
                    if(ssRelay.size() != 0)
                    {
                        /* Build the sender packet. */
                        typename ProtocolType::packet_t PACKET = typename ProtocolType::packet_t(qRelay.first);
                        PACKET.SetData(ssRelay);

                        /* Write packet to socket. */
                        CONNECTION->WritePacket(PACKET);
                    }

                    /* Attempt to flush data when buffer is available. */
                    if(CONNECTION->Buffered() && CONNECTION->Flush() < 0)
                        runtime::sleep(std::min(5u, CONNECTION->nConsecutiveErrors.load() / 1000)); //we want to sleep when we have periodic failures
                }
                catch(const std::exception& e) { }
            }
        }
    }


    /* Tell the data thread an event has occured and notify each connection. */
    template<class ProtocolType>
    void DataThread<ProtocolType>::NotifyEvent()
    {
        /* Loop through each connection. */
        typename std::vector<std::shared_ptr<ProtocolType>>::iterator ITT = CONNECTIONS->begin();

        while(ITT != CONNECTIONS->end())
        {
            /* Access the shared pointer of the connection */
            std::shared_ptr<ProtocolType> CONNECTION = *ITT;

            if(CONNECTION)
            {
                try { CONNECTION->NotifyEvent(); }
                catch(const std::exception& e) { }
            }
        }
    }


    /* Disconnects given connection from current Data Thread. */
    template<class ProtocolType>
    void DataThread<ProtocolType>::Disconnect(const uint32_t nIndex)
    {
        /* Make sure connection is active. */
        if(CONNECTIONS->at(nIndex))
        {
            /* First disconnect our sockets. */
            CONNECTIONS->at(nIndex)->Disconnect();

            /* Now remove it from our connections vector. */
            remove_connection_with_event(nIndex, DISCONNECT::FORCE);
        }
    }


    /* Get the number of active connection pointers from data threads. */
    template <class ProtocolType>
    uint32_t DataThread<ProtocolType>::GetConnectionCount(const uint8_t nFlags)
    {
        /* Check for incoming connections. */
        uint32_t nConnections = 0;
        if(nFlags & FLAGS::INCOMING)
            nConnections += nIncoming.load();

        /* Check for outgoing connections. */
        if(nFlags & FLAGS::OUTGOING)
            nConnections += nOutbound.load();

        return nConnections;
    }


    /* Fires off a Disconnect event with the given disconnect reason and also removes the data thread connection. */
    template <class ProtocolType>
    void DataThread<ProtocolType>::remove_connection_with_event(const uint32_t nIndex, const uint8_t nReason)
    {
        /* Check that we have an active connection here. */
        if(!CONNECTIONS->at(nIndex))
            return;

        /* For authenticated mining connections (IsTimeoutExempt == true), log
         * the disconnect at verbosity 0 with full diagnostic context so that
         * operators can diagnose silent connection drops in production.
         * Non-mining / unauthenticated connections still rely on the protocol-
         * level Event(EVENTS::DISCONNECT) handler for logging. */
        if(CONNECTIONS->at(nIndex)->IsTimeoutExempt())
        {
            /* Human-readable disconnect reason. */
            const char* pReason = "UNKNOWN";
            switch(nReason)
            {
                case DISCONNECT::TIMEOUT:       pReason = "TIMEOUT (read idle)";       break;
                case DISCONNECT::ERRORS:        pReason = "ERRORS (socket I/O)";       break;
                case DISCONNECT::POLL_ERROR:    pReason = "POLL_ERROR";                break;
                case DISCONNECT::POLL_EMPTY:    pReason = "POLL_EMPTY (spurious POLLIN)"; break;
                case DISCONNECT::DDOS:          pReason = "DDOS (rate limit)";         break;
                case DISCONNECT::FORCE:         pReason = "FORCE (server/protocol)";   break;
                case DISCONNECT::PEER:          pReason = "PEER (remote closed)";      break;
                case DISCONNECT::BUFFER:        pReason = "BUFFER (send overflow)";    break;
                case DISCONNECT::TIMEOUT_WRITE: pReason = "TIMEOUT_WRITE (write stall)"; break;
                case DISCONNECT::PARTIAL_STALL: pReason = "PARTIAL_STALL (incomplete frame)"; break;
            }

            debug::log(0, FUNCTION, "DataThread[", ID, "]: Removing AUTHENTICATED mining connection ",
                       CONNECTIONS->at(nIndex)->GetAddress().ToStringIP(),
                       " reason=", pReason,
                       " buffered=", CONNECTIONS->at(nIndex)->Buffered());
        }

        /* Fire off our disconnect event now. */
        CONNECTIONS->at(nIndex)->Event(EVENTS::DISCONNECT, nReason);
        remove_connection(nIndex);
    }


    /* Removes given connection from current Data Thread. This happens on timeout/error, graceful close, or disconnect command. */
    template <class ProtocolType>
    void DataThread<ProtocolType>::remove_connection(const uint32_t nIndex)
    {
        /* Check that we have an active connection here. */
        if(!CONNECTIONS->at(nIndex))
            return;

        /* Adjust our internal counters for incoming/outbound. */
        if(CONNECTIONS->at(nIndex)->Incoming())
        {
            --nIncoming;

            /* Handle Meters and DDOS. */
            if(fMETER)
                ++ProtocolType::DISCONNECTS;
        }
        else
            --nOutbound;

        /* Free the memory and notify threads. */
        CONNECTIONS->at(nIndex) = nullptr;
        CONDITION.notify_all();
    }


    /* Returns the index of a component of the CONNECTIONS vector that has been flagged Disconnected */
    template <class ProtocolType>
    uint32_t DataThread<ProtocolType>::find_slot()
    {
        /* Loop through each connection. */
        const uint32_t nSize = static_cast<uint32_t>(CONNECTIONS->size());
        for(uint32_t nIndex = 0; nIndex < nSize; ++nIndex)
        {
            /* Find an available connection. */
            if(!CONNECTIONS->at(nIndex))
                return nIndex;
        }

        return nSize;
    }


    /* Explicity instantiate all template instances needed for compiler. */
    template class DataThread<TritiumNode>;
    template class DataThread<LookupNode>;
    template class DataThread<TimeNode>;
    template class DataThread<APINode>;
    template class DataThread<FileNode>;
    template class DataThread<RPCNode>;
    template class DataThread<Miner>;
    template class DataThread<StatelessMinerConnection>;
}
