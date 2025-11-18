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
#include <TAO/Ledger/types/block.h>
#include <atomic>
#include <mutex>
#include <map>

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

        /** The map to hold the list of blocks that are being mined. **/
        std::map<uint512_t, TAO::Ledger::Block *> mapBlocks;

        /** Used as an ID iterator for generating unique hashes from same block transactions. **/
        static std::atomic<uint32_t> nBlockIterator;

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

        /** new_block
         *
         *  Adds a new block to the map.
         *
         *  @return Pointer to newly created block, or nullptr on failure.
         *
         **/
        TAO::Ledger::Block* new_block();

        /** find_block
         *
         *  Determines if the block exists.
         *
         *  @param[in] hashMerkleRoot The merkle root to search for.
         *
         *  @return True if block exists, false otherwise.
         *
         **/
        bool find_block(const uint512_t& hashMerkleRoot);

        /** sign_block
         *
         *  Signs the block to seal the proof of work.
         *
         *  @param[in] nNonce The nonce secret for the block proof.
         *  @param[in] hashMerkleRoot The root hash of the merkle tree.
         *
         *  @return True if block is valid, false otherwise.
         *
         **/
        bool sign_block(uint64_t nNonce, const uint512_t& hashMerkleRoot);

        /** validate_block
         *
         *  Validates the block for the derived miner class.
         *
         *  @param[in] hashMerkleRoot The root hash of the merkle tree.
         *
         *  @return Returns true if block is valid, false otherwise.
         *
         **/
        bool validate_block(const uint512_t& hashMerkleRoot);

        /** is_prime_mod
         *
         *  Helper function used for prime channel modification rule in loop.
         *  Returns true if the condition is satisfied, false otherwise.
         *
         *  @param[in] nBitMask The bitMask for the highest order bits of a block hash to check for to satisfy rule.
         *  @param[in] pBlock The block to check.
         *
         **/
        bool is_prime_mod(uint32_t nBitMask, TAO::Ledger::Block *pBlock);

        /** clear_map
         *
         *  Clear the blocks map.
         *
         **/
        void clear_map();
    };
}

#endif
