/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People
____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLP_TYPES_MINER_H
#define NEXUS_LLP_TYPES_MINER_H

#include <LLP/templates/connection.h>
#include <TAO/Ledger/types/block.h>
#include <Legacy/types/coinbase.h>
#include <atomic>

namespace Legacy
{
    class ReserveKey;
}


namespace LLP
{

    /** Miner
     *
     *  Connection class that handles requests and responses from miners.
     *
     **/
    class Miner : public Connection
    {
        enum
        {
            /** DATA PACKETS **/
            BLOCK_DATA   = 0,
            SUBMIT_BLOCK = 1,
            BLOCK_HEIGHT = 2,
            SET_CHANNEL  = 3,
            BLOCK_REWARD = 4,
            SET_COINBASE = 5,
            GOOD_BLOCK   = 6,
            ORPHAN_BLOCK = 7,


            /** DATA REQUESTS **/
            CHECK_BLOCK  = 64,
            SUBSCRIBE    = 65,


            /** REQUEST PACKETS **/
            GET_BLOCK    = 129,
            GET_HEIGHT   = 130,
            GET_REWARD   = 131,


            /** SERVER COMMANDS **/
            CLEAR_MAP    = 132,
            GET_ROUND    = 133,


            /** RESPONSE PACKETS **/
            BLOCK_ACCEPTED       = 200,
            BLOCK_REJECTED       = 201,


            /** VALIDATION RESPONSES **/
            COINBASE_SET  = 202,
            COINBASE_FAIL = 203,

            /** ROUND VALIDATIONS. **/
            NEW_ROUND     = 204,
            OLD_ROUND     = 205,

            /** GENERIC **/
            PING     = 253,
            CLOSE    = 254
        };

    private:

        /* Externally set coinbase to be set on mined blocks */
        Legacy::Coinbase CoinbaseTx;


        /** Mutex for thread safe data access **/
        std::mutex MUTEX;


        /** The map to hold the list of blocks that are being mined. */
        std::map<uint512_t, TAO::Ledger::Block *> mapBlocks;


        /** The current best block. **/
        std::atomic<uint32_t> nBestHeight;


        /* Subscribe to display how many blocks connection subscribed to */
        std::atomic<uint32_t> nSubscribed;


        /* The current channel mining for. */
        std::atomic<uint32_t> nChannel;


        /* the mining key for block rewards to send */
        Legacy::ReserveKey *pMiningKey;


        /* The last txid on user's signature chain. Used to orphan blocks if another transaction is made while mining. */
        uint512_t nHashLast;


        /* The last height that the notifications processor was run at.  This is used to ensure that events are only processed once
           across all threads when the height changes */
        static std::atomic<uint32_t> nLastNotificationsHeight;


        /** Used as an ID iterator for generating unique hashes from same block transactions. **/
        static std::atomic<uint32_t> nBlockIterator;

    public:

        /** Default Constructor **/
        Miner();


        /** Constructor **/
        Miner(const Socket& SOCKET_IN, DDOS_Filter* DDOS_IN, bool fDDOSIn = false);


        /** Constructor **/
        Miner(DDOS_Filter* DDOS_IN, bool fDDOSIn = false);


        /** Default Destructor **/
        ~Miner();


        /** Name
         *
         *  Returns a string for the name of this type of Node.
         *
         **/
         static std::string Name()
         {
             return "Miner";
         }


        /** Event
         *
         *  Virtual Functions to Determine Behavior of Message LLP.
         *
         *  @param[in] EVENT The byte header of the event type.
         *  @param[in[ LENGTH The size of bytes read on packet read events.
         *
         */
        void Event(uint8_t EVENT, uint32_t LENGTH = 0) final;


        /** ProcessPacket
         *
         *  Main message handler once a packet is recieved.
         *
         *  @return True is no errors, false otherwise.
         *
         **/
        bool ProcessPacket() final;


    private:

        /** respond
         *
         *  Sends a packet response.
         *
         *  @param[in] header_response The header message to send.
         *  @param[in] data The packet data to send.
         *
         **/
        void respond(uint8_t nHeader, const std::vector<uint8_t>& vData = std::vector<uint8_t>());


        /** check_best_height
         *
         *  Checks the current height index and updates best height. Clears the block map if the height is outdated.
         *
         *  @return Returns true if best height was outdated, false otherwise.
         *
         **/
        bool check_best_height();


        /** check_round
         *
         *  For Tritium, this checks the mempool to make sure that there are no new transactions that would be orphaned by the
         *  the current round block.
         *
         *  @return Returns false if the current round is no longer valid.
         *
         **/
        bool check_round();


        /** clear_map
         *
         *  Clear the blocks map.
         *
         **/
        void clear_map();


        /** find_block
         *
         *  Determines if the block exists.
         *
         **/
        bool find_block(const uint512_t& hashMerkleRoot);


        /** new_block
         *
         *  Adds a new block to the map.
         *
         **/
        TAO::Ledger::Block *new_block();


        /** validate_block
         *
         *  validates the block for the derived miner class.
         *
         *  @param[in] hashMerkleRoot The root hash of the merkle tree.
         *
         *  @return Returns true if block is valid, false otherwise.
         *
         **/
        bool validate_block(const uint512_t& hashMerkleRoot);


        /** sign_block
         *
         *  signs the block to seal the proof of work.
         *
         *  @param[in] nNonce The nonce secret for the block proof.
         *  @param[in] hashMerkleRoot The root hash of the merkle tree.
         *
         *  @return Returns true if block is valid, false otherwise.
         *
         **/
        bool sign_block(uint64_t nNonce, const uint512_t& hashMerkleRoot);


        /** is_locked
         *
         *  Returns true if the mining wallet locked, false otherwise.
         *
         **/
        bool is_locked();


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

    };
}

#endif
