/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/include/random.h>

#include <LLD/include/global.h>

#include <LLP/types/tritium.h>
#include <LLP/include/global.h>
#include <LLP/templates/events.h>
#include <LLP/include/manager.h>
#include <LLP/include/inv.h>

#include <TAO/Operation/include/execute.h>

#include <TAO/Register/types/object.h>

#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/types/mempool.h>

#include <Legacy/wallet/wallet.h>

#include <Util/include/runtime.h>
#include <Util/include/args.h>
#include <Util/include/debug.h>


#include <climits>
#include <memory>
#include <iomanip>

namespace LLP
{
    std::atomic<uint32_t> TritiumNode::nAsked(0);


    /* Static instantiation of orphan blocks in queue to process. */
    std::map<uint1024_t, std::unique_ptr<TAO::Ledger::Block>> TritiumNode::mapOrphans;


    /* Mutex to protect checking more than one block at a time. */
    std::mutex TritiumNode::PROCESSING_MUTEX;


    /* Mutex to protect the legacy orphans map. */
    std::mutex TritiumNode::ORPHAN_MUTEX;


    /* Mutex to protect connected sessions. */
    std::mutex TritiumNode::SESSIONS_MUTEX;

    /* global map connections to session ID's to be used to prevent duplicate connections to the same
           sever, but via a different RLOC / EID */
    std::map<uint64_t, TritiumNode*> TritiumNode::mapConnectedSessions;


    /* Static initialization of last get blocks. */
    memory::atomic<uint1024_t> TritiumNode::hashLastGetblocks;


    /* The time since last getblocks. */
    std::atomic<uint64_t> TritiumNode::nLastGetBlocks;


    /* The fast sync average speed. */
    std::atomic<uint32_t> TritiumNode::nFastSyncAverage;


    /* The current node that is being used for fast sync */
    memory::atomic<BaseAddress> TritiumNode::addrFastSync;


    /* The last time a block was received. */
    std::atomic<uint64_t> TritiumNode::nLastTimeReceived;


    /* Timer for sync metrics. */
    static uint64_t nTimer = runtime::timestamp(true);


    /* The local relay inventory cache. */
    LLD::KeyLRU TritiumNode::cacheInventory = LLD::KeyLRU(1024 * 1024);


    /* The session identifier. */
    uint64_t TritiumNode::nSessionID = LLC::GetRand();


    /** Default Constructor **/
    TritiumNode::TritiumNode()
    : BaseConnection<TritiumPacket>()
    , nCurrentSession(0)
    , nStartingHeight(0)
    , nLastPing(0)
    , nLastSamples(0)
    , mapLatencyTracker()
    , mapSentRequests()
    , hashContinue(0)
    , nConsecutiveFails(0)
    , nConsecutiveOrphans(0)
    , fInbound(false)
    {
    }


    /** Constructor **/
    TritiumNode::TritiumNode(Socket SOCKET_IN, DDOS_Filter* DDOS_IN, bool isDDOS)
    : BaseConnection<TritiumPacket>(SOCKET_IN, DDOS_IN, isDDOS)
    , nCurrentSession(0)
    , nStartingHeight(0)
    , nLastPing(0)
    , nLastSamples(0)
    , mapLatencyTracker()
    , mapSentRequests()
    , hashContinue(0)
    , nConsecutiveFails(0)
    , nConsecutiveOrphans(0)
    , fInbound(false)
    {
    }


    /** Constructor **/
    TritiumNode::TritiumNode(DDOS_Filter* DDOS_IN, bool isDDOS)
    : BaseConnection<TritiumPacket>(DDOS_IN, isDDOS)
    , nCurrentSession(0)
    , nStartingHeight(0)
    , nLastPing(0)
    , nLastSamples(0)
    , mapLatencyTracker()
    , mapSentRequests()
    , hashContinue(0)
    , nConsecutiveFails(0)
    , nConsecutiveOrphans(0)
    , fInbound(false)
    {
    }


    /** Default Destructor **/
    TritiumNode::~TritiumNode()
    {
    }


    /* Helper function to switch the nodes on sync. */
    void TritiumNode::SwitchNode()
    {
        /* Normal case of asking for a getblocks inventory message. */
        memory::atomic_ptr<TritiumNode>& pBest = TRITIUM_SERVER->GetConnection(TritiumNode::addrFastSync.load());

        /* Null check the pointer. */
        if(pBest != nullptr)
        {
            try
            {
                /* Switch to a new node for fast sync. */
                pBest->PushGetInventory(TAO::Ledger::ChainState::hashBestChain.load(), uint1024_t(0));
            }
            catch(const std::runtime_error& e)
            {
                debug::error(FUNCTION, e.what());

                SwitchNode();
            }
        }
    }

