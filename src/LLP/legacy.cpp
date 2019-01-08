/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/


#include <LLC/include/random.h>

#include <LLP/include/hosts.h>
#include <LLP/include/inv.h>
#include <LLP/include/global.h>
#include <LLP/types/legacy.h>
#include <LLP/templates/events.h>

#include <Util/include/args.h>
#include <Util/include/hex.h>
#include <Util/include/debug.h>
#include <Util/include/runtime.h>

#include <TAO/Ledger/types/transaction.h>

#include <TAO/Ledger/include/chainstate.h>

namespace LLP
{

    /* Push a Message With Information about This Current Node. */
    void LegacyNode::PushVersion()
    {
        /* Random Session ID */
        RAND_bytes((uint8_t*)&nSessionID, sizeof(nSessionID));

        /* Current Unified timestamp. */
        int64_t nTime = runtime::unifiedtimestamp();

        /* Dummy Variable NOTE: Remove in Tritium ++ */
        uint64_t nLocalServices = 0;

        /* Relay Your Address. */
        Address addrMe  = Address(Service("0.0.0.0",0));
        Address addrYou = Address(Service("0.0.0.0",0));

        /* Push the Message to receiving node. */
        PushMessage("version", LLP::PROTOCOL_VERSION, nLocalServices, nTime, addrYou, addrMe,
                    nSessionID, strProtocolName, TAO::Ledger::ChainState::nBestHeight);
    }


    /** Handle Event Inheritance. **/
    void LegacyNode::Event(uint8_t EVENT, uint32_t LENGTH)
    {
        /** Handle any DDOS Packet Filters. **/
        if(EVENT == EVENT_HEADER)
        {
            debug::log(3, "***** Node recieved Message (", INCOMING.GetMessage(), ", ", INCOMING.LENGTH, ")");

            if(fDDOS)
            {

                /* Give higher DDOS score if the Node happens to try to send multiple version messages. */
                if (INCOMING.GetMessage() == "version" && nCurrentVersion != 0)
                    DDOS->rSCORE += 25;


                /* Check the Packet Sizes to Unified Time Commands. */
                if((INCOMING.GetMessage() == "getoffset" || INCOMING.GetMessage() == "offset") && INCOMING.LENGTH != 16)
                    DDOS->Ban(debug::strprintf("INVALID PACKET SIZE | OFFSET/GETOFFSET | LENGTH %u", INCOMING.LENGTH));
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

                    debug::log(3, "***** Dropped Packet (Complete: ", INCOMING.Complete() ? "Y" : "N",
                        " - Valid: )",  INCOMING.IsValid() ? "Y" : "N");

                    DDOS->rSCORE += 15;
                }

            }

            if(INCOMING.Complete())
            {
                debug::log(4, "***** Node Received Packet (", INCOMING.LENGTH, ", ", INCOMING.GetBytes().size(), ")");

                if(config::GetArg("-verbose", 0) >= 5)
                    PrintHex(INCOMING.GetBytes());
            }

            return;
        }


        /* Handle Node Pings on Generic Events */
        if(EVENT == EVENT_GENERIC)
        {

            if(nLastPing + 1 < runtime::unifiedtimestamp())
            {

                for(int i = 0; i < config::GetArg("-ping", 1); i++)
                {
                    RAND_bytes((uint8_t*)&nSessionID, sizeof(nSessionID));

                    nLastPing = runtime::unifiedtimestamp();

                    mapLatencyTracker.emplace(nSessionID, runtime::timer());
                    mapLatencyTracker[nSessionID].Start();

                    PushMessage("ping", nSessionID);
                }
            }

            //TODO: mapRequests data, if no response given retry the request at given times
        }


        /** On Connect Event, Assign the Proper Handle. **/
        if(EVENT == EVENT_CONNECT)
        {
            addrThisNode = addr;
            nLastPing    = runtime::unifiedtimestamp();

            debug::log(1, "***** ", fOUTGOING ? "Outgoing" : "Incoming", " Node ", addrThisNode.ToString(),
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

            if(LEGACY_SERVER && LEGACY_SERVER->pAddressManager)
                LEGACY_SERVER->pAddressManager->AddAddress(GetAddress(), ConnectState::DROPPED);

            debug::log(1, "xxxxx ", fOUTGOING ? "Outgoing" : "Incoming",
                " Node ", addrThisNode.ToString(),
                " Disconnected (", strReason, ") at timestamp ", runtime::unifiedtimestamp());

            return;
        }

    }


