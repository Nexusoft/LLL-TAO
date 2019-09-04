/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/


#include <LLC/include/random.h>

#include <LLD/include/global.h>

#include <Legacy/types/transaction.h>
#include <Legacy/wallet/wallet.h>

#include <LLP/include/hosts.h>
#include <LLP/include/global.h>
#include <LLP/include/inv.h>
#include <LLP/types/legacy.h>
#include <LLP/templates/events.h>
#include <LLP/include/manager.h>

#include <LLD/cache/binary_key.h>

#include <Util/include/args.h>
#include <Util/include/hex.h>
#include <Util/include/debug.h>
#include <Util/include/runtime.h>
#include <Util/include/version.h>

#include <TAO/Ledger/types/transaction.h>
#include <TAO/Ledger/types/mempool.h>
#include <TAO/Ledger/types/locator.h>

#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/include/process.h>

#include <iomanip>


namespace LLP
{

    /* Mutex to protect connected sessions. */
    std::mutex LegacyNode::SESSIONS_MUTEX;

    /* Map to keep track of duplicate nonce sessions. */
    std::map<uint64_t, std::pair<uint32_t, uint32_t>> LegacyNode::mapSessions;

    /* Helper function to switch the nodes on sync. */
    void SwitchNode()
    {
        /* Normal case of asking for a getblocks inventory message. */
        memory::atomic_ptr<LegacyNode>& pBest = LEGACY_SERVER->GetConnection(LegacyNode::addrFastSync.load());
        if(pBest != nullptr)
        {
            /* Send out another getblocks request. */
            try
            {
                pBest->PushGetBlocks(TAO::Ledger::ChainState::hashBestChain.load(), uint1024_t(0));
            }
            catch(const std::runtime_error& e)
            {
                /* Recurse on failure. */
                debug::error(FUNCTION, e.what());

                SwitchNode();
            }
        }
        else
        {
            /* Reset the sync address. */
            LegacyNode::addrFastSync = BaseAddress();

            /* Reset the getblocks request timeout. */
            LegacyNode::nLastGetBlocks.store(0);

            /* Logging to verify (for debugging). */
            debug::log(0, FUNCTION, "No Sync Nodes Available, Resetting State");
        }
    }


    /* Static initialization of last get blocks. */
    memory::atomic<uint1024_t> LegacyNode::hashLastGetblocks;


    /* The time since last getblocks. */
    std::atomic<uint64_t> LegacyNode::nLastGetBlocks;


    /* The fast sync average speed. */
    std::atomic<uint64_t> LegacyNode::nFastSyncAverage;


    /* The current node that is being used for fast sync */
    memory::atomic<BaseAddress> LegacyNode::addrFastSync;


    /* The last time a block was received. */
    std::atomic<uint64_t> LegacyNode::nLastTimeReceived;


    /* Timer for sync metrics. */
    static uint64_t nTimer = runtime::timestamp(true);


    /* The local relay inventory cache. */
    static LLD::KeyLRU cacheInventory = LLD::KeyLRU(1024 * 1024);


    /** Default Constructor **/
    LegacyNode::LegacyNode()
    : BaseConnection<LegacyPacket>()
    , strNodeVersion()
    , nCurrentVersion(LLP::PROTOCOL_VERSION)
    , nCurrentSession(0)
    , nStartingHeight(0)
    , nConsecutiveFails(0)
    , nConsecutiveOrphans(0)
    , fInbound(false)
    , nLastPing(runtime::timestamp())
    , hashContinue(0)
    , mapLatencyTracker()
    , mapSentRequests()
    {
    }


    /** Constructor **/
    LegacyNode::LegacyNode(Socket SOCKET_IN, DDOS_Filter* DDOS_IN, bool isDDOS)
    : BaseConnection<LegacyPacket>(SOCKET_IN, DDOS_IN, isDDOS)
    , strNodeVersion()
    , nCurrentVersion(LLP::PROTOCOL_VERSION)
    , nCurrentSession(0)
    , nStartingHeight(0)
    , nConsecutiveFails(0)
    , nConsecutiveOrphans(0)
    , fInbound(false)
    , nLastPing(runtime::timestamp())
    , hashContinue(0)
    , mapLatencyTracker()
    , mapSentRequests()
    {
    }