    /** Virtual Functions to Determine Behavior of Message LLP. **/
    void TritiumNode::Event(uint8_t EVENT, uint32_t LENGTH)
    {
        switch(EVENT)
        {
            case EVENT_CONNECT:
            {
                /* Setup the variables for this node. */
                nLastPing    = runtime::unifiedtimestamp();

                debug::log(1, NODE, fOUTGOING ? "Outgoing" : "Incoming",
                       " Connected at timestamp ",   runtime::unifiedtimestamp());

                if(fOUTGOING)
                {
                    /* Send version if making the connection. */
                    PushMessage(DAT_VERSION, TritiumNode::nSessionID, GetAddress());
                }

                break;
            }

            case EVENT_HEADER:
            {
                break;
            }

            case EVENT_PACKET:
            {
                /* Check a packet's validity once it is finished being read. */
                if(fDDOS)
                {

                    /* Give higher score for Bad Packets. */
                    if(INCOMING.Complete() && !INCOMING.IsValid())
                    {

                        debug::log(3, NODE "Dropped Packet (Complete: ", INCOMING.Complete() ? "Y" : "N",
                            " - Valid:)",  INCOMING.IsValid() ? "Y" : "N");

                        if(DDOS)
                            DDOS->rSCORE += 15;
                    }

                }

                if(INCOMING.Complete())
                {
                    debug::log(4, NODE "Received Packet (", INCOMING.LENGTH, ", ", INCOMING.GetBytes().size(), ")");

                    if(config::GetArg("-verbose", 0) >= 5)
                        PrintHex(INCOMING.GetBytes());
                }
                break;
            }

            case EVENT_GENERIC:
            {
                /* Generic event - pings. */
                if(nLastPing + 15 < runtime::unifiedtimestamp())
                {
                    uint64_t nNonce = LLC::GetRand();
                    nLastPing = runtime::unifiedtimestamp();


                    mapLatencyTracker.insert(std::pair<uint64_t, runtime::timer>(nNonce, runtime::timer()));
                    mapLatencyTracker[nNonce].Start();


                    /* Push a ping request. */
                    PushMessage(DAT_PING, nNonce);

                }

                /* Generic events - unified time. */
                if(runtime::timestamp() - nLastSamples > 30)
                {
                    /* Generate the request identification. */
                    uint32_t nRequestID = LLC::GetRand(std::numeric_limits<uint32_t>::max());

                    /* Add sent requests. */
                    mapSentRequests[nRequestID] = runtime::timestamp();

                    /* Request time samples. */
                    PushMessage(GET_OFFSET, nRequestID, runtime::timestamp(true));

                    /* Update the samples timer. */
                    nLastSamples = runtime::timestamp();
                }

                /* Unreliabilitiy re-requesting (max time since getblocks) */
                if(config::GetBoolArg("-fastsync", true)
                && TAO::Ledger::ChainState::Synchronizing()
                && addrFastSync == GetAddress()
                && nLastTimeReceived.load() + 10 < runtime::timestamp()
                && nLastGetBlocks.load() + 10 < runtime::timestamp())
                {
                    debug::log(0, NODE "fast sync event timeout");

                    /* Switch to a new node. */
                    SwitchNode();

                    /* Reset the event timeouts. */
                    nLastTimeReceived = runtime::timestamp();
                    nLastGetBlocks    = runtime::timestamp();
                }

                break;
            }

            case EVENT_DISCONNECT:
            {
                /* Debut output. */
                uint8_t reason = LENGTH;
                std::string strReason;

                switch(reason)
                {
                    case DISCONNECT_TIMEOUT:
                        strReason = "DISCONNECT_TIMEOUT";
                        break;
                    case DISCONNECT_ERRORS:
                        strReason = "DISCONNECT_ERRORS";
                        break;
                    case DISCONNECT_DDOS:
                        strReason = "DISCONNECT_DDOS";
                        break;
                    case DISCONNECT_FORCE:
                        strReason = "DISCONNECT_FORCE";
                        break;
                    default:
                        strReason = "UNKNOWN";
                        break;
                }

                /* Detect if the fast sync node was disconnected. */
                if(addrFastSync == GetAddress())
                {
                    /* Switch the node. */
                    SwitchNode();
                }

                /* Debug output for node disconnect. */
                debug::log(1, NODE, fOUTGOING ? "Outgoing" : "Incoming",
                    " Disconnected (", strReason, ") at timestamp ", runtime::unifiedtimestamp());

                if(TRITIUM_SERVER && TRITIUM_SERVER->pAddressManager)
                    TRITIUM_SERVER->pAddressManager->AddAddress(GetAddress(), ConnectState::DROPPED);

                /* Finally remove this connection from the global map by session*/
                {
                    LOCK(SESSIONS_MUTEX);

                    /* Free this session, if it is this connection that we mapped.
                       When we disconnect a duplicate session then it will not have been added to the map,
                       so we need to skip removing the session ID*/
                    if(TritiumNode::mapConnectedSessions.count(nCurrentSession)
                    && TritiumNode::mapConnectedSessions[nCurrentSession] == this)
                        TritiumNode::mapConnectedSessions.erase(nCurrentSession);
                }

                break;
            }
        }
    }