    /** This function is necessary for a template LLP server. It handles your
        custom messaging system, and how to interpret it from raw packets. **/
    bool LegacyNode::ProcessPacket()
    {

        DataStream ssMessage(INCOMING.DATA, SER_NETWORK, MIN_PROTO_VERSION);
        if(INCOMING.GetMessage() == "getoffset")
        {
            /* Don't service unified seeds unless time is unified. */
            //if(!Core::fTimeUnified)
            //    return true;

            /* De-Serialize the Request ID. */
            uint32_t nRequestID;
            ssMessage >> nRequestID;

            /* De-Serialize the timestamp Sent. */
            uint64_t nTimestamp;
            ssMessage >> nTimestamp;

            /* Log into the sent requests Map. */
            mapSentRequests[nRequestID] = runtime::unifiedtimestamp(true);

            /* Calculate the offset to current clock. */
            int   nOffset    = (int)(runtime::unifiedtimestamp(true) - nTimestamp);
            PushMessage("offset", nRequestID, runtime::unifiedtimestamp(true), nOffset);

            /* Verbose logging. */
            debug::log(3, "***** Node: Sent Offset ", nOffset,
                " | ", addrThisNode.ToString(), " | Unified ", runtime::unifiedtimestamp());
        }

        /* Recieve a Time Offset from this Node. */
        else if(INCOMING.GetMessage() == "offset")
        {

            /* De-Serialize the Request ID. */
            uint32_t nRequestID;
            ssMessage >> nRequestID;


            /* De-Serialize the timestamp Sent. */
            uint64_t nTimestamp;
            ssMessage >> nTimestamp;

            /* Handle the Request ID's. */
            //uint32_t nLatencyTime = (Core::runtime::unifiedtimestamp(true) - nTimestamp);


            /* Ignore Messages Recieved that weren't Requested. */
            if(!mapSentRequests.count(nRequestID))
            {
                DDOS->rSCORE += 5;

                debug::log(3, "***** Node (", addrThisNode.ToString(),
                    "): Invalid Request : Message Not Requested [", nRequestID, "][", nNodeLatency, " ms]");

                return true;
            }


            /* Reject Samples that are recieved 30 seconds after last check on this node. */
            if(runtime::unifiedtimestamp(true) - mapSentRequests[nRequestID] > 30000)
            {
                mapSentRequests.erase(nRequestID);

                debug::log(3, "***** Node (", addrThisNode.ToString(),
                    "): Invalid Request : Message Stale [", nRequestID, "][", nNodeLatency, " ms]");

                DDOS->rSCORE += 15;

                return true;
            }


            /* De-Serialize the Offset. */
            int nOffset;
            ssMessage >> nOffset;

            /* Adjust the Offset for Latency. */
            nOffset -= nNodeLatency;

            /* Add the Samples. */
            //setTimeSamples.insert(nOffset);

            /* Remove the Request from the Map. */
            mapSentRequests.erase(nRequestID);

            /* Verbose Logging. */
            debug::log(3, "***** Node (", addrThisNode.ToString(),
                "): Received Unified Offset ", nOffset, " [", nRequestID, "][", nNodeLatency, " ms]");
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
            TAO::Ledger::Transaction tx;
            ssMessage >> tx;

            tx.print();
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

            /* Set the latency used for address manager within server */
            if(LEGACY_SERVER && LEGACY_SERVER->pAddressManager)
                LEGACY_SERVER->pAddressManager->SetLatency(nLatency, GetAddress());

            /* Debug Level 3: output Node Latencies. */
            debug::log(3, "***** Node ", addrThisNode.ToString(), " Latency (Nonce ", nonce, " - ", nLatency, " ms)");
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
            Address addrMe;
            Address addrFrom;
            uint64_t nServices = 0;


            /* Check the Protocol Versions */
            ssMessage >> nCurrentVersion;


            /* Deserialize the rest of the data. */
            ssMessage >> nServices >> nTime >> addrMe >> addrFrom >> nSessionID >> strNodeVersion >> nStartingHeight;
            debug::log(1, "***** Node version message: version ", nCurrentVersion, ", blocks=",  nStartingHeight);


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
            std::vector<Address> vAddr;
            ssMessage >> vAddr;

            /* Don't want addr from older versions unless seeding */
            if (vAddr.size() > 2000)
            {
                DDOS->rSCORE += 20;

                return debug::error("***** Node message addr size() = ", vAddr.size(), "... Dropping Connection");
            }

            if(LEGACY_SERVER)
            {
                /* try to establish the connection on the port the server is listening to */
                for(auto it = vAddr.begin(); it != vAddr.end(); ++it)
                    it->SetPort(LEGACY_SERVER->PORT);

                /* Add the connections to Legacy Server. */
                if(LEGACY_SERVER->pAddressManager)
                    LEGACY_SERVER->pAddressManager->AddAddresses(vAddr);
            }

        }


        /* Handle new Inventory Messages.
        * This is used to know what other nodes have in their inventory to compare to our own.
        */
        else if (INCOMING.GetMessage() == "inv")
        {
            std::vector<CInv> vInv;
            ssMessage >> vInv;

            debug::log(1, "***** Inventory Message of ", vInv.size(), " elements");

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
            //std::vector<LLP::Address> vAddr = Core::pManager->GetAddresses();

            //PushMessage("addr", vAddr);
        }


        return true;
    }
}