    /** Constructor **/
    LegacyNode::LegacyNode(DDOS_Filter* DDOS_IN, bool isDDOS)
    : BaseConnection<LegacyPacket>(DDOS_IN, isDDOS)
    , strNodeVersion()
    , nCurrentVersion(LLP::PROTOCOL_VERSION)
    , nCurrentSession(0)
    , nStartingHeight(0)
    , nConsecutiveFails(0)
    , nConsecutiveOrphans(0)
    , fInbound(false)
    , nLastPing(runtime::timestamp())
    , hashContinue(0)
    , mapLatencyTracker()
    , mapSentRequests()
    {
    }


    /* Virtual destructor. */
    LegacyNode::~LegacyNode()
    {
    }


    /* Push a Message With Information about This Current Node. */
    void LegacyNode::PushVersion()
    {
        /* Current Unified timestamp. */
        int64_t nTime = runtime::unifiedtimestamp();

        /* Dummy Variable not supported in Tritium */
        uint64_t nLocalServices = 0;

        /* Relay Your Address. */
        LegacyAddress addrMe;
        LegacyAddress addrYou = GetAddress();

        /* Push the Message to receiving node. */
        PushMessage("version", LLP::PROTOCOL_VERSION, nLocalServices, nTime, addrYou, addrMe,
                    SESSION_ID, version::CLIENT_VERSION_BUILD_STRING, TAO::Ledger::ChainState::nBestHeight.load());
    }


