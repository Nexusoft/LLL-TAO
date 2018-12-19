/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/Operation/include/execute.h>

#include <LLC/include/random.h>

#include <LLD/include/global.h>

#include <LLP/include/global.h>
#include <LLP/types/tritium.h>

#include <Util/include/runtime.h>
#include <Util/include/args.h>
#include <Util/include/debug.h>

#include <climits>

namespace LLP
{

        /** Virtual Functions to Determine Behavior of Message LLP.
         *
         *  @param[in] EVENT The byte header of the event type
         *  @param[in[ LENGTH The size of bytes read on packet read events
         *
         */
        void TritiumNode::Event(uint8_t EVENT, uint32_t LENGTH)
        {
            switch(EVENT)
            {
                case EVENT_CONNECT:
                {
                    /* Setup the variables for this node. */
                    nLastPing    = runtime::Timestamp();

                    /* Debut output. */
                    debug::log(0, NODE "%s Connected at timestamp %" PRIu64 "", GetAddress().ToString().c_str(), runtime::UnifiedTimestamp());

                    /* Send version if making the connection. */
                    if(fOUTGOING)
                    {
                        uint64_t nSession = LLC::GetRand(std::numeric_limits<uint64_t>::max());
                        PushMessage(DAT_VERSION, nSession, GetAddress());
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
                    if(runtime::Timestamp() - nLastPing > 5)
                    {
                        /* Generate the nNonce. */
                        uint64_t nNonce = LLC::GetRand(std::numeric_limits<uint64_t>::max());

                        /* Add to latency tracker. */
                        mapLatencyTracker[nNonce] = runtime::Timestamp(true);

                        /* Push a ping request. */
                        PushMessage(DAT_PING, nNonce);

                        /* Update the last ping. */
                        nLastPing = runtime::Timestamp();
                    }

                    /* Generic events - unified time. */
                    if(runtime::Timestamp() - nLastSamples > 30)
                    {
                        /* Generate the request identification. */
                        uint32_t nRequestID = LLC::GetRand(std::numeric_limits<int32_t>::max());

                        /* Add sent requests. */
                        mapSentRequests[nRequestID] = runtime::Timestamp();

                        /* Request time samples. */
                        PushMessage(GET_OFFSET, nRequestID, runtime::Timestamp(true));

                        /* Update the samples timer. */
                        nLastSamples = runtime::Timestamp();
                    }

                    break;
                }

                case EVENT_DISCONNECT:
                {
                    if(TRITIUM_SERVER && TRITIUM_SERVER->pAddressManager)
                        TRITIUM_SERVER->pAddressManager->AddAddress(GetAddress(), ConnectState::DROPPED);

                    break;
                }
            }
        }


        /** Main message handler once a packet is recieved. **/
        bool TritiumNode::ProcessPacket()
        {

            CDataStream ssPacket(INCOMING.DATA, SER_NETWORK, PROTOCOL_VERSION);
            switch(INCOMING.MESSAGE)
            {

                case DAT_VERSION:
                {
                    /* Deserialize the session identifier. */
                    ssPacket >> nSessionID;

                    /* Get your address. */
                    Address addr;
                    ssPacket >> addr;

                    /* Check the server if it is set. */
                    if(!TRITIUM_SERVER->addrThisNode.IsValid())
                    {
                        addr.SetPort(config::GetArg("-port", config::fTestNet ? 8888 : 9888));
                        debug::log(1, NODE "recieved external address %s", addr.ToString().c_str());
                        TRITIUM_SERVER->addrThisNode = addr;
                    }

                    /* Send version message if connection is inbound. */
                    if(!fOUTGOING)
                    {
                        uint64_t nSession = LLC::GetRand(std::numeric_limits<uint64_t>::max());
                        PushMessage(DAT_VERSION, nSession, GetAddress());
                    }
                    else
                        PushMessage(GET_ADDRESSES);

                    /* Debug output for offsets. */
                    debug::log(3, NODE "received session identifier (%" PRIx64 ")", nSessionID);

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
                    int32_t nOffset = (runtime::Timestamp(true) - nTimestamp);

                    /* Debug output for offsets. */
                    debug::log(3, NODE "received timestamp of (%" PRIu64 ") - sending offset %i", nTimestamp, nOffset);

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
                    if(runtime::Timestamp() - mapSentRequests[nRequestID] > 10)
                    {
                        debug::log(0, NODE "offset is stale.");
                        mapSentRequests.erase(nRequestID);

                        break;
                    }

                    /* Deserialize the offset. */
                    int32_t nOffset;
                    ssPacket >> nOffset;

                    /* Debug output for offsets. */
                    debug::log(3, NODE "received offset %i", nOffset);

                    /* Remove sent requests from mpa. */
                    mapSentRequests.erase(nRequestID);

                    break;
                }


                case DAT_INVENTORY:
                {
                    /* Deserialize the data received. */
                    std::vector<uint512_t> vData;
                    ssPacket >> vData;

                    /* Request the inventory. */
                    for(auto hash : vData)
                        PushMessage(GET_INVENTORY, hash);

                    break;
                }


                case GET_INVENTORY:
                {
                    /* Deserialize the inventory. */
                    uint512_t hash;
                    ssPacket >> hash;

                    /* Check if you have it. */
                    TAO::Ledger::Transaction tx;
                    if(LLD::legDB->ReadTx(hash, tx))
                        PushMessage(DAT_TRANSACTION, tx);

                    break;
                }


                case DAT_TRANSACTION:
                {
                    /* Deserialize the tx. */
                    TAO::Ledger::Transaction tx;
                    ssPacket >> tx;

                    /* Check if we have it. */
                    if(!LLD::legDB->HasTx(tx.GetHash()))
                    {
                        /* Debug output for tx. */
                        debug::log(3, NODE "recieved tx %s", tx.GetHash().ToString().substr(0, 20).c_str());

                        /* Check if tx is valid. */
                        if(!tx.IsValid())
                        {
                            debug::error(NODE "tx %s REJECTED", tx.GetHash().ToString().substr(0, 20).c_str());

                            break;
                        }

                        /* Write the transaction to ledger database. */
                        if(!LLD::legDB->WriteTx(tx.GetHash(), tx))
                        {
                            debug::error(NODE "tx failed to write to disk");

                            break;
                        }

                        /* Process the transaction operations. */
                        if(!TAO::Operation::Execute(tx.vchLedgerData, tx.hashGenesis))
                        {
                            debug::error(NODE "tx failed to process register/operations");

                            break;
                        }
                    }

                    /* Debug output for offsets. */
                    debug::log(3, NODE "already have tx %s", tx.GetHash().ToString().substr(0, 20).c_str());

                    break;
                }


                case GET_ADDRESSES:
                {
                    /* Grab the connections. */
                    std::vector<Address> vAddr = TRITIUM_SERVER->GetAddresses();

                    /* Push the response addresses. */
                    PushMessage(DAT_ADDRESSES, vAddr);

                    break;
                }


                case DAT_ADDRESSES:
                {
                    /* De-Serialize the Addresses. */
                    std::vector<Address> vAddr;
                    ssPacket >> vAddr;

                    /* Add the connections to Tritium Server. */
                    if(TRITIUM_SERVER && TRITIUM_SERVER->pAddressManager)
                        TRITIUM_SERVER->pAddressManager->AddAddresses(vAddr);


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

                    uint32_t lat = runtime::Timestamp(true) - mapLatencyTracker[nNonce];

                    /* Set the latency used for address manager within server */
                    if(TRITIUM_SERVER && TRITIUM_SERVER->pAddressManager)
                        TRITIUM_SERVER->pAddressManager->SetLatency(lat, GetAddress());

                    /* Debug output for latency. */
                    debug::log(3, NODE "latency %u ms", lat);

                    /* Clear the latency tracker record. */
                    mapLatencyTracker.erase(nNonce);

                    break;
                }
            }

            return true;
        }

}