    /** Main message handler once a packet is recieved. **/
    bool TritiumNode::ProcessPacket()
    {

        DataStream ssPacket(INCOMING.DATA, SER_NETWORK, PROTOCOL_VERSION);
        switch(INCOMING.MESSAGE)
        {
            case DAT_DUPE_DISCONNECT:
            {
                /* The disconnect message is sent by peers when they want to gracefully close a connection
                   when they have detected more than one connection to the same node.
                   We send the same message in reply and then return false from ProcessPacket which will
                   in turn issue a FORCE_DISCONNECT event on this connection to gracefully close the
                   connection at our end.  By responding with a DAT_DUPE_DISCONNECT message the same will occur
                   at the calling end to ensure both ends close gracefully. */
                   debug::log(0, NODE "Disconnecting duplicate connection.");
                   PushMessage(DAT_DUPE_DISCONNECT);
                return false;
                break;
            }


            case DAT_VERSION:
            {
                /* Deserialize the session identifier. */
                ssPacket >> nCurrentSession;

                /* Get your address. */
                BaseAddress addr;
                ssPacket >> addr;

                /* Check for a connect to self. */
                {
                    LOCK(SESSIONS_MUTEX);
                    if(nCurrentSession == TritiumNode::nSessionID)
                    {
                        debug::log(0, NODE "connected to self");

                        /* Cache self-address in the banned list of the Address Manager. */
                        if(TRITIUM_SERVER && TRITIUM_SERVER->pAddressManager)
                            TRITIUM_SERVER->pAddressManager->Ban(addr);

                        return false;
                    }
                    /* Check for existing connection to same node*/
                    else if(TritiumNode::mapConnectedSessions.count(nCurrentSession) > 0)
                    {
                        TritiumNode* pConnection = TritiumNode::mapConnectedSessions.at(nCurrentSession);

                        /* If the existing connection is via LISP then we make a preference for it and disallow the
                        incoming connection. Otherwise if the incoming is via LISP and the existing is not we
                        disconnect the the existing connection in favour of the LISP route */
                        if(!GetAddress().IsEID() || pConnection->GetAddress().IsEID())
                        {
                            /* don't allow new connection */
                            debug::log(0, NODE "duplicate connection attempt to same server prevented (session ID ", nCurrentSession, ").  Existing: ", pConnection->GetAddress().ToStringIP(), " New: ", GetAddress().ToStringIP());

                            /* If we attempted to connect via a different IP to the existing connection then
                               notify the AddressManager to ban the second IP so that we favour the existing one */
                            if(GetAddress() != pConnection->GetAddress() && TRITIUM_SERVER && TRITIUM_SERVER->pAddressManager)
                                TRITIUM_SERVER->pAddressManager->Ban(addr);

                            return false;
                        }
                        else
                        {
                            /* initiate disconnect of existing connection in favour of new one */
                            debug::log(0, NODE "duplicate connection attempt to same server (session ID ", nCurrentSession, ").  Switching to EID connection.  Existing: ", pConnection->GetAddress().ToStringIP(), " New: ", GetAddress().ToStringIP());

                            /* Notify the peer to disconnect the existing connection */
                            pConnection->PushMessage(DAT_DUPE_DISCONNECT);

                            /* Notify the AddressManager to ban the underlay IP so that we favour the EID */
                            if(TRITIUM_SERVER && TRITIUM_SERVER->pAddressManager)
                                TRITIUM_SERVER->pAddressManager->Ban(pConnection->GetAddress());
                        }
                    }


                    /* Add this connection into the global map once we have verified the DAT_VERSION message and
                        are happy to allow the connection */
                    TritiumNode::mapConnectedSessions[nCurrentSession] = this;
                }

                /* Debug output for offsets. */
                debug::log(3, NODE "received session identifier ",nCurrentSession);


                /* Send version message if connection is inbound. */
                if(!fOUTGOING)
                    PushMessage(DAT_VERSION, TritiumNode::nSessionID, GetAddress());
                else
                    PushMessage(GET_ADDRESSES);

                /* Ask the new node for their inventory*/
                PushGetInventory(TAO::Ledger::ChainState::hashBestChain.load(), uint1024_t(0));

                // if(fOUTGOING && nAsked == 0)
                // {
                //     ++nAsked;
                //    PushGetInventory(TAO::Ledger::ChainState::hashBestChain.load(), uint1024_t(0));
                // }


                break;
            }


            case GET_OFFSET:
            {
                /* Deserialize request id. */
                uint32_t nRequestID;
                ssPacket >> nRequestID;

                /* Deserialize the timestamp. */
                uint64_t nTimestamp;
                ssPacket >> nTimestamp;

                /* Find the sample offset. */
                int32_t nOffset = (runtime::timestamp(true) - nTimestamp);

                /* Debug output for offsets. */
                debug::log(3, NODE "received timestamp of ", nTimestamp, " sending offset ", nOffset);

                /* Push a timestamp in response. */
                PushMessage(DAT_OFFSET, nRequestID, nOffset);

                break;
            }


            case DAT_OFFSET:
            {
                /* Deserialize the request id. */
                uint32_t nRequestID;
                ssPacket >> nRequestID;

                /* Check map known requests. */
                if(!mapSentRequests.count(nRequestID))
                {
                    if(DDOS)
                        DDOS->rSCORE += 5;

                    debug::log(3, NODE "Invalid Request : Message Not Requested [", nRequestID, "][", nLatency, " ms]");

                    break;
                }

                /* Check the time since request was sent. */
                if(runtime::timestamp() - mapSentRequests[nRequestID] > 10)
                {
                    debug::log(3, NODE "Invalid Request : Message Stale [", nRequestID, "][", nLatency, " ms]");

                    if(DDOS)
                        DDOS->rSCORE += 15;

                    mapSentRequests.erase(nRequestID);

                    break;
                }

                /* Deserialize the offset. */
                int32_t nOffset;
                ssPacket >> nOffset;

                /* Adjust the Offset for Latency. */
                nOffset -= nLatency;

                /* Debug output for offsets. */
                debug::log(3, NODE "Received Unified Offset ", nOffset, " [", nRequestID, "][", nLatency, " ms]");

                /* Remove sent requests from mpa. */
                mapSentRequests.erase(nRequestID);

                break;
            }


            case GET_INVENTORY:
            {
                TAO::Ledger::Locator locator;
                uint1024_t hashStop;
                ssPacket >> locator >> hashStop;

                /* Return if nothing in locator. */
                if(locator.vHave.size() == 0)
                    return false;

                /* Get the block state from. */
                TAO::Ledger::BlockState state;
                for(const auto& have : locator.vHave)
                {
                    /* Check the database for the ancestor block. */
                    if(LLD::Ledger->ReadBlock(have, state))
                        break;
                }

                /* If no ancestor blocks were found. */
                if(!state)
                    return debug::error(FUNCTION, "no ancestor blocks found");

                /* Set the search from search limit. */
                int32_t nLimit = 1000;
                debug::log(2, NODE "getblocks ", state.nHeight, " to ", hashStop.ToString().substr(0, 20), " limit ", nLimit);

                /* Iterate forward the blocks required. */
                std::vector<CInv> vInv;
                bool fIsLegacy = false;
                while(!config::fShutdown.load())
                {
                    /* Iterate to next state, if there is one */
                    state = state.Next();

                    if(!state)
                        break;

                    fIsLegacy = state.vtx[0].first == TAO::Ledger::TYPE::LEGACY_TX;

                    /* Check for hash stop. */
                    if(state.GetHash() == hashStop)
                    {
                        debug::log(3, NODE "getblocks stopping at ", state.nHeight, " to ", state.GetHash().ToString().substr(0, 20));

                        /* Tell about latest block if hash stop is found. */
                        if(hashStop != TAO::Ledger::ChainState::hashBestChain.load())
                        {
                            /* First add all of the transactions hashes from the block.
                               Start at index 1 so that we dont' include producer, as that is sent as part of the block */
                            for(int i=1; i < state.vtx.size(); i++)
                                vInv.push_back(CInv(state.vtx[i].second, state.vtx[i].first == TAO::Ledger::TYPE::LEGACY_TX ? MSG_TX_LEGACY : MSG_TX_TRITIUM));

                            /* lastly add the block hash */
                            vInv.push_back(CInv(TAO::Ledger::ChainState::hashBestChain.load(), fIsLegacy ? MSG_BLOCK_LEGACY : MSG_BLOCK_TRITIUM));
                        }


                        break;
                    }

                    /* Push new item to inventory. */
                    /* First add all of the transactions hashes from the block.
                        Start at index 1 so that we dont' include producer, as that is sent as part of the block */
                    for(int i=1; i < state.vtx.size(); i++)
                        vInv.push_back(CInv(state.vtx[i].second, state.vtx[i].first == TAO::Ledger::TYPE::LEGACY_TX ? MSG_TX_LEGACY : MSG_TX_TRITIUM));

                    /* lastly add the block hash */
                    vInv.push_back(CInv(state.GetHash(), fIsLegacy ? MSG_BLOCK_LEGACY : MSG_BLOCK_TRITIUM));

                    /* Stop at limits. */
                    if(--nLimit <= 0 || vInv.size() > 50000)
                    {
                        // When this block is requested, we'll send an inv that'll make them
                        // getblocks the next batch of inventory.
                        debug::log(3, NODE "getblocks stopping at limit ", state.nHeight, " to ", state.GetHash().ToString().substr(0,20));

                        hashContinue = state.GetHash();
                        break;
                    }
                }

                /* Push the inventory. */
                if(vInv.size() > 0)
                    PushMessage(DAT_INVENTORY, vInv);

                break;
            }

            /* Handle new Inventory Messages.
            * This is used to know what other nodes have in their inventory to compare to our own.
            */
            case DAT_INVENTORY:
            {
                std::vector<CInv> vInv;
                ssPacket >> vInv;

                debug::log(3, NODE "Inventory Message of ", vInv.size(), " elements");

                /* Make sure the inventory size is not too large. */
                if(vInv.size() > 100000)
                {
                    if(DDOS)
                        DDOS->rSCORE += 20;

                    return true;
                }

                /* Fast sync mode. */
                if(config::GetBoolArg("-fastsync", true)
                && addrFastSync == GetAddress()
                && TAO::Ledger::ChainState::Synchronizing()
                && (vInv.back().GetType() == MSG_BLOCK_LEGACY || vInv.back().GetType() == MSG_BLOCK_TRITIUM)
                && vInv.size() > 100) //an assumption that a getblocks batch will be at least 100 blocks or more.
                {
                    /* Normal case of asking for a getblocks inventory message. */
                    PushGetInventory(vInv.back().GetHash(), uint1024_t(0));
                }

                std::vector<CInv> vGet;

                /* Make a copy of the data to request that is not in inventory. */
                if(!TAO::Ledger::ChainState::Synchronizing())
                {
                    /* Filter duplicates if not synchronizing. */

                    /* Reverse iterate for blocks in inventory. */
                    std::reverse(vInv.begin(), vInv.end());
                    for(const auto& inv : vInv)
                    {
                        /* If this is a block type, only request if not in database. */
                        if(inv.GetType() == MSG_BLOCK_LEGACY || inv.GetType() == MSG_BLOCK_TRITIUM)
                        {
                            /* Check the LLD for block. */
                            if(!LLD::Ledger->HasBlock(inv.GetHash()))
                            {
                                /* Add this item to request queue. */
                                vGet.push_back(inv);

                                /* Add this item to cached relay inventory (key only). */
                                //cacheInventory.Add(inv.GetHash());
                            }
                            else
                                break; //break since iterating backwards (searching newest to oldest)
                                    //the assumtion here is that if you have newer inventory, you have older
                        }

                        /* Check the memory pool for transactions being relayed. */
                        else if(!TAO::Ledger::mempool.Has(uint512_t(inv.GetHash())))
                        {
                            /* Add this item to request queue. */
                            vGet.push_back(inv);

                            /* Add this item to cached relay inventory (key only). */
                            //cacheInventory.Add(uint512_t(inv.GetHash()));
                        }
                    }

                    /* Reverse inventory to get to receive data in proper order. */
                    std::reverse(vGet.begin(), vGet.end());

                }
                else
                    vGet = vInv; /* just request data for all inventory*/

                /* Ask your friendly neighborhood node for the data */
                if(vGet.size() > 0)
                    PushMessage(GET_DATA, vGet);

                break;
            }

            /* Get the Data for either a transaction or a block. */
            case GET_DATA:
            {
                std::vector<CInv> vInv;
                ssPacket >> vInv;

                if(vInv.size() > 100000)
                {
                    if(DDOS)
                        DDOS->rSCORE += 20;

                    return true;
                }

                debug::log(3, NODE "received getdata of ", vInv.size(), " elements");

                /* Loop the inventory and deliver messages. */
                for(const auto& inv : vInv)
                {

                    /* Log the inventory message receive. */
                    debug::log(3, NODE "processing getdata ", inv.ToString());

                    /* Handle the block message. */
                    if(inv.GetType() == MSG_BLOCK_LEGACY || inv.GetType() == MSG_BLOCK_TRITIUM)
                    {
                        /* Don't send genesis if asked for. */
                        if(inv.GetHash() == TAO::Ledger::ChainState::Genesis())
                            continue;

                        /* Read the block from disk. */
                        TAO::Ledger::BlockState state;
                        if(!LLD::Ledger->ReadBlock(inv.GetHash(), state))
                        {
                            debug::log(3, NODE "getdata readblock failed ", inv.GetHash().ToString().substr(0, 20));
                            continue;
                        }

                        /* Push the response message. */
                        if(inv.GetType() == MSG_BLOCK_TRITIUM)
                        {
                            TAO::Ledger::TritiumBlock block(state);
                            PushMessage(DAT_BLOCK, (uint8_t)MSG_BLOCK_TRITIUM, block);
                        }
                        else
                        {
                            Legacy::LegacyBlock block(state);
                            PushMessage(DAT_BLOCK, (uint8_t)MSG_BLOCK_LEGACY, block);
                        }


                        /* Trigger a new getblocks if hash continue is set. */
                        if(inv.GetHash() == hashContinue)
                        {
                            PushMessage(GET_INVENTORY, TAO::Ledger::Locator(TAO::Ledger::ChainState::hashBestChain.load()), uint1024_t(0));
                            hashContinue = 0;
                        }
                    }
                    else if(inv.GetType() == LLP::MSG_TX_TRITIUM)
                    {
                        TAO::Ledger::Transaction tx;
                        if(!TAO::Ledger::mempool.Get(uint512_t(inv.GetHash()), tx)
                        && !LLD::Ledger->ReadTx(uint512_t(inv.GetHash()), tx))
                            continue;

                        PushMessage(DAT_TRANSACTION, (uint8_t)LLP::MSG_TX_TRITIUM, tx);
                    }
                    else if(inv.GetType() == LLP::MSG_TX_LEGACY)
                    {
                        Legacy::Transaction tx;
                        if(!TAO::Ledger::mempool.Get((uint512_t)inv.GetHash(), tx)
                        && !LLD::Legacy->ReadTx((uint512_t)inv.GetHash(), tx))
                            continue;

                        PushMessage(DAT_TRANSACTION, (uint8_t)LLP::MSG_TX_LEGACY, tx);
                    }
                }

                break;
            }


            // case DAT_HAS_TX:
            // {
            //     /* Deserialize the data received. */
            //     std::vector<uint512_t> vData;
            //     ssPacket >> vData;

            //     /* Request the inventory. */
            //     for(const auto& hash : vData)
            //         PushMessage(GET_TRANSACTION, hash);

            //     break;
            // }


            // case GET_TRANSACTION:
            // {
            //     /* Deserialize the inventory. */
            //     uint512_t hash;
            //     ssPacket >> hash;

            //     /* Check if you have it. */
            //     TAO::Ledger::Transaction tx;
            //     if(LLD::Ledger->ReadTx(hash, tx) || TAO::Ledger::mempool.Get(hash, tx))
            //         PushMessage(DAT_TRANSACTION, tx);

            //     break;
            // }


            // case DAT_HAS_BLOCK:
            // {
            //     /* Deserialize the data received. */
            //     std::vector<uint1024_t> vData;
            //     ssPacket >> vData;

            //     /* Request the inventory. */
            //     for(const auto& hash : vData)
            //         PushMessage(GET_BLOCK, hash);

            //     break;
            // }


            // case GET_BLOCK:
            // {
            //     /* Deserialize the inventory. */
            //     uint1024_t hash;
            //     ssPacket >> hash;

            //     /* Check if you have it. */
            //     TAO::Ledger::BlockState state;
            //     if(LLD::Ledger->ReadBlock(hash, state))
            //     {
            //         TAO::Ledger::TritiumBlock block(state);
            //         PushMessage(DAT_BLOCK, block);
            //     }

            //     break;
            // }


            case DAT_TRANSACTION:
            {
                /* Deserialize the tx. */
                uint8_t type;
                ssPacket >> type;

                //PS TODO See if we can't refactor this if...else since most of the code is cloned
                if(type == LLP::MSG_TX_TRITIUM)
                {
                    TAO::Ledger::Transaction tx;
                    ssPacket >> tx;


                    /* Check if we have it. */
                    if(!LLD::Ledger->HasTx(tx.GetHash()))
                    {
                        /* Debug output for tx. */
                        debug::log(3, NODE "Received tx ", tx.GetHash().ToString().substr(0, 20));

                        /* Add the transaction to the memory pool. */
                        if(TAO::Ledger::mempool.Accept(tx, this))
                        {
                            std::vector<CInv> vInv = { CInv(tx.GetHash(), MSG_TX_TRITIUM) };
                            TRITIUM_SERVER->Relay(DAT_INVENTORY, vInv);
                        }
                        else
                        {
                            /* Give this item a time penalty in the relay cache to make it ignored from here forward. */
                            //cacheInventory.Ban(tx.GetHash());
                        }
                    }
                    else
                    {
                        /* Debug output for offsets. */
                       debug::log(3, NODE "already have tx ", tx.GetHash().ToString().substr(0, 20));
                    }

                }
                else if(type == LLP::MSG_TX_LEGACY)
                {
                    Legacy::Transaction tx;
                    ssPacket >> tx;


                    /* Check if we have it. */
                    if(!LLD::Legacy->HasTx(tx.GetHash()))
                    {
                        /* Debug output for tx. */
                        debug::log(3, NODE "Received tx ", tx.GetHash().ToString().substr(0, 20));

                        /* Check if tx is valid. */
                        if(!tx.CheckTransaction())
                        {
                            debug::error(NODE "tx ", tx.GetHash().ToString().substr(0, 20), " REJECTED");

                            break;
                        }

                        /* Add the transaction to the memory pool. */
                        if(TAO::Ledger::mempool.Accept(tx))
                        {
                            TAO::Ledger::BlockState notUsed;
                            Legacy::Wallet::GetInstance().AddToWalletIfInvolvingMe(tx, notUsed, true);

                            std::vector<CInv> vInv = { CInv(tx.GetHash(), MSG_TX_LEGACY) };
                            TRITIUM_SERVER->Relay(DAT_INVENTORY, vInv);
                        }
                        else
                        {
                            /* Give this item a time penalty in the relay cache to make it ignored from here forward. */
                            cacheInventory.Ban(tx.GetHash());
                        }
                    }
                    else
                    {
                        /* Debug output for offsets. */
                        debug::log(3, NODE "already have tx ", tx.GetHash().ToString().substr(0, 20));
                    }

                }

                break;
            }
            case DAT_BLOCK:
            {
                uint8_t type;
                ssPacket >> type;

                if(type == MSG_BLOCK_LEGACY)
                {
                    /* Deserialize the block. */
                    Legacy::LegacyBlock block;
                    ssPacket >> block;

                    debug::log(3, NODE "Received legacy block data ", block.GetHash().ToString().substr(0, 20));

                    /* Process the block. */
                    LegacyNode::Process(block, nullptr);
                }
                else if(type == MSG_BLOCK_TRITIUM)
                {
                    /* Deserialize the block. */
                    TAO::Ledger::TritiumBlock block;
                    ssPacket >> block;

                    debug::log(3, NODE "Received tritium block data ", block.GetHash().ToString().substr(0, 20));

                    /* Process the block. */
                    TritiumNode::Process(block, this);
                }

                break;
            }


            case GET_ADDRESSES:
            {
                /* Grab the connections. */
                if(TRITIUM_SERVER)
                {
                    std::vector<LegacyAddress> vAddr = TRITIUM_SERVER->GetAddresses();
                    std::vector<LegacyAddress> vLegacyAddr;

                    for(auto it = vAddr.begin(); it != vAddr.end(); ++it)
                        vLegacyAddr.push_back(*it);

                    /* Push the response addresses. */
                    PushMessage(DAT_ADDRESSES, vLegacyAddr);
                }

                break;
            }


            case DAT_ADDRESSES:
            {
                /* De-Serialize the Addresses. */
                std::vector<LegacyAddress> vLegacyAddr;
                std::vector<BaseAddress> vAddr;
                ssPacket >> vLegacyAddr;

                /* Don't want addr from older versions unless seeding */
                if(vLegacyAddr.size() > 2000)
                {
                    if(DDOS)
                        DDOS->rSCORE += 20;

                    return debug::error(NODE, "message addr size() = ", vLegacyAddr.size(), "... Dropping Connection");
                }

                if(TRITIUM_SERVER)
                {
                    /* try to establish the connection on the port the server is listening to */
                    for(auto it = vLegacyAddr.begin(); it != vLegacyAddr.end(); ++it)
                    {
                        it->SetPort(TRITIUM_SERVER->GetPort());

                        /* Create a base address vector from legacy addresses */
                        vAddr.push_back(*it);
                    }


                    /* Add the connections to Tritium Server. */
                    if(TRITIUM_SERVER->pAddressManager)
                        TRITIUM_SERVER->pAddressManager->AddAddresses(vAddr);
                }

                break;
            }

            case DAT_PING:
            {
                /* Deserialize the nOnce. */
                uint64_t nNonce;
                ssPacket >> nNonce;

                debug::log(3, NODE "Ping (Nonce ", std::hex, nNonce, ")");

                /* Push a pong as a response. */
                PushMessage(DAT_PONG, nNonce);

                break;
            }

            case DAT_PONG:
            {
                /* Deserialize the nOnce. */
                uint64_t nNonce;
                ssPacket >> nNonce;

                /* If the nonce was not received or known from pong. */
                if(!mapLatencyTracker.count(nNonce))
                {
                    if(DDOS)
                        DDOS->rSCORE += 5;

                    return true;
                }

                /* Calculate the Average Latency of the Connection. */
                nLatency = mapLatencyTracker[nNonce].ElapsedMilliseconds();
                mapLatencyTracker.erase(nNonce);

                /* Set the latency used for address manager within server */
                if(TRITIUM_SERVER && TRITIUM_SERVER->pAddressManager)
                    TRITIUM_SERVER->pAddressManager->SetLatency(nLatency, GetAddress());

                /* Debug output for latency. */
                debug::log(3, NODE "Latency (Nonce ", std::hex, nNonce, " - ", std::dec, nLatency, " ms)");

                /* Clear the latency tracker record. */
                mapLatencyTracker.erase(nNonce);

                break;
            }
        }

        return true;
    }

