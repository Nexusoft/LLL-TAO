/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2017] ++
            
            (c) Copyright The Nexus Developers 2014 - 2017
            
            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.
            
            "fides in stellis, virtus in numeris" - Faith in the Stars, Power in Numbers

____________________________________________________________________________________________*/


#include "../Core/include/manager.h"

#include "include/checkpoints.h"
#include "include/message.h"
#include "include/hosts.h"
#include "include/legacy.h"
#include "include/inv.h"

#include "../LLC/include/random.h"
#include "../LLD/include/index.h"

#include "../Util/include/args.h"
#include "../Util/include/hex.h"

#include "../main.h"


namespace LLP
{	
    
    /* Push a Message With Information about This Current Node. */
    void CLegacyNode::PushVersion()
    {
        /* Random Session ID */
        RAND_bytes((unsigned char*)&nSessionID, sizeof(nSessionID));
        
        /* Current Unified Timestamp. */
        int64 nTime = Core::UnifiedTimestamp();
        
        /* Dummy Variable NOTE: Remove in Tritium ++ */
        uint64 nLocalServices = 0;
        
        /* Relay Your Address. */
        CAddress addrMe  = CAddress(CService("0.0.0.0",0));
        CAddress addrYou = CAddress(CService("0.0.0.0",0));
        
        /* Push the Message to receiving node. */
        PushMessage("version", PROTOCOL_VERSION, nLocalServices, nTime, addrYou, addrMe,
                    nSessionID, FormatFullVersion(), Core::nBestHeight);
    }
    
    
    /** Handle Event Inheritance. **/
    void CLegacyNode::Event(unsigned char EVENT, unsigned int LENGTH)
    {
        /** Handle any DDOS Packet Filters. **/
        if(EVENT == EVENT_HEADER)
        {
            if(GetArg("-verbose", 0) >= 4)
                printf("***** Node recieved Message (%s, %u)\n", INCOMING.GetMessage().c_str(), INCOMING.LENGTH);
                    
            if(fDDOS)
            {
                
                /* Give higher DDOS score if the Node happens to try to send multiple version messages. */
                if (INCOMING.GetMessage() == "version" && nCurrentVersion != 0)
                    DDOS->rSCORE += 25;
                
                
                /* Check the Packet Sizes to Unified Time Commands. */
                if((INCOMING.GetMessage() == "getoffset" || INCOMING.GetMessage() == "offset") && INCOMING.LENGTH != 16)
                    DDOS->Ban(strprintf("INVALID PACKET SIZE | OFFSET/GETOFFSET | LENGTH %u", INCOMING.LENGTH));
            }
            
            return;
        }
            
        /** Handle for a Packet Data Read. **/
        if(EVENT == EVENT_PACKET)
        {
            if(GetArg("-verbose", 0) >= 5)
                printf("***** Node Read Data for Message (%s, %u, %s)\n", INCOMING.GetMessage().c_str(), LENGTH, INCOMING.Complete() ? "TRUE" : "FALSE");
                    
            /* Check a packet's validity once it is finished being read. */
            if(fDDOS) {

                /* Give higher score for Bad Packets. */
                if(INCOMING.Complete() && !INCOMING.IsValid()){
                    
                    if(GetArg("-verbose", 0) >= 3)
                        printf("***** Dropped Packet (Complete: %s - Valid: %s)\n", INCOMING.Complete() ? "Y" : "N" , INCOMING.IsValid() ? "Y" : "N" );
                    
                    DDOS->rSCORE += 15;
                }

            }
                
            return;
        }
        
        
        /* Handle Node Pings on Generic Events */
        if(EVENT == EVENT_GENERIC)
        {
            
            if(nLastPing + 5 < Core::UnifiedTimestamp()) {
                RAND_bytes((unsigned char*)&nSessionID, sizeof(nSessionID));
                
                nLastPing = Core::UnifiedTimestamp();
                cLatencyTimer.Reset();
                
                PushMessage("ping", nSessionID);
            }
            
            //TODO: mapRequests data, if no response given retry the request at given times 
        }
            
            
        /** On Connect Event, Assign the Proper Handle. **/
        if(EVENT == EVENT_CONNECT)
        {
            addrThisNode = CAddress(CService(GetIPAddress(), GetDefaultPort()));
            nLastPing    = Core::UnifiedTimestamp();
            
            if(GetArg("-verbose", 0) >= 1)
                printf("***** %s Node %s Connected at Timestamp %" PRIu64 "\n", fOUTGOING ? "Ougoing" : "Incoming", addrThisNode.ToString().c_str(), Core::UnifiedTimestamp());
            
            if(fOUTGOING)
                PushVersion();
            
            return;
        }
        
        
        /* Handle the Socket Disconnect */
        if(EVENT == EVENT_DISCONNECT)
        {
            Core::pManager->vDropped.push_back(addrThisNode);
            
            if(GetArg("-verbose", 0) >= 1)
                printf("xxxxx %s Node %s Disconnected (%s) at Timestamp %" PRIu64 "\n", fOUTGOING ? "Ougoing" : "Incoming", addrThisNode.ToString().c_str(), ErrorMessage().c_str(), Core::UnifiedTimestamp());
            
            return;
        }
        
    }
        
        
    /** This function is necessary for a template LLP server. It handles your 
        custom messaging system, and how to interpret it from raw packets. **/
    bool CLegacyNode::ProcessPacket()
    {
        CDataStream ssMessage(INCOMING.DATA, SER_NETWORK, MIN_PROTO_VERSION);
        
        
        if(INCOMING.GetMessage() == "getoffset")
        {
            /* Don't service unified seeds unless time is unified. */
            if(!Core::fTimeUnified)
                return true;
            
            /* De-Serialize the Request ID. */
            unsigned int nRequestID;
            ssMessage >> nRequestID;
            
            /* De-Serialize the Timestamp Sent. */
            uint64 nTimestamp;
            ssMessage >> nTimestamp;
            
            /* Log into the sent requests Map. */
            mapSentRequests[nRequestID] = Core::UnifiedTimestamp(true);
            
            /* Calculate the offset to current clock. */
            int   nOffset    = (int)(Core::UnifiedTimestamp(true) - nTimestamp);
            PushMessage("offset", nRequestID, Core::UnifiedTimestamp(true), nOffset);
                
            if(GetArg("-verbose", 0) >= 3)
                printf("***** Node: Sent Offset %i | %s | Unified %" PRIu64 "\n", nOffset, addrThisNode.ToString().c_str(), Core::UnifiedTimestamp());

            return true;
        }
        
        /* Recieve a Time Offset from this Node. */
        else if(INCOMING.GetMessage() == "offset")
        {
            
            /* De-Serialize the Request ID. */
            unsigned int nRequestID;
            ssMessage >> nRequestID;
            
            
            /* De-Serialize the Timestamp Sent. */
            uint64 nTimestamp;
            ssMessage >> nTimestamp;
            
            
            /* Handle the Request ID's. */
            unsigned int nLatencyTime = (Core::UnifiedTimestamp(true) - nTimestamp);

            
            /* Ignore Messages Recieved that weren't Requested. */
            if(!mapSentRequests.count(nRequestID)) {
                DDOS->rSCORE += 5;
                    
                if(GetArg("-verbose", 0) >= 3)
                    printf("***** Node (%s): Invalid Request : Message Not Requested [%x][%u ms]\n", addrThisNode.ToString().c_str(), nRequestID, nLatencyTime);
                        
                return true;
            }
                
                
            /* Reject Samples that are recieved 30 seconds after last check on this node. */
            if(Core::UnifiedTimestamp(true) - mapSentRequests[nRequestID] > 30000) {
                mapSentRequests.erase(nRequestID);
                    
                if(GetArg("-verbose", 0) >= 3)
                    printf("***** Node (%s): Invalid Request : Message Stale [%x][%u ms]\n", addrThisNode.ToString().c_str(), nRequestID, nLatencyTime);
                        
                DDOS->rSCORE += 15;
                    
                return true;
            }
                

            /* De-Serialize the Offset. */
            int nOffset;
            ssMessage >> nOffset;
                
            if(GetArg("-verbose", 0) >= 3)
                printf("***** Node (%s): Received Unified Offset %i [%x][%u ms]\n", addrThisNode.ToString().c_str(), nOffset, nRequestID, nLatencyTime);
                
            /* Adjust the Offset for Latency. */
            nOffset -= nLatencyTime;
                
            /* Add the Samples. */
            setTimeSamples.insert(nOffset);
                
            /* Remove the Request from the Map. */
            mapSentRequests.erase(nRequestID);
            
            return true;
        }
        
        
        /* Push a transaction into the Node's Recieved Transaction Queue. */
        else if (INCOMING.GetMessage() == "tx")
        {
            
            /* Deserialize the Transaction. */
            Core::CTransaction tx;
            ssMessage >> tx;
            
            
            /* Don't double process what one already has. */
            if(Core::pManager->txPool.Has(tx.GetHash()))
                return true;
            
            
            /* Valid Transaction. */
            bool fMissingInputs = false;
            
            
            LLD::CIndexDB indexdb("r");
            
            Timer cTimer;
            cTimer.Start();
            
            if(!Core::pManager->txPool.Accept(indexdb, tx, true, &fMissingInputs))
            {
                Core::pManager->txPool.Add(tx.GetHash(), tx);
                
                /* Orphaned Transaction. */
                if (fMissingInputs)
                    Core::pManager->txPool.SetState(tx.GetHash(), Core::pManager->txPool.ORPHANED);
                
                else
                    return true;
                
                if(GetArg("-verbose", 0) >= 3)
                    printf("PASSED checks in " PRIu64 " us\n", cTimer.ElapsedMicroseconds());
            }
            
            /* Only relay the transaction data if it was accepted. */
            else
            {
                std::vector<CInv> vInv = { CInv(MSG_TX, tx.GetHash()) };
                
                std::vector<LLP::CLegacyNode*> vNodes = Core::pManager->LegacyServer->GetConnections();
                for(auto node : vNodes)
                    if(node != this)
                        node->PushMessage("inv", vInv);
            }
            
            
            /* Level 3 Debugging: Output Protocol Messages. */
            if(GetArg("-verbose", 0) >= 3)
                printf("received transaction %s\n", tx.GetHash().ToString().substr(0,20).c_str());
                
            
            /* Level 4 Debugging: Output Raw Data Dumps. */
            if(GetArg("-verbose", 0) >= 4)
                tx.print();
            
            return true;
        }
        
        
        /* Push a block into the Node's Recieved Blocks Queue. */
        else if (INCOMING.GetMessage() == "block")
        {
            Core::CBlock block;
            ssMessage >> block;
            
                        
            /* Get the Block Hash. */
            uint1024 hashBlock = block.GetHash();


                
            /* Make sure it's not an already process(ing) block. */
            if(Core::pManager->blkPool.State(hashBlock) != Core::pManager->blkPool.REQUESTED)
            {
                
                if(GetArg("-verbose", 0) >= 3)
                    printf("duplicate block %s\n", hashBlock.ToString().substr(0,20).c_str());
                
                DDOS->rSCORE += 5;
            
                return true;
            }
            
            
            /* Level 3 Debugging: Output Protocol Messages. */
            if(GetArg("-verbose", 0) >= 3)
                printf("received block %s height %u\n", hashBlock.ToString().substr(0,20).c_str(), block.nHeight);
            
            
            /* Level 4 Debugging: Output raw data dumps. */
            if(GetArg("-verbose", 0) >= 3)
                block.print();
            
            
            /* Process the Block. */
            if(!Core::pManager->blkPool.Process(block))
            {
                if(GetArg("-verbose", 0) >= 3)
                    printf("failed block processing %s\n", hashBlock.ToString().substr(0, 20).c_str());
                
                return true;
            }
            
            return true;
        }
        
        
        /* Send a Ping with a nNonce to get Latency Calculations. */
        else if (INCOMING.GetMessage() == "ping")
        {
            uint64 nonce = 0;
            ssMessage >> nonce;
            
            /* Calculate the Average Latency of the Connection. */
            nLastPing = Core::UnifiedTimestamp();
            cLatencyTimer.Start();
                
            PushMessage("pong", nonce);
            
            return true;
        }
        
        
        else if(INCOMING.GetMessage() == "pong")
        {
            uint64 nonce = 0;
            ssMessage >> nonce;
            
            
            /* Calculate the Average Latency of the Connection. */
            nNodeLatency = cLatencyTimer.ElapsedMilliseconds();
            cLatencyTimer.Reset();
            
            
            /* Debug Level 3: output Node Latencies. */
            if(GetArg("-verbose", 0) >= 3)
                printf("***** Node %s Latency (%u ms)\n", addrThisNode.ToString().c_str(), nNodeLatency);
            
            return true;
        }
        
            
        /* ______________________________________________________________
        * 
        * 
        * NOTE: These following methods will be deprecated post Tritium. 
        *
        * ______________________________________________________________
        */
            
            
        /* Message Version is the first message received.
        * It gives you basic stats about the node to know how to
        * communicate with it.
        */
        else if (INCOMING.GetMessage() == "version")
        {
            
            int64 nTime;
            CAddress addrMe;
            CAddress addrFrom;
            uint64 nServices = 0;

            
            /* Check the Protocol Versions */
            ssMessage >> nCurrentVersion;
            
            
            /* Check the Tritium Protocol. */
            if (nCurrentVersion >= MIN_TRITIUM_VERSION)
            {
                if(GetArg("verbose", 0) >= 1)
                    printf("***** Node %s detected on the tritium protocol %i; moving to tritium message sets\n", GetIPAddress().c_str(), PROTOCOL_VERSION);
            }
            
            
            /* Deserialize the rest of the data. */
            ssMessage >> nServices >> nTime >> addrMe >> addrFrom >> nSessionID >> strNodeVersion >> nStartingHeight;
            if(GetArg("-verbose", 0) >= 1)
                printf("***** Node version message: version %d, blocks=%d\n", nCurrentVersion, nStartingHeight);
            
            
            /* Send the Version Response to ensure communication channel is open. */
            PushMessage("verack");
            
            
            /* Push our version back since we just completed getting the version from the other node. */
            if (fOUTGOING)
            {
                Core::cPeerBlockCounts.Add(nStartingHeight);
                
                PushMessage("getheaders", Core::CBlockLocator(Core::pindexBest), uint1024(0));
            }
            else
                PushVersion();
            
            PushMessage("getaddr");
        }

        
        /* Handle a new Address Message. 
        * This allows the exchanging of addresses on the network.
        */
        else if (INCOMING.GetMessage() == "addr")
        {
            std::vector<CAddress> vAddr;
            ssMessage >> vAddr;

            /* Don't want addr from older versions unless seeding */
            if (vAddr.size() > 2000){
                DDOS->rSCORE += 20;
                
                return error("***** Node message addr size() = %d... Dropping Connection", vAddr.size());
            }

            for(auto addr : vAddr)
                Core::pManager->AddAddress(addr);
        }
        
        
        /* Handle new Inventory Messages.
        * This is used to know what other nodes have in their inventory to compare to our own. 
        */
        else if (INCOMING.GetMessage() == "inv")
        {
            std::vector<CInv> vInv;
            ssMessage >> vInv;
            
            
            if(GetArg("-verbose", 0) >= 1)
                printf("***** Inventory Message of %u elements\n", vInv.size());
            
            
            /* Make sure the inventory size is not too large. */
            if (vInv.size() > 10000)
            {
                DDOS->rSCORE += 20;
                
                return true;
            }

            
            std::vector<CInv> vInvNew;
            for (int i = 0; i < vInv.size(); i++)
            {
                /* Log Level 4: Inventory Message (Relay v1.0). */
                if(GetArg("-verbose", 0) >= 4)
                    printf("***** Node recieved inventory: %s\n", vInv[i].ToString().c_str());
                
                /* Skip asking for inventory that is already known. */
                if(vInv[i].type == MSG_BLOCK)
                {
                    if(Core::pManager->blkPool.Has(vInv[i].hash))
                        continue;
                    
                    Core::CBlock blk;
                    Core::pManager->blkPool.Add(vInv[i].hash, blk, Core::pManager->blkPool.REQUESTED);
                }
                
                /* Skip asking for inventory that is already known. */
                if(vInv[i].type == MSG_TX && Core::pManager->txPool.Has(vInv[i].hash.getuint512()))
                    continue;

                /* Add the inventory to new vector. (Only TX) */
                vInvNew.push_back(vInv[i]);
            }
            
            
            /* Ask for the data. */
            PushMessage("getdata", vInvNew);
        }
        
        
        /* Handle block header inventory message
        * 
        * This is just block data without transactions.
        * 
        */
        else if (INCOMING.GetMessage() == "headers")
        {
            std::vector<Core::CBlock> vBlocks;
            ssMessage >> vBlocks;
            
            
            if(GetArg("-verbose", 0) >= 1)
                printf("***** Recieved Message of %u Headers %u - %u\n", vBlocks.size(), vBlocks.front().nHeight, vBlocks.back().nHeight);
            
            
            /* Make sure it is not beyond limits */
            if (vBlocks.size() > 5000 || vBlocks.size() == 0)
            {
                DDOS->rSCORE += 20;
                printf("ERROR SIZE TOO LARGE\n");
                
                return true;
            }
            
            
            /* Add the list of new block headers into the block pool. */
            std::vector<CInv> vRequest;
            for(auto block : vBlocks)
                Core::pManager->blkPool.Add(block.GetHash(), block, Core::pManager->blkPool.HEADER);

            
            return true;
        }

        
        /* Get the Data for a Specific Command. 
        TODO: Expand this for the data types. 
        */
        else if (INCOMING.GetMessage() == "getdata")
        {
            std::vector<CInv> vInv;
            ssMessage >> vInv;
            if (vInv.size() > 10000)
            {
                DDOS->rSCORE += 20;
                
                return true;
            }
            
            
            for(int i = 0; i < vInv.size(); i++)
            {

                if (vInv[i].type == MSG_BLOCK)
                {
                    Core::CBlock block;
                    if(Core::pManager->blkPool.Get(vInv[i].hash, block)){
                        PushMessage("block", block);
                        
                        continue;
                    }
                            
                    std::map<uint1024, Core::CBlockIndex*>::iterator mi = Core::mapBlockIndex.find(vInv[i].hash);
                    if (mi != Core::mapBlockIndex.end())
                    {
                        if(!block.ReadFromDisk((*mi).second))
                            continue;
                            
                        PushMessage("block", block);
                    }
                }
                
                
                else if(vInv[i].type == MSG_TX)
                {
                    Core::CTransaction tx;
                    if(Core::pManager->txPool.Get(vInv[i].hash.getuint512(), tx))
                        PushMessage("tx", tx);
                    
                }
                
                
                /* Log Level 4: Get data protocol level messages. */
                if(GetArg("-verbose", 0) >= 4)
                    printf("received getdata for: %s\n", vInv[i].ToString().c_str());
            }
        }


        /* Handle a Request to get a list of Blocks from a Node. */
        else if (INCOMING.GetMessage() == "getblocks")
        {
            Core::CBlockLocator locator;
            uint1024 hashStop;
            ssMessage >> locator >> hashStop;

            /* Find the last block the caller has in the main chain */
            Core::CBlockIndex* pindex = locator.GetBlockIndex();

            /* Send the rest of the chain as INV messages. */
            if (pindex)
                pindex = pindex->pnext;
            
            int nLimit = 1000 + locator.GetDistanceBack();
            if(GetArg("-verbose", 0) >= 3)
                printf("***** Node Requested getblocks %d to %s limit %d\n", (pindex ? pindex->nHeight : -1), hashStop.ToString().substr(0,20).c_str(), nLimit);
                
            std::vector<CInv> vInv;
            for (; pindex; pindex = pindex->pnext)
            {
                if (pindex->GetBlockHash() == hashStop || vInv.size() >= nLimit || !pindex->pnext)
                    break;
                
                vInv.push_back(CInv(MSG_BLOCK, pindex->GetBlockHash()));
            }
            
            PushMessage("inv", vInv);
        }
        
        
        /* Handle a Request to get a list of Blocks from a Node. */
        else if (INCOMING.GetMessage() == "getheaders")
        {
            Core::CBlockLocator locator;
            uint1024 hashStop;
            ssMessage >> locator >> hashStop;

            /* Find the last block the caller has in the main chain */
            Core::CBlockIndex* pindex = locator.GetBlockIndex();

            /* Send the rest of the chain as INV messages. */
            if (pindex)
                pindex = pindex->pnext;
            
            int nLimit = 2000 + locator.GetDistanceBack();
            if(GetArg("-verbose", 0) >= 3)
                printf("***** Node Requested getheaders %d to %s limit %d\n", (pindex ? pindex->nHeight : -1), hashStop.ToString().substr(0,20).c_str(), nLimit);
                
            std::vector<Core::CBlock> vHeaders;
            for (; pindex; pindex = pindex->pnext)
            {
                if (pindex->GetBlockHash() == hashStop || vHeaders.size() >= nLimit || !pindex->pnext)
                    break;
                
                vHeaders.push_back(pindex->GetBlockHeader());
            }
            
            PushMessage("headers", vHeaders);
        }


        /* TODO: Change this Algorithm. */
        else if (INCOMING.GetMessage() == "getaddr")
        {
            std::vector<LLP::CAddress> vAddr = Core::pManager->GetAddresses();
            
            PushMessage("addr", vAddr);
        }


        return true;
    }
    
}