    /** Handle Event Inheritance. **/
    void LegacyNode::Event(uint8_t EVENT, uint32_t LENGTH)
    {
        /** Handle any DDOS Packet Filters. **/
        if(EVENT == EVENT_HEADER)
        {
            const std::string message = INCOMING.GetMessage();
            uint32_t nLenght = INCOMING.LENGTH;

            debug::log(3, NODE, "Received Message (", message, ", ", nLenght, ")");

            if(fDDOS)
            {

                /* Give higher DDOS score if the Node happens to try to send multiple version messages. */
                if(message == "version" && nCurrentVersion != 0)
                    if(DDOS)
                        DDOS->rSCORE += 25;

                /* Check the Packet Sizes to Unified Time Commands. */
                if((message == "getoffset" || message == "offset") && nLenght != 16)
                    if(DDOS)
                        DDOS->Ban(debug::safe_printstr("INVALID PACKET SIZE | OFFSET/GETOFFSET | LENGTH ", nLenght));
            }

            return;
        }


        /** Handle for a Packet Data Read. **/
        if(EVENT == EVENT_PACKET)
        {

            /* Check a packet's validity once it is finished being read. */
            if(fDDOS)
            {

                /* Give higher score for Bad Packets. */
                if(INCOMING.Complete() && !INCOMING.IsValid())
                {

                    debug::log(3, NODE, "Dropped Packet (Complete: ", INCOMING.Complete() ? "Y" : "N",
                        " - Valid:)",  INCOMING.IsValid() ? "Y" : "N");

                    if(DDOS)
                        DDOS->rSCORE += 15;
                }

            }

            if(INCOMING.Complete())
            {
                debug::log(4, NODE, "Received Packet (", INCOMING.LENGTH, ", ", INCOMING.GetBytes().size(), ")");

                if(config::GetArg("-verbose", 0) >= 5)
                    PrintHex(INCOMING.GetBytes());
            }

            return;
        }


        /* Handle Node Pings on Generic Events */
        if(EVENT == EVENT_GENERIC)
        {
            /* Handle sending the pings to remote node.. */
            if(nLastPing + 15 < runtime::unifiedtimestamp())
            {
                uint64_t nNonce = LLC::GetRand();
                nLastPing = runtime::unifiedtimestamp();


                mapLatencyTracker.insert(std::pair<uint64_t, runtime::timer>(nNonce, runtime::timer()));
                mapLatencyTracker[nNonce].Start();


                PushMessage("ping", nNonce);

                /* Rebroadcast transactions. */
                if(!TAO::Ledger::ChainState::Synchronizing())
                    Legacy::Wallet::GetInstance().ResendWalletTransactions();
            }


            /* Precompute the timestamp. */
            uint64_t nTimestamp = runtime::timestamp();

            /* Unreliabilitiy re-requesting (max time since getblocks) */
            if(TAO::Ledger::ChainState::Synchronizing()
            && addrFastSync == GetAddress()
            && nLastTimeReceived.load() + 15 < nTimestamp
            && nLastGetBlocks.load() + 15 < nTimestamp)
            {
                debug::log(0, NODE, "Sync Node Timeout");

                /* Switch to a new node. */
                SwitchNode();

                /* Reset the event timeouts. */
                nLastTimeReceived = nTimestamp;
                nLastGetBlocks    = nTimestamp;
            }

            //TODO: mapRequests data, if no response given retry the request at given times
        }


        /** On Connect Event, Assign the Proper Handle. **/
        if(EVENT == EVENT_CONNECT)
        {
            nLastPing    = runtime::unifiedtimestamp();

            debug::log(1, NODE, fOUTGOING ? "Outgoing" : "Incoming",
                       " Connected at timestamp ",   runtime::unifiedtimestamp());

            if(fOUTGOING)
                PushVersion();

            return;
        }


        /* Handle the Socket Disconnect */
        if(EVENT == EVENT_DISCONNECT)
        {
            std::string strReason = "";
            switch(LENGTH)
            {
                case DISCONNECT_TIMEOUT:
                    strReason = "Timeout";
                    break;

                case DISCONNECT_PEER:
                    strReason = "Peer disconnected";
                    break;

                case DISCONNECT_ERRORS:
                    strReason = "Errors";
                    break;

                case DISCONNECT_POLL_ERROR:
                    strReason = "Poll Error";
                    break;

                case DISCONNECT_POLL_EMPTY:
                    strReason = "Unavailable";
                    break;

                case DISCONNECT_DDOS:
                    strReason = "DDOS";
                    break;

                case DISCONNECT_FORCE:
                    strReason = "Forced";
                    break;

                default:
                    strReason = "Other";
                    break;
            }

            /* Detect if the fast sync node was disconnected. */
            if(addrFastSync == GetAddress())
            {
                debug::log(0, NODE, "Sync Node Disconnected");

                /* Switch the node. */
                SwitchNode();
            }

            {
                LOCK(SESSIONS_MUTEX);

                /* Check for sessions to free. */
                if(mapSessions.count(nCurrentSession))
                {
                    /* Make sure that we aren't freeing our already connected session if handling duplicate connections. */
                    const std::pair<uint32_t, uint32_t>& pair = mapSessions[nCurrentSession];
                    if(pair.first == nDataThread && pair.second == nDataIndex)
                        mapSessions.erase(nCurrentSession);
                }
            }

            /* Update address manager that this connection was dropped. */
            if(LEGACY_SERVER->pAddressManager)
                LEGACY_SERVER->pAddressManager->AddAddress(GetAddress(), ConnectState::DROPPED);

            /* Debug output for node disconnect. */
            debug::log(1, NODE, fOUTGOING ? "Outgoing" : "Incoming",
                " Disconnected (", strReason, ") at timestamp ", runtime::unifiedtimestamp());

            return;
        }

    }

