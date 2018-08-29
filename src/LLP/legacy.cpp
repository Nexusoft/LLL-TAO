/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/


#include "include/hosts.h"
#include "include/legacy.h"
#include "include/inv.h"

#include "../LLC/include/random.h"

#include "../Util/include/args.h"
#include "../Util/include/hex.h"
#include "../Util/include/runtime.h"


#include "../TAO/Ledger/include/sigchain.h"


namespace LLP
{

    /* Push a Message With Information about This Current Node. */
    void LegacyNode::PushVersion()
    {
        /* Random Session ID */
        RAND_bytes((uint8_t*)&nSessionID, sizeof(nSessionID));

        /* Current Unified Timestamp. */
        int64_t nTime = UnifiedTimestamp();

        /* Dummy Variable NOTE: Remove in Tritium ++ */
        uint64_t nLocalServices = 0;

        /* Relay Your Address. */
        CAddress addrMe  = CAddress(CService("0.0.0.0",0));
        CAddress addrYou = CAddress(CService("0.0.0.0",0));

        uint32_t nBestHeight = 0; //TODO: Chain State Parameters (Ledger Layer)

        /* Push the Message to receiving node. */
        PushMessage("version", PROTOCOL_VERSION, nLocalServices, nTime, addrYou, addrMe,
                    nSessionID, strProtocolName, nBestHeight); //Core::nBestHeight);
    }


