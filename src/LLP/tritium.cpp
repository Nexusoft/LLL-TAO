/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLP/types/tritium.h>

#include <TAO/Operation/include/execute.h>
#include <Legacy/wallet/wallet.h>
#include <LLC/include/random.h>

#include <LLD/include/global.h>

#include <LLP/include/global.h>
#include <LLP/templates/events.h>
#include <LLP/include/manager.h>

#include <TAO/Ledger/types/mempool.h>

#include <Util/include/runtime.h>
#include <Util/include/args.h>
#include <Util/include/debug.h>
#include <LLP/include/inv.h>

#include <climits>
#include <memory>

namespace
{
    std::atomic<uint32_t> nAsked(0);

    /* Static instantiation of orphan blocks in queue to process. */
    std::map<uint1024_t, std::unique_ptr<TAO::Ledger::Block>> mapOrphans;

    /* Mutex to protect checking more than one block at a time. */
    std::mutex PROCESSING_MUTEX;
}

namespace LLP
{
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

    /** Virtual Functions to Determine Behavior of Message LLP. **/
    void TritiumNode::Event(uint8_t EVENT, uint32_t LENGTH)
    {
        switch(EVENT)
        {
            case EVENT_CONNECT:
            {
                /* Setup the variables for this node. */
                nLastPing    = runtime::timestamp();

                /* Debut output. */
                debug::log(1, NODE, " Connected at timestamp ", runtime::unifiedtimestamp());

                if(fOUTGOING)
                {
                    /* Send version if making the connection. */
                    PushMessage(DAT_VERSION, TritiumNode::nSessionID, GetAddress());

                    /* Ask the node you are connecting to for their inventory*/
                    PushGetInventory(TAO::Ledger::ChainState::hashBestChain.load(), uint1024_t(0));
                }

                break;
            }

            case EVENT_HEADER:
            {
                break;
            }

            case EVENT_PACKET:
            {
                break;
            }

            case EVENT_GENERIC:
            {
                /* Generic event - pings. */
                if(runtime::timestamp() - nLastPing > 5)
                {
                    /* Generate the nNonce. */
                    uint64_t nNonce = LLC::GetRand(std::numeric_limits<uint64_t>::max());

                    /* Add to latency tracker. */
                    mapLatencyTracker[nNonce] = runtime::timestamp(true);

                    /* Push a ping request. */
                    PushMessage(DAT_PING, nNonce);

                    /* Update the last ping. */
                    nLastPing = runtime::timestamp();
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
                if(config::GetBoolArg("-fastsync")
                && TAO::Ledger::ChainState::Synchronizing()
                && addrFastSync == GetAddress()
                && nLastTimeReceived.load() + 10 < runtime::timestamp()
                && nLastGetBlocks.load() + 10 < runtime::timestamp())
                {
                    debug::log(0, NODE, "fast sync event timeout");

                    /* Normal case of asking for a getblocks inventory message. */
                    memory::atomic_ptr<TritiumNode>& pBest = TRITIUM_SERVER->GetConnection(addrFastSync.load());

                    /* Null pointer check. */
                    if(pBest != nullptr)
                    {
                        try
                        {
                            /* Ask for another inventory batch. */
                            pBest->PushGetInventory(TAO::Ledger::ChainState::hashBestChain.load(), uint1024_t(0));

                        }
                        catch(const std::runtime_error& e)
                        {
                            debug::error(FUNCTION, e.what());
                        }
                    }
                    else
                    {
                        debug::error(FUNCTION, "no nodes available to switch");

                        nLastTimeReceived = runtime::timestamp();
                        nLastGetBlocks    = runtime::timestamp();
                    }
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
                    /* Normal case of asking for a getblocks inventory message. */
                    memory::atomic_ptr<TritiumNode>& pBest = TRITIUM_SERVER->GetConnection(addrFastSync.load());

                    /* Null check the pointer. */
                    if(pBest != nullptr)
                    {
                        try
                        {
                            /* Switch to a new node for fast sync. */
                            pBest->PushGetInventory(TAO::Ledger::ChainState::hashBestChain.load(), uint1024_t(0));

                            /* Debug output. */
                            debug::log(0, NODE, "fast sync node dropped, switching to ", addrFastSync.load().ToStringIP());
                        }
                        catch(const std::runtime_error& e)
                        {
                            debug::error(FUNCTION, e.what());
                        }
                    }
                }

                /* Print disconnect and reason message */
                debug::log(1, NODE, " Disconnected at timestamp ", runtime::unifiedtimestamp(),
                    " (", strReason, ")");

                if(TRITIUM_SERVER && TRITIUM_SERVER->pAddressManager)
                    TRITIUM_SERVER->pAddressManager->AddAddress(GetAddress(), ConnectState::DROPPED);

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

            case DAT_VERSION:
            {
                /* Deserialize the session identifier. */
                uint64_t nSession;
                ssPacket >> nSession;

                /* Get your address. */
                BaseAddress addr;
                ssPacket >> addr;

                /* Check for a connect to self. */
                if(nSession == TritiumNode::nSessionID)
                {
                    debug::log(0, FUNCTION, "connected to self");

                    /* Cache self-address in the banned list of the Address Manager. */
                    if(TRITIUM_SERVER && TRITIUM_SERVER->pAddressManager)
                        TRITIUM_SERVER->pAddressManager->Ban(addr);

                    return false;
                }



                /* Send version message if connection is inbound. */
                if(!fOUTGOING)
                    PushMessage(DAT_VERSION, TritiumNode::nSessionID, GetAddress());
                else
                    PushMessage(GET_ADDRESSES);

                if (fOUTGOING && nAsked == 0)
                {
                    ++nAsked;
                    PushGetInventory(TAO::Ledger::ChainState::hashBestChain.load(), uint1024_t(0));
                }

                /* Debug output for offsets. */
                debug::log(3, NODE, "received session identifier ",nSessionID);

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
                debug::log(3, NODE, "received timestamp of ", nTimestamp, " sending offset ", nOffset);

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
                    return debug::error(NODE "offset not requested");

                /* Check the time since request was sent. */
                if(runtime::timestamp() - mapSentRequests[nRequestID] > 10)
                {
                    debug::log(0, NODE, "offset is stale.");
                    mapSentRequests.erase(nRequestID);

                    break;
                }

                /* Deserialize the offset. */
                int32_t nOffset;
                ssPacket >> nOffset;

                /* Debug output for offsets. */
                debug::log(3, NODE, "received offset ", nOffset);

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
                    if(LLD::legDB->ReadBlock(have, state))
                        break;
                }

                /* If no ancestor blocks were found. */
                if(!state)
                    return debug::error(FUNCTION, "no ancestor blocks found");

                /* Set the search from search limit. */
                int32_t nLimit = 1000;
                debug::log(2, "getblocks ", state.nHeight, " to ", hashStop.ToString().substr(0, 20), " limit ", nLimit);

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
                    if (state.GetHash() == hashStop)
                    {
                        debug::log(3, "  getblocks stopping at ", state.nHeight, " to ", state.GetHash().ToString().substr(0, 20));

                        /* Tell about latest block if hash stop is found. */
                        if (hashStop != TAO::Ledger::ChainState::hashBestChain.load())
                        {
                            /* first add all of the transactions hashes from the block */
                            for( const auto& tx : state.vtx)
                                vInv.push_back(CInv(tx.second, tx.first == TAO::Ledger::TYPE::LEGACY_TX ? MSG_TX_LEGACY : MSG_TX_TRITIUM)); 

                            /* lastly add the block hash */
                            vInv.push_back(CInv(TAO::Ledger::ChainState::hashBestChain.load(), fIsLegacy ? MSG_BLOCK_LEGACY : MSG_BLOCK_TRITIUM));
                        }
                            

                        break;
                    }

                    /* Push new item to inventory. */
                    /* first add all of the transactions hashes from the block */
                    for( const auto& tx : state.vtx)
                        vInv.push_back(CInv(tx.second, tx.first == TAO::Ledger::TYPE::LEGACY_TX ? MSG_TX_LEGACY : MSG_TX_TRITIUM));
                    
                    /* lastly add the block hash */
                    vInv.push_back(CInv(state.GetHash(), fIsLegacy ? MSG_BLOCK_LEGACY : MSG_BLOCK_TRITIUM));

                    /* Stop at limits. */
                    if (--nLimit <= 0)
                    {
                        // When this block is requested, we'll send an inv that'll make them
                        // getblocks the next batch of inventory.
                        debug::log(3, "  getblocks stopping at limit ", state.nHeight, " to ", state.GetHash().ToString().substr(0,20));

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
            case DAT_INVENTORY :
            {
                std::vector<CInv> vInv;
                ssPacket >> vInv;

                debug::log(3, NODE, "Inventory Message of ", vInv.size(), " elements");

                /* Make sure the inventory size is not too large. */
                if (vInv.size() > 10000)
                {
                    DDOS->rSCORE += 20;

                    return true;
                }

                /* Fast sync mode. */
                if(config::GetBoolArg("-fastsync")
                && addrFastSync == GetAddress()
                && TAO::Ledger::ChainState::Synchronizing()
                && (vInv.back().GetType() == MSG_BLOCK_LEGACY || vInv.back().GetType() == MSG_BLOCK_TRITIUM )
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
                            if(!cacheInventory.Has(inv.GetHash())
                            && !LLD::legDB->HasBlock(inv.GetHash()))
                            {
                                /* Add this item to request queue. */
                                vGet.push_back(inv);

                                /* Add this item to cached relay inventory (key only). */
                                cacheInventory.Add(inv.GetHash());
                            }
                            else
                                break; //break since iterating backwards (searching newest to oldest)
                                    //the assumtion here is that if you have newer inventory, you have older
                        }

                        /* Check the memory pool for transactions being relayed. */
                        else if(!cacheInventory.Has(inv.GetHash().getuint512())
                            && !TAO::Ledger::mempool.Has(inv.GetHash().getuint512()))
                        {
                            /* Add this item to request queue. */
                            vGet.push_back(inv);

                            /* Add this item to cached relay inventory (key only). */
                            cacheInventory.Add(inv.GetHash().getuint512());
                        }
                    }

                    /* Reverse inventory to get to receive data in proper order. */
                    std::reverse(vGet.begin(), vGet.end());

                }
                else
                    vGet = vInv; /* just request data for all inventory*/ 

                /* Ask your friendly neighborhood node for the data */
                
                PushMessage(GET_DATA, vGet);

                break;
            }

            /* Get the Data for either a transaction or a block. */
            case GET_DATA:
            {
                std::vector<CInv> vInv;
                ssPacket >> vInv;

                if (vInv.size() > 10000)
                {
                    DDOS->rSCORE += 20;

                    return true;
                }

                debug::log(3, FUNCTION, "received getdata of ", vInv.size(), " elements");

                /* Loop the inventory and deliver messages. */
                for(const auto& inv : vInv)
                {

                    /* Log the inventory message receive. */
                    debug::log(3, FUNCTION, "processing getdata ", inv.ToString());

                    /* Handle the block message. */
                    if (inv.GetType() == MSG_BLOCK_LEGACY || inv.GetType() == MSG_BLOCK_TRITIUM)
                    {
                        /* Don't send genesis if asked for. */
                        if(inv.GetHash() == TAO::Ledger::ChainState::Genesis())
                            continue;

                        /* Read the block from disk. */
                        TAO::Ledger::BlockState state;
                        if(!LLD::legDB->ReadBlock(inv.GetHash(), state))
                            continue;

                        /* Push the response message. */
                        if( inv.GetType() == MSG_BLOCK_TRITIUM)
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
                        if (inv.GetHash() == hashContinue)
                        {
                            bool fStateBestIsLegacy = TAO::Ledger::ChainState::stateBest.load().vtx[0].first == TAO::Ledger::TYPE::LEGACY_TX;
                            std::vector<CInv> vInv = { CInv(TAO::Ledger::ChainState::hashBestChain.load(), fStateBestIsLegacy ? LLP::MSG_BLOCK_LEGACY : MSG_BLOCK_TRITIUM) };
                            PushMessage(GET_INVENTORY, vInv);
                            hashContinue = 0;
                        }
                    }
                    else if (inv.GetType() == LLP::MSG_TX_TRITIUM)
                    {
                        TAO::Ledger::Transaction tx;
                        if(!TAO::Ledger::mempool.Get(inv.GetHash().getuint512(), tx) && !LLD::legDB->ReadTx(inv.GetHash().getuint512(), tx))
                            continue;

                        PushMessage(DAT_TRANSACTION, (uint8_t)LLP::MSG_TX_TRITIUM, tx);
                    }
                    else if (inv.GetType() == LLP::MSG_TX_LEGACY)
                    {
                        Legacy::Transaction tx;
                        if(!TAO::Ledger::mempool.Get(inv.GetHash().getuint512(), tx) && !LLD::legacyDB->ReadTx(inv.GetHash().getuint512(), tx))
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
            //     if(LLD::legDB->ReadTx(hash, tx) || TAO::Ledger::mempool.Get(hash, tx))
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
            //     if(LLD::legDB->ReadBlock(hash, state))
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
                    if(!LLD::legDB->HasTx(tx.GetHash()))
                    {
                        /* Debug output for tx. */
                        debug::log(3, NODE "Received tx ", tx.GetHash().ToString().substr(0, 20));

                        /* Check if tx is valid. */
                        if(!tx.IsValid())
                        {
                            debug::error(NODE "tx ", tx.GetHash().ToString().substr(0, 20), " REJECTED");

                            break;
                        }

                        /* Add the transaction to the memory pool. */
                        if (TAO::Ledger::mempool.Accept(tx))
                        {
                            std::vector<CInv> vInv = { CInv(tx.GetHash(), MSG_TX_TRITIUM) };
                            TRITIUM_SERVER->Relay(DAT_INVENTORY, vInv);
                        }
                        else
                        {
                            /* Give this item a time penalty in the relay cache to make it ignored from here forward. */
                            cacheInventory.Ban(tx.GetHash());
                        }
                    }

                    /* Debug output for offsets. */
                    debug::log(3, NODE "already have tx ", tx.GetHash().ToString().substr(0, 20));
                }
                else if(type == LLP::MSG_TX_LEGACY)
                {
                    Legacy::Transaction tx;
                    ssPacket >> tx;
                

                    /* Check if we have it. */
                    if(!LLD::legacyDB->HasTx(tx.GetHash()))
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
                        if (TAO::Ledger::mempool.Accept(tx))
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

                    /* Debug output for offsets. */
                    debug::log(3, NODE "already have tx ", tx.GetHash().ToString().substr(0, 20));
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
                    if(!LegacyNode::Process(block, nullptr))
                        return false;
                }
                else if(type == MSG_BLOCK_TRITIUM)
                {
                    /* Deserialize the block. */
                    TAO::Ledger::TritiumBlock block;
                    ssPacket >> block;

                    debug::log(3, NODE "Received tritium block data ", block.GetHash().ToString().substr(0, 20));

                    /* Process the block. */
                    if(!TritiumNode::Process(block, this))
                        return false;
                }
                return true;
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

                if(TRITIUM_SERVER)
                {
                    /* try to establish the connection on the port the server is listening to */
                    for(auto it = vLegacyAddr.begin(); it != vLegacyAddr.end(); ++it)
                    {
                        if(config::mapArgs.find("-port") != config::mapArgs.end())
                            it->SetPort(atoi(config::mapArgs["-port"].c_str()));
                        else
                            it->SetPort(TRITIUM_SERVER->PORT);

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
                uint32_t nNonce;
                ssPacket >> nNonce;

                /* Push a pong as a response. */
                PushMessage(DAT_PONG, nNonce);

                break;
            }

            case DAT_PONG:
            {
                /* Deserialize the nOnce. */
                uint32_t nNonce;
                ssPacket >> nNonce;

                /* Check for unsolicted pongs. */
                if(!mapLatencyTracker.count(nNonce))
                    return debug::error(NODE "unsolicited pong");

                uint32_t lat = runtime::timestamp(true) - mapLatencyTracker[nNonce];

                /* Set the latency used for address manager within server */
                if(TRITIUM_SERVER && TRITIUM_SERVER->pAddressManager)
                    TRITIUM_SERVER->pAddressManager->SetLatency(lat, GetAddress());

                /* Debug output for latency. */
                debug::log(3, NODE "latency ", lat, " ms");

                /* Clear the latency tracker record. */
                mapLatencyTracker.erase(nNonce);

                break;
            }
        }

        return true;
    }

    /* pnode = Node we received block from, nullptr if we are originating the block (mined or staked) */
    std::map<uint1024_t, std::unique_ptr<TAO::Ledger::Block>> mapTest;
    bool TritiumNode::Process(const TAO::Ledger::Block& block, TritiumNode* pnode)
    {
        LOCK(PROCESSING_MUTEX);
        
        /* Check if the block is valid. */
        uint1024_t hash = block.GetHash();
        if(!block.Check())
            return true;

        /* Check for orphan. */
        if(!LLD::legDB->HasBlock(block.hashPrevBlock))
        {
            /* Fast sync block requests. */
            if(!TAO::Ledger::ChainState::Synchronizing())
                pnode->PushGetInventory(TAO::Ledger::ChainState::hashBestChain.load(), uint1024_t(0));
            else if(!config::GetBoolArg("-fastsync"))
            {
                /* Normal case of asking for a getblocks inventory message. */
                memory::atomic_ptr<TritiumNode>& pBest = TRITIUM_SERVER->GetConnection(addrFastSync.load());

                /* Null check the pointer. */
                if(pBest != nullptr)
                {
                    try
                    {
                        /* Push a new getblocks request. */
                        pBest->PushGetInventory(TAO::Ledger::ChainState::hashBestChain.load(), uint1024_t(0));

                    }
                    catch(const std::runtime_error& e)
                    {
                        debug::error(FUNCTION, e.what());
                    }
                }
            }

            /* Continue if already have this orphan. */
            if(mapOrphans.count(block.hashPrevBlock))
                return true;

            /* Increment the consecutive orphans. */
            if(pnode)
                ++pnode->nConsecutiveOrphans;

            /* Detect large orphan chains and ask for new blocks from origin again. */
            if(pnode
            && pnode->nConsecutiveOrphans >= 500)
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
        if(!block.Accept())
        {
            /* Increment the consecutive failures. */
            if(pnode)
                ++pnode->nConsecutiveFails;

            /* Check for failure limit on node. */
            if(pnode
            && pnode->nConsecutiveFails >= 500)
            {

                /* Fast Sync node switch. */
                if(config::GetBoolArg("-fastsync")
                && TAO::Ledger::ChainState::Synchronizing())
                {
                    /* Find a new fast sync node if too many failures. */
                    if(addrFastSync == pnode->GetAddress())
                    {
                        /* Normal case of asking for a getblocks inventory message. */
                        memory::atomic_ptr<TritiumNode>& pBest = TRITIUM_SERVER->GetConnection(addrFastSync.load());

                        /* Null check the pointer. */
                        if(pBest != nullptr)
                        {
                            try
                            {
                                /* Switch to a new node for fast sync. */
                                pBest->PushGetInventory(TAO::Ledger::ChainState::hashBestChain.load(), uint1024_t(0));

                                /* Debug output. */
                                debug::error(FUNCTION, "fast sync node reached failure limit...");
                            }
                            catch(const std::runtime_error& e)
                            {
                                debug::error(FUNCTION, e.what());
                            }
                        }
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
                    " [", 1000000 / nElapsed, " blocks/s]" );

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

}
