/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLP_INCLUDE_PACKET_HANDLER_H
#define NEXUS_LLP_INCLUDE_PACKET_HANDLER_H

#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>
#include <memory>

#include <LLP/include/stateless_miner.h>
#include <LLP/packets/stateless_packet.h>

namespace LLP
{

    /** PacketHandler
     *
     *  Abstract base class for per-opcode packet handlers.
     *
     *  Each handler owns its own std::mutex, providing MUTEX isolation between
     *  opcode services.  This eliminates the problem where a single monolithic
     *  MUTEX serializes all packet types (auth, session, get_block, submit_block)
     *  against each other, causing unnecessary contention.
     *
     *  Design principles:
     *    - One handler per opcode (or small group of related opcodes)
     *    - Each handler has its own mutex — auth doesn't block keepalive
     *    - Pure virtual Handle() returns ProcessResult (immutable context pattern)
     *    - Handler name is used for logging and diagnostics
     *
     **/
    class PacketHandler
    {
    public:
        virtual ~PacketHandler() = default;

        /** Name
         *
         *  Human-readable name for this handler (used in logs).
         *
         **/
        virtual std::string Name() const = 0;

        /** Handle
         *
         *  Process a packet and return the result.
         *  Implementations should lock their own mutex internally when needed.
         *
         *  @param[in] context  Current mining context (immutable input).
         *  @param[in] packet   The incoming packet to process.
         *
         *  @return ProcessResult with updated context and optional response.
         *
         **/
        virtual ProcessResult Handle(
            const MiningContext& context,
            const StatelessPacket& packet) = 0;

    protected:

        /** handler_mutex
         *
         *  Per-handler mutex for thread safety.
         *  Subclasses should use std::lock_guard<std::mutex> lk(handler_mutex)
         *  in their Handle() implementation when accessing shared state.
         *
         **/
        mutable std::mutex handler_mutex;
    };


    /** PacketHandlerRegistry
     *
     *  Registry that maps opcodes to their handlers.
     *  Used by both LegacyLaneHandler and StatelessLaneHandler to dispatch
     *  packets to the correct per-opcode handler.
     *
     *  Thread-safe: handlers are registered at startup (single-threaded init)
     *  and only read during dispatch (no mutation after init).
     *
     **/
    class PacketHandlerRegistry
    {
    public:

        /** Register
         *
         *  Register a handler for a specific opcode.
         *
         *  @param[in] nOpcode   The opcode to handle.
         *  @param[in] pHandler  Shared pointer to the handler.
         *
         **/
        void Register(uint16_t nOpcode, std::shared_ptr<PacketHandler> pHandler)
        {
            mapHandlers[nOpcode] = std::move(pHandler);
        }

        /** Lookup
         *
         *  Find the handler for a given opcode.
         *
         *  @param[in] nOpcode  The opcode to look up.
         *
         *  @return Pointer to handler, or nullptr if not registered.
         *
         **/
        PacketHandler* Lookup(uint16_t nOpcode) const
        {
            auto it = mapHandlers.find(nOpcode);
            if(it != mapHandlers.end())
                return it->second.get();
            return nullptr;
        }

        /** HasHandler
         *
         *  Check if a handler is registered for the given opcode.
         *
         **/
        bool HasHandler(uint16_t nOpcode) const
        {
            return mapHandlers.find(nOpcode) != mapHandlers.end();
        }

    private:
        std::unordered_map<uint16_t, std::shared_ptr<PacketHandler>> mapHandlers;
    };


}  /* namespace LLP */

#endif /* NEXUS_LLP_INCLUDE_PACKET_HANDLER_H */