    /** This function is necessary for a template LLP server. It handles your
        custom messaging system, and how to interpret it from raw packets. **/
    bool LegacyNode::ProcessPacket()
    {

        DataStream ssMessage(INCOMING.DATA, SER_NETWORK, MIN_PROTO_VERSION);

        const std::string message = INCOMING.GetMessage();

        /* Message Version is the first message received.
        * It gives you basic stats about the node to know how to
        * communicate with it.
        */
        if(message == "version")
        {
            int64_t nTime;
            LegacyAddress addrMe;
            LegacyAddress addrFrom;
            uint64_t nServices = 0;

            /* Check the Protocol Versions */
            ssMessage >> nCurrentVersion;

            /* Deserialize the rest of the data. */
            ssMessage >> nServices >> nTime >> addrMe >> addrFrom >> nCurrentSession >> strNodeVersion >> nStartingHeight;
            debug::log(1, NODE, "version message: version ", nCurrentVersion, ", blocks=",  nStartingHeight);

            /* Check for a connect to self. */
            if(nCurrentSession == SESSION_ID)
            {
                debug::log(0, FUNCTION, "connected to self");

                /* Cache self-address in the banned list of the Address Manager. */
                if(LEGACY_SERVER->pAddressManager)
                    LEGACY_SERVER->pAddressManager->Ban(addrMe);

                return false;
            }


            /* Check for duplicate connections. */
            if(TritiumNode::SessionActive(nCurrentSession))
                return debug::drop(NODE, "already have tritium connection");

            /* Check legacy connected sessions. */
            {
                LOCK(SESSIONS_MUTEX);
                if(mapSessions.count(nCurrentSession))
                {
                    debug::log(0, FUNCTION, "duplicate connection");

                    return false;
                }

                /* Claim this connection's session ID. */
                mapSessions[nCurrentSession] = std::make_pair(nDataThread, nDataIndex);
            }


            /* Update the block height in the Address Manager. */
            if(LEGACY_SERVER->pAddressManager)
                LEGACY_SERVER->pAddressManager->SetHeight(nStartingHeight, addrFrom);

            /* Push version in response. */
            if(!fOUTGOING)
                PushVersion();

            /* Send the Version Response to ensure communication is open. */
            PushMessage("verack");

            /* Push our version back since we just completed getting the version from the other node. */
            if(fOUTGOING && !addrFastSync.load().IsValid())
                PushGetBlocks(TAO::Ledger::ChainState::hashBestChain.load(), uint1024_t(0));

            PushMessage("getaddr");
        }
        else if(nCurrentVersion == 0)
        {
            return false;
        }

        /* Push a transaction into the Node's Received Transaction Queue. */
        else if(message == "tx")
        {
            /* Deserialize the Transaction. */
            Legacy::Transaction tx;
            ssMessage >> tx;

            /* Check for sync. */
            if(TAO::Ledger::ChainState::Synchronizing())
                return true;

            /* Accept to memory pool. */
            TAO::Ledger::BlockState notUsed;
            if(TAO::Ledger::mempool.Accept(tx))
            {
                Legacy::Wallet::GetInstance().AddToWalletIfInvolvingMe(tx, notUsed, true);

                std::vector<CInv> vInv = { CInv(tx.GetHash(), MSG_TX_LEGACY) };
                LEGACY_SERVER->Relay("inv", vInv);
            }
            else
            {
                /* Give this item a time penalty in the relay cache to make it ignored from here forward. */
                cacheInventory.Ban(tx.GetHash());
            }
        }


        /* Push a block into the Node's Received Blocks Queue. */
        else if(message == "block")
        {
            /* Deserialize the block. */
            Legacy::LegacyBlock block;
            ssMessage >> block;

            /* Process the block. */
            uint8_t nStatus = 0;
            TAO::Ledger::Process(block, nStatus);

            /* Check for specific status messages. */
            if(nStatus & TAO::Ledger::PROCESS::ACCEPTED)
            {
                /* Reset the fails and orphans. */
                nConsecutiveFails   = 0;
                nConsecutiveOrphans = 0;
            }

            /* Check for failure status messages. */
            if(nStatus & TAO::Ledger::PROCESS::REJECTED)
                ++nConsecutiveFails;

            /* Check for orphan status messages. */
            if(nStatus & TAO::Ledger::PROCESS::ORPHAN)
            {
                ++nConsecutiveOrphans;

                /* Check for duplicate. */
                if(!(nStatus & TAO::Ledger::PROCESS::DUPLICATE)
                && !(nStatus & TAO::Ledger::PROCESS::IGNORED))
                {
                    /* Inventory requests. */
                    std::vector<CInv> vInv = { CInv(block.hashPrevBlock, LLP::MSG_BLOCK_LEGACY) };

                    /* Get batch of inventory. */
                    PushMessage("getdata", vInv);

                    /* Run a getblocks to be sure. */
                    PushGetBlocks(TAO::Ledger::ChainState::hashBestChain.load(), uint1024_t(0));
                }
            }

            /* Check for failure limit on node. */
            if(nConsecutiveFails >= 500)
            {
                /* Fast Sync node switch. */
                if(TAO::Ledger::ChainState::Synchronizing())
                {
                    /* Find a new fast sync node if too many failures. */
                    if(addrFastSync == GetAddress())
                    {
                        /* Switch to a new node. */
                        SwitchNode();
                    }
                }

                /* Drop pesky nodes. */
                return false;
            }


            /* Detect large orphan chains and ask for new blocks from origin again. */
            if(nConsecutiveOrphans >= 500)
            {
                LOCK(TAO::Ledger::PROCESSING_MUTEX);

                debug::log(0, FUNCTION, "node reached orphan limit... closing");

                /* Clear the memory to prevent DoS attacks. */
                TAO::Ledger::mapOrphans.clear();

                /* Disconnect from a node with large orphan chain. */
                return false;
            }

            return true;
        }


        /* Send a Ping with a nNonce to get Latency Calculations. */
        else if(message == "ping")
        {
            uint64_t nonce = 0;
            ssMessage >> nonce;

            PushMessage("pong", nonce);
        }


        else if(message == "pong")
        {
            uint64_t nonce = 0;
            ssMessage >> nonce;

            /* If the nonce was not received or known from pong. */
            if(!mapLatencyTracker.count(nonce))
            {
                if(DDOS)
                    DDOS->rSCORE += 5;

                return true;
            }

            /* Calculate the Average Latency of the Connection. */
            nLatency = mapLatencyTracker[nonce].ElapsedMilliseconds();
            mapLatencyTracker.erase(nonce);

            /* Set the latency used for address manager within server */
            if(LEGACY_SERVER && LEGACY_SERVER->pAddressManager)
                LEGACY_SERVER->pAddressManager->SetLatency(nLatency, GetAddress());

            /* Debug Level 3: output Node Latencies. */
            debug::log(3, NODE, "Latency (Nonce ", std::hex, nonce, " - ", std::dec, nLatency, " ms)");
        }


        /* Handle a new Address Message.
        * This allows the exchanging of addresses on the network.
        */
        else if(message == "addr")
        {
            std::vector<LegacyAddress> vLegacyAddr;
            std::vector<BaseAddress> vAddr;

            ssMessage >> vLegacyAddr;

            /* Don't want addr from older versions unless seeding */
            if(vLegacyAddr.size() > 2000)
            {
                if(DDOS)
                    DDOS->rSCORE += 20;

                return debug::error(NODE, "message addr size() = ", vLegacyAddr.size(), "... Dropping Connection");
            }

            if(LEGACY_SERVER)
            {

                /* Try to establish the connection on the port the server is listening to. */
                for(const auto& legacyAddr : vLegacyAddr)
                {
                    BaseAddress addr;
                    addr.SetPort(LEGACY_SERVER->GetPort());
                    addr.SetIP(legacyAddr);

                    /* Create a base address vector from legacy addresses */
                    vAddr.push_back(addr);
                }

                /* Add new addresses to Legacy Server. */
                if(LEGACY_SERVER->pAddressManager)
                    LEGACY_SERVER->pAddressManager->AddAddresses(vAddr);

            }

        }


        /* Handle new Inventory Messages.
        * This is used to know what other nodes have in their inventory to compare to our own.
        */
        else if(message == "inv")
        {
            std::vector<CInv> vInv;
            ssMessage >> vInv;

            /* Get the inventory size. */
            uint32_t nInvSize = static_cast<uint32_t>(vInv.size());

            debug::log(3, NODE, "Inventory Message of ", nInvSize, " elements");

            /* Make sure the inventory size is not too large. */
            if(nInvSize > 10000)
            {
                if(DDOS)
                    DDOS->rSCORE += 20;

                return true;
            }

            /* Fast sync mode. */
            if(addrFastSync == GetAddress()
            && TAO::Ledger::ChainState::Synchronizing()
            && vInv.back().GetType() == MSG_BLOCK_LEGACY
            && vInv.size() > 100) //an assumption that a getblocks batch will be at least 100 blocks or more.
            {
                /* Normal case of asking for a getblocks inventory message. */
                PushGetBlocks(vInv.back().GetHash(), uint1024_t(0));
            }

            /* Make a copy of the data to request that is not in inventory. */
            if(!TAO::Ledger::ChainState::Synchronizing())
            {
                /* Filter duplicates if not synchronizing. */
                std::vector<CInv> vGet;

                /* Precompute values for inventory. */
                uint1024_t hashBlock;
                uint512_t hashTx;

                /* Reverse iterate for blocks in inventory. */
                for(auto inv = vInv.rbegin(); inv != vInv.rend(); ++inv)
                {
                    /* Get the inventory hash. */
                    hashBlock = inv->GetHash();
                    hashTx    = hashBlock;

                    /* If this is a block type, only request if not in database. */
                    if(inv->GetType() == MSG_BLOCK_LEGACY)
                    {
                        /* Check the LLD for block. */
                        if(!cacheInventory.Has(hashBlock) && !LLD::Ledger->HasBlock(hashBlock))
                        {
                            /* Add this item to request queue. */
                            vGet.push_back(*inv);

                            /* Add this item to cached relay inventory (key only). */
                            cacheInventory.Add(hashBlock);
                        }
                    }

                    /* Check the memory pool for transactions being relayed. */
                    else if(!cacheInventory.Has(hashTx) && !TAO::Ledger::mempool.Has(hashTx))
                    {
                        /* Add this item to cached relay inventory (key only). */
                        cacheInventory.Add(hashTx);
                    }
                }

                /* Reverse inventory to get to receive data in proper order. */
                std::reverse(vGet.begin(), vGet.end());

                /* Ask your friendly neighborhood node for the data. */
                PushMessage("getdata", vGet);
            }
            else
                PushMessage("getdata", vInv);
        }


        /* Get the Data for either a transaction or a block. */
        else if(message == "getdata")
        {
            std::vector<CInv> vInv;
            ssMessage >> vInv;

            if(vInv.size() > 10000)
            {
                if(DDOS)
                    DDOS->rSCORE += 20;

                return true;
            }

            /* Precompute values for inventory. */
            uint1024_t hashBlock;
            uint512_t hashTx;
            int32_t nInvType;

            /* Loop the inventory and deliver messages. */
            for(const auto& inv : vInv)
            {
                /* Determine the inventory type. */
                nInvType = inv.GetType();

                /* Get the inventory hash. */
                hashBlock = inv.GetHash();

                /* Log the inventory message receive. */
                debug::log(3, FUNCTION, "received getdata ", inv.ToString());

                /* Handle the block message. */
                if(nInvType == LLP::MSG_BLOCK_LEGACY)
                {
                    /* Don't send genesis if asked for. */
                    if(hashBlock == TAO::Ledger::ChainState::Genesis())
                        continue;

                    /* Read the block from disk. */
                    TAO::Ledger::BlockState state;
                    if(!LLD::Ledger->ReadBlock(hashBlock, state))
                        continue;

                    /* Scan each transaction in the block and process those related to this wallet */
                    Legacy::LegacyBlock block(state);

                    /* Check that all transactions were included. */
                    if(block.vtx.size() != state.vtx.size())
                    {
                        std::vector<CInv> vInv = { CInv(TAO::Ledger::ChainState::hashBestChain.load(), LLP::MSG_BLOCK_LEGACY) };
                        PushMessage("inv", vInv);
                        hashContinue = 0;

                        return true;
                    }

                    /* Push the response message. */
                    PushMessage("block", block);

                    /* Trigger a new getblocks if hash continue is set. */
                    if (hashBlock == hashContinue)
                    {
                        std::vector<CInv> vInv = { CInv(TAO::Ledger::ChainState::hashBestChain.load(), LLP::MSG_BLOCK_LEGACY) };
                        PushMessage("inv", vInv);
                        hashContinue = 0;
                    }
                }
                else if(nInvType == LLP::MSG_TX_LEGACY)
                {
                    hashTx = hashBlock;

                    Legacy::Transaction tx;
                    if(!TAO::Ledger::mempool.Get(hashTx, tx) && !LLD::Legacy->ReadTx(hashTx, tx))
                        continue;

                    PushMessage("tx", tx);
                }
            }
        }


        /* Handle a Request to get a list of Blocks from a Node. */
        else if(message == "getblocks")
        {
            /* Get the locator. */
            TAO::Ledger::Locator locator;
            ssMessage >> locator;

            /* Get the stopping hash. */
            uint1024_t hashStop;
            ssMessage >> hashStop;

            /* Return if nothing in locator. */
            if(locator.vHave.size() == 0)
                return false;

            /* Get the block state from. */
            std::vector<TAO::Ledger::BlockState> vStates;
            for(const auto& have : locator.vHave)
            {
                /* Check the database for the ancestor block. */
                if(LLD::Ledger->BatchRead(have, "block", vStates, 1000))
                    break;
            }

            /* If no ancestor blocks were found. */
            if(vStates.size() == 0)
                return true;

            /* Set the search from search limit. */
            debug::log(2, "getblocks ", vStates[0].nHeight, " to ", hashStop.ToString().substr(0, 20));

            /* Iterate forward the blocks required. */
            std::vector<CInv> vInv;

            /* Loop through found states. */
            for(const auto& state : vStates)
            {
                /* Check if in main chain. */
                if(!state.IsInMainChain())
                    continue;

                /* Get the state hash. */
                hashContinue = state.GetHash();

                /* Check for hash stop. */
                if(hashContinue == hashStop)
                {
                    debug::log(3, "  getblocks stopping at ", state.nHeight, " to ", hashContinue.ToString().substr(0, 20));

                    /* Tell about latest block if hash stop is found. */
                    if(hashStop != TAO::Ledger::ChainState::hashBestChain.load())
                        vInv.push_back(CInv(TAO::Ledger::ChainState::hashBestChain.load(), LLP::MSG_BLOCK_LEGACY));

                    break;
                }

                /* Push new item to inventory. */
                vInv.push_back(CInv(hashContinue, LLP::MSG_BLOCK_LEGACY));
            }

            /* Push the inventory. */
            if(vInv.size() > 0)
                PushMessage("inv", vInv);
        }


        /* Get a list of addresses from another node. */
        else if(message == "getaddr")
        {
            /* Get addresses from manager. */
            std::vector<BaseAddress> vAddr;
            if(LEGACY_SERVER->pAddressManager)
                LEGACY_SERVER->pAddressManager->GetAddresses(vAddr);

            /* Add the best 1000 (or less) addresses. */
            std::vector<LegacyAddress> vSend;
            const uint32_t nCount = std::min((uint32_t)vAddr.size(), 1000u);
            for(uint32_t n = 0; n < nCount; ++n)
                vSend.push_back(vAddr[n]);

            /* Send the addresses off. */
            if(nCount > 0)
                PushMessage("addr", vSend);
        }

        return true;
    }



