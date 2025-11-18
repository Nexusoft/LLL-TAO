/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLP_TYPES_STATELESS_MINER_CONNECTION_H
#define NEXUS_LLP_TYPES_STATELESS_MINER_CONNECTION_H

#include <LLP/templates/connection.h>
#include <LLP/include/stateless_miner.h>
#include <atomic>
#include <mutex>

namespace LLP
{
    /** StatelessMinerConnection
     *
     *  Phase 2 stateless miner connection handler.
     *  Wraps the pure functional StatelessMiner processor in a Connection class.
     *
     *  This is the dedicated stateless miner LLP connection type for miningport.
     *  It uses MiningContext for immutable state management and routes all packets
     *  through StatelessMiner::ProcessPacket for pure functional processing.
     *
     *  PROTOCOL DESIGN:
     *  - Falcon authentication is mandatory before any mining operations
     *  - All state is managed through immutable MiningContext objects
     *  - Packet processing is stateless and returns ProcessResult with updated context
     *  - Compatible with NexusMiner Phase 2 protocol
     *
     **/
    class StatelessMinerConnection : public Connection
    {
    private:
        /** The current mining context (immutable snapshot) **/
        MiningContext context;

        /** Mutex for thread-safe context updates **/
        std::mutex MUTEX;

    public:
        /** Default Constructor **/
        StatelessMinerConnection();

        /** Constructor **/
        StatelessMinerConnection(const Socket& SOCKET_IN, DDOS_Filter* DDOS_IN, bool fDDOSIn = false);

        /** Constructor **/
        StatelessMinerConnection(DDOS_Filter* DDOS_IN, bool fDDOSIn = false);

        /** Default Destructor **/
        ~StatelessMinerConnection();

        /** Name
         *
         *  Returns a string for the name of this type of Node.
         *
         **/
        static std::string Name()
        {
            return "StatelessMiner";
        }

        /** Event
         *
         *  Virtual Functions to Determine Behavior of Message LLP.
         *
         *  @param[in] EVENT The byte header of the event type.
         *  @param[in] LENGTH The size of bytes read on packet read events.
         *
         */
        void Event(uint8_t EVENT, uint32_t LENGTH = 0) final;

        /** ProcessPacket
         *
         *  Main message handler once a packet is received.
         *
         *  @return True if no errors, false otherwise.
         *
         **/
        bool ProcessPacket() final;

    private:
        /** respond
         *
         *  Sends a packet response.
         *
         *  @param[in] packet The packet to send.
         *
         **/
        void respond(const Packet& packet);
    };
}

#endif