    /* pnode = Node we received block from, nullptr if we are originating the block (mined or staked) */
    bool TritiumNode::Process(const TAO::Ledger::Block& block, TritiumNode* pnode)
    {
        uint1024_t hash = block.GetHash();

        /* Check if the block is valid. */
        if(!block.Check())
            return false;

        /* Check for orphan. */
        if(!LLD::Ledger->HasBlock(block.hashPrevBlock))
        {
            /* Fast sync block requests. */
            if(!TAO::Ledger::ChainState::Synchronizing())
                pnode->PushGetInventory(TAO::Ledger::ChainState::hashBestChain.load(), uint1024_t(0));
            else if(!config::GetBoolArg("-fastsync", true))
            {
                /* Switch to a new node. */
                SwitchNode();
            }

            LOCK(ORPHAN_MUTEX);

            /* Continue if already have this orphan. */
            if(mapOrphans.count(block.hashPrevBlock))
                return true;

            /* Increment the consecutive orphans. */
            if(pnode)
                ++pnode->nConsecutiveOrphans;

            /* Detect large orphan chains and ask for new blocks from origin again. */
            if(pnode
            && pnode->nConsecutiveOrphans > 1000)
            {
                debug::log(0, FUNCTION, "node reached orphan limit... closing");

                /* Clear the memory to prevent DoS attacks. */
                mapOrphans.clear();

                /* Disconnect from a node with large orphan chain. */
                return false;
            }

            /* Skip if already in orphan queue. */
            if(!mapOrphans.count(block.hashPrevBlock))
                mapOrphans.insert(std::make_pair(block.hashPrevBlock, std::unique_ptr<TAO::Ledger::Block>(block.Clone())));

            /* Debug output. */
            debug::log(0, FUNCTION, "ORPHAN height=", block.nHeight, " prev=", block.hashPrevBlock.ToString().substr(0, 20));

            return true;
        }

        /* Check if valid in the chain. */
        LOCK(PROCESSING_MUTEX);
        if(!block.Accept())
        {
            /* Increment the consecutive failures. */
            if(pnode)
                ++pnode->nConsecutiveFails;

            /* Check for failure limit on node. */
            if(pnode
            && pnode->nConsecutiveFails > 1000)
            {

                /* Fast Sync node switch. */
                if(config::GetBoolArg("-fastsync", true)
                && TAO::Ledger::ChainState::Synchronizing())
                {
                    /* Find a new fast sync node if too many failures. */
                    if(addrFastSync == pnode->GetAddress())
                    {
                        SwitchNode();
                    }
                }

                /* Drop pesky nodes. */
                return false;
            }

            return true;
        }
        else
        {
            /* Special meter for synchronizing. */
            if(TAO::Ledger::ChainState::Synchronizing()
            && block.nHeight % 1000 == 0)
            {
                uint64_t nElapsed = runtime::timestamp(true) - nTimer;
                debug::log(0, FUNCTION,
                    "Processed 1000 blocks in ", nElapsed, " ms [", std::setw(2),
                    TAO::Ledger::ChainState::PercentSynchronized(), " %]",
                    " height=", TAO::Ledger::ChainState::nBestHeight.load(),
                    " trust=", TAO::Ledger::ChainState::nBestChainTrust.load(),
                    " [", 1000000 / nElapsed, " blocks/s]");

                nTimer = runtime::timestamp(true);
            }

            /* Update the last time received. */
            nLastTimeReceived = runtime::timestamp();

            /* Reset the consecutive failures. */
            if(pnode)
                pnode->nConsecutiveFails = 0;

            /* Reset the consecutive orphans. */
            if(pnode)
                pnode->nConsecutiveOrphans = 0;
        }

        /* Process orphan if found. */
        std::unique_lock<std::mutex> lock(ORPHAN_MUTEX);
        while(mapOrphans.count(hash))
        {
            uint1024_t hashPrev = mapOrphans[hash]->GetHash();

            debug::log(0, FUNCTION, "processing ORPHAN prev=", hashPrev.ToString().substr(0, 20), " size=", mapOrphans.size());

            if(!mapOrphans[hash]->Accept())
                return true;

            mapOrphans.erase(hash);
            hash = hashPrev;
        }

        return true;
    }