    /** Handle Event Inheritance. **/
    void LegacyNode::Event(uint8_t EVENT, uint32_t LENGTH)
    {
        /** Handle any DDOS Packet Filters. **/
        if(EVENT == EVENT_HEADER)
        {
            if(GetArg("-verbose", 0) >= 3)
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

            /* Check a packet's validity once it is finished being read. */
            if(fDDOS) {

                /* Give higher score for Bad Packets. */
                if(INCOMING.Complete() && !INCOMING.IsValid())
                {

                    if(GetArg("-verbose", 0) >= 3)
                        printf("***** Dropped Packet (Complete: %s - Valid: %s)\n", INCOMING.Complete() ? "Y" : "N" , INCOMING.IsValid() ? "Y" : "N" );

                    DDOS->rSCORE += 15;
                }

            }

            if(INCOMING.Complete())
            {
                if(GetArg("-verbose", 0) >= 4)
                    printf("***** Node Received Packet (%u, %u)\n", INCOMING.LENGTH, INCOMING.GetBytes().size());

                if(GetArg("-verbose", 0) >= 5) {
                    printf("***** Hex Message Dump\n");

                    PrintHex(INCOMING.GetBytes());
                }
            }

            return;
        }


        /* Handle Node Pings on Generic Events */
        if(EVENT == EVENT_GENERIC)
        {

            if(nLastPing + 1 < UnifiedTimestamp()) {

                for(int i = 0; i < GetArg("-ping", 1); i++)
                {
                    RAND_bytes((uint8_t*)&nSessionID, sizeof(nSessionID));

                    nLastPing = UnifiedTimestamp();

                    mapLatencyTracker.emplace(nSessionID, Timer());
                    mapLatencyTracker[nSessionID].Start();

                    PushMessage("ping", nSessionID);

                    if(fListen)
                        test->WriteSample(nSessionID, UnifiedTimestamp(true));
                }

                if(!fListen)
                {

                    TAO::Ledger::SignatureChain sigChain("username", "password");

                    TAO::Ledger::TritiumTransaction tx = TAO::Ledger::TritiumTransaction();
                    tx.nVersion    = 1;
                    tx.nTimestamp  = UnifiedTimestamp();
                    tx.NextHash(sigChain.Generate(nSessionID, "PIN"));
                    tx.hashGenesis = uint256_t(0);
                    tx.vchLedgerData = {0xff, 0xff, 0xff, 0xff, 0xfe, 0xfe};
                    tx.Sign(sigChain.Generate(nSessionID + 1, "PIN"));

                    //tx.print();
                    for(int i = 0; i < GetArg("-tx", 1); i++)
                    {
                        PushMessage("tritium", tx);
                    }
                }
            }

            //TODO: mapRequests data, if no response given retry the request at given times
        }


        /** On Connect Event, Assign the Proper Handle. **/
        if(EVENT == EVENT_CONNECT)
        {
            addrThisNode = SOCKET.addr;
            nLastPing    = UnifiedTimestamp();

            if(GetArg("-verbose", 0) >= 1)
                printf("***** %s Node %s Connected at Timestamp %" PRIu64 "\n", fOUTGOING ? "Ougoing" : "Incoming", addrThisNode.ToString().c_str(), UnifiedTimestamp());

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

                case DISCONNECT_ERRORS:
                    strReason = "Errors";
                    break;

                case DISCONNECT_DDOS:
                    strReason = "DDOS";
                    break;

                case DISCONNECT_FORCE:
                    strReason = "Forced";
                    break;
            }

            if(GetArg("-verbose", 0) >= 1)
                printf("xxxxx %s Node %s Disconnected (%s) at Timestamp %" PRIu64 "\n", fOUTGOING ? "Ougoing" : "Incoming", addrThisNode.ToString().c_str(), strReason.c_str(), UnifiedTimestamp());

            return;
        }

    }


    /** This function is necessary for a template LLP server. It handles your
        custom messaging system, and how to interpret it from raw packets. **/
    bool LegacyNode::ProcessPacket()
    {

        CDataStream ssMessage(INCOMING.DATA, SER_NETWORK, MIN_PROTO_VERSION);
        if(INCOMING.GetMessage() == "getoffset")
        {
            /* Don't service unified seeds unless time is unified. */
            //if(!Core::fTimeUnified)
            //    return true;

            /* De-Serialize the Request ID. */
            uint32_t nRequestID;
            ssMessage >> nRequestID;

            /* De-Serialize the Timestamp Sent. */
            uint64_t nTimestamp;
            ssMessage >> nTimestamp;

            /* Log into the sent requests Map. */
            mapSentRequests[nRequestID] = UnifiedTimestamp(true);

            /* Calculate the offset to current clock. */
            int   nOffset    = (int)(UnifiedTimestamp(true) - nTimestamp);
            PushMessage("offset", nRequestID, UnifiedTimestamp(true), nOffset);

            if(GetArg("-verbose", 0) >= 3)
                printf("***** Node: Sent Offset %i | %s | Unified %" PRIu64 "\n", nOffset, addrThisNode.ToString().c_str(), UnifiedTimestamp());

        }

        /* Recieve a Time Offset from this Node. */
        else if(INCOMING.GetMessage() == "offset")
        {

            /* De-Serialize the Request ID. */
            uint32_t nRequestID;
            ssMessage >> nRequestID;


            /* De-Serialize the Timestamp Sent. */
            uint64_t nTimestamp;
            ssMessage >> nTimestamp;

            /* Handle the Request ID's. */
            //uint32_t nLatencyTime = (Core::UnifiedTimestamp(true) - nTimestamp);


            /* Ignore Messages Recieved that weren't Requested. */
            if(!mapSentRequests.count(nRequestID)) {
                DDOS->rSCORE += 5;

                if(GetArg("-verbose", 0) >= 3)
                    printf("***** Node (%s): Invalid Request : Message Not Requested [%x][%u ms]\n", addrThisNode.ToString().c_str(), nRequestID, nNodeLatency);

                return true;
            }


            /* Reject Samples that are recieved 30 seconds after last check on this node. */
            if(UnifiedTimestamp(true) - mapSentRequests[nRequestID] > 30000) {
                mapSentRequests.erase(nRequestID);

                if(GetArg("-verbose", 0) >= 3)
                    printf("***** Node (%s): Invalid Request : Message Stale [%x][%u ms]\n", addrThisNode.ToString().c_str(), nRequestID, nNodeLatency);

                DDOS->rSCORE += 15;

                return true;
            }


            /* De-Serialize the Offset. */
            int nOffset;
            ssMessage >> nOffset;

            if(GetArg("-verbose", 0) >= 3)
                printf("***** Node (%s): Received Unified Offset %i [%x][%u ms]\n", addrThisNode.ToString().c_str(), nOffset, nRequestID, nNodeLatency);

            /* Adjust the Offset for Latency. */
            nOffset -= nNodeLatency;

            /* Add the Samples. */
            //setTimeSamples.insert(nOffset);

            /* Remove the Request from the Map. */
            mapSentRequests.erase(nRequestID);

        }


        /* Push a transaction into the Node's Recieved Transaction Queue. */
        else if (INCOMING.GetMessage() == "tx")
        {

            /* Deserialize the Transaction. */
            //Core::CTransaction tx;
            //ssMessage >> tx;


        }


        else if(INCOMING.GetMessage() == "tritium")
        {
            TAO::Ledger::TritiumTransaction tx;
            ssMessage >> tx;

            //tx.print();

            if(!tx.IsValid())
            {
                printf("Invalid tx %sw\n", tx.GetHash().ToString().c_str());

                return true;
            }

            if(fListen)
                test->WriteTransaction(GetRand512(), tx);
        }


        /* Push a block into the Node's Recieved Blocks Queue. */
        else if (INCOMING.GetMessage() == "block")
        {
            //Core::CBlock block;
            //ssMessage >> block;


        }


        /* Send a Ping with a nNonce to get Latency Calculations. */
        else if (INCOMING.GetMessage() == "ping")
        {
            uint64_t nonce = 0;
            ssMessage >> nonce;

            PushMessage("pong", nonce);
        }


        else if(INCOMING.GetMessage() == "pong")
        {
            uint64_t nonce = 0;
            ssMessage >> nonce;

            /* If the nonce was not received or known from pong. */
            if(!mapLatencyTracker.count(nonce))
            {
                DDOS->rSCORE += 5;
                return true;
            }

            /* Calculate the Average Latency of the Connection. */
            uint32_t nLatency = mapLatencyTracker[nonce].ElapsedMilliseconds();
            mapLatencyTracker.erase(nonce);

            uint64_t nTimestamp;
            if(fListen)
                test->ReadSample(nonce, nTimestamp);


            /* Debug Level 3: output Node Latencies. */
            if(GetArg("-verbose", 0) >= 3)
                printf("***** Node %s Latency (Nonce %" PRIu64 " - %u ms)\n", addrThisNode.ToString().c_str(), nonce, nLatency);
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

            int64_t nTime;
            CAddress addrMe;
            CAddress addrFrom;
            uint64_t nServices = 0;


            /* Check the Protocol Versions */
            ssMessage >> nCurrentVersion;


            /* Deserialize the rest of the data. */
            ssMessage >> nServices >> nTime >> addrMe >> addrFrom >> nSessionID >> strNodeVersion >> nStartingHeight;
            if(GetArg("-verbose", 0) >= 1)
                printf("***** Node version message: version %d, blocks=%d\n", nCurrentVersion, nStartingHeight);


            /* Send the Version Response to ensure communication channel is open. */
            PushMessage("verack");


            /* Push our version back since we just completed getting the version from the other node. */
            if (fOUTGOING)
            {

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

            PushMessage("getdata", vInv);
        }


        /* Handle block header inventory message
        *
        * This is just block data without transactions.
        *
        */
        else if (INCOMING.GetMessage() == "headers")
        {
            //std::vector<Core::CBlock> vBlocks;
            //ssMessage >> vBlocks;
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

        }


        /* Handle a Request to get a list of Blocks from a Node. */
        else if (INCOMING.GetMessage() == "getblocks")
        {
            //Core::CBlockLocator locator;
            //uint1024_t hashStop;
            //ssMessage >> locator >> hashStop;

        }


        /* Handle a Request to get a list of Blocks from a Node. */
        else if (INCOMING.GetMessage() == "getheaders")
        {
            //Core::CBlockLocator locator;
            //uint1024_t hashStop;
            //ssMessage >> locator >> hashStop;

        }


        /* TODO: Change this Algorithm. */
        else if (INCOMING.GetMessage() == "getaddr")
        {
            //std::vector<LLP::CAddress> vAddr = Core::pManager->GetAddresses();

            //PushMessage("addr", vAddr);
        }


        return true;
    }

}