    /*  Non-Blocking Packet reader to build a packet from TCP Connection.
     *  This keeps thread from spending too much time for each Connection. */
    void LegacyNode::ReadPacket()
    {
        if(!INCOMING.Complete())
        {
            /** Handle Reading Packet Length Header. **/
            if(INCOMING.IsNull() && Available() >= 24)
            {
                std::vector<uint8_t> BYTES(24, 0);
                if(Read(BYTES, 24) == 24)
                {
                    DataStream ssHeader(BYTES, SER_NETWORK, MIN_PROTO_VERSION);
                    ssHeader >> INCOMING;

                    Event(EVENT_HEADER);
                }
            }

            /** Handle Reading Packet Data. **/
            uint32_t nAvailable = Available();
            if(nAvailable > 0 && !INCOMING.IsNull() && INCOMING.DATA.size() < INCOMING.LENGTH)
            {

                /* Create the packet data object. */
                std::vector<uint8_t> DATA(std::min(nAvailable, (uint32_t)(INCOMING.LENGTH - INCOMING.DATA.size())), 0);

                /* Read up to 512 bytes of data. */
                if(Read(DATA, DATA.size()) == DATA.size())
                {
                    INCOMING.DATA.insert(INCOMING.DATA.end(), DATA.begin(), DATA.end());
                    Event(EVENT_PACKET, static_cast<uint32_t>(DATA.size()));
                }
            }
        }
    }


    /* Get a node by connected session. */
    memory::atomic_ptr<LegacyNode>& LegacyNode::GetNode(const uint64_t nSession)
    {
        LOCK(SESSIONS_MUTEX);

        /* Check for connected session. */
        static memory::atomic_ptr<LegacyNode> pNULL;
        if(!mapSessions.count(nSession))
            return pNULL;

        /* Get a reference of session. */
        const std::pair<uint32_t, uint32_t>& pair = mapSessions[nSession];
        return LEGACY_SERVER->GetConnection(pair.first, pair.second);
    }
}