    /* Send a request to get recent inventory from remote node. */
    void TritiumNode::PushGetInventory(const uint1024_t& hashBlockFrom, const uint1024_t& hashBlockTo)
    {
        /* Filter out duplicate requests. */
        if(hashLastGetblocks.load() == hashBlockFrom && nLastGetBlocks.load() + 1 > runtime::timestamp())
            return;

        /* Set the fast sync address. */
        if(addrFastSync != GetAddress())
        {
            /* Set the new sync address. */
            addrFastSync = GetAddress();

            /* Reset the last time received. */
            nLastTimeReceived = runtime::timestamp();

            debug::log(0, NODE, "New sync address set");
        }

        /* Calculate the fast sync average. */
        nFastSyncAverage = std::min((uint64_t)25, (nFastSyncAverage.load() + (runtime::timestamp() - nLastGetBlocks.load())) / 2);

        /* Update the last timestamp this was called. */
        nLastGetBlocks = runtime::timestamp();

        /* Update the hash that was used for last request. */
        hashLastGetblocks = hashBlockFrom;

        /* Push the request to the node. */
        PushMessage(GET_INVENTORY, TAO::Ledger::Locator(hashBlockFrom), hashBlockTo);

        /* Debug output for monitoring. */
        debug::log(0, NODE, "(", nFastSyncAverage.load(), ") requesting getinventory from ", hashBlockFrom.ToString().substr(0, 20), " to ", hashBlockTo.ToString().substr(0, 20));
    }


    /*  Non-Blocking Packet reader to build a packet from TCP Connection.
     *  This keeps thread from spending too much time for each Connection. */
   void TritiumNode::ReadPacket()
   {
       if(!INCOMING.Complete())
       {
           /** Handle Reading Packet Length Header. **/
           if(INCOMING.IsNull() && Available() >= 10)
           {
               std::vector<uint8_t> BYTES(10, 0);
               if(Read(BYTES, 10) == 10)
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
               std::vector<uint8_t> DATA( std::min( nAvailable, (uint32_t)(INCOMING.LENGTH - INCOMING.DATA.size())), 0);

               /* Read up to 512 bytes of data. */
               if(Read(DATA, DATA.size()) == DATA.size())
               {
                   INCOMING.DATA.insert(INCOMING.DATA.end(), DATA.begin(), DATA.end());
                   Event(EVENT_PACKET, static_cast<uint32_t>(DATA.size()));
               }
           }
       }
   }
}
