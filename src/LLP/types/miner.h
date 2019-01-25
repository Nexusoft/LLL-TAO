/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People
____________________________________________________________________________________________*/

#ifndef NEXUS_LLP_TYPES_MINER_H
#define NEXUS_LLP_TYPES_MINER_H

#include <LLP/templates/connection.h>

#include <TAO/Ledger/types/tritium.h>

namespace LLP
{

    /** Miner
     *
     *  Connection class that handles requests and responses from miners.
     *
     **/
    class Miner : public Connection
    {
    public:
        /** The map to hold the list of blocks that are being mined. */
        std::map<uint512_t, TAO::Ledger::TritiumBlock> mapBlocks;


        /** The current channel mining for. */
        uint32_t nChannel;


        /** The current best block. **/
        uint1024_t hashBestChain;

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

    //public:
        Miner()
        : Connection()
        , mapBlocks()
        , nChannel(0)
        , hashBestChain(0)
        {

        }

        Miner( Socket_t SOCKET_IN, DDOS_Filter* DDOS_IN, bool isDDOS = false )
        : Connection( SOCKET_IN, DDOS_IN )
        , mapBlocks()
        , nChannel(0)
        , hashBestChain(0)
        {

        }

        ~Miner()
        {

        }


        /** Clear
         *
         *  Clear the blocks map.
         *
         **/
        void Clear()
        {
            mapBlocks.clear();

            debug::log(2, FUNCTION, "new block, clearing maps.");
        }


        /** Event
         *
         *  Handle custom message events.
         *
         **/
        void Event(uint8_t EVENT, uint32_t LENGTH = 0) final;


        /** This function is necessary for a template LLP server. It handles your
            custom messaging system, and how to interpret it from raw packets. **/
        bool ProcessPacket() final;


        /** Convert the Header of a Block into a Byte Stream for Reading and Writing Across Sockets. **/
        std::vector<uint8_t> SerializeBlock(TAO::Ledger::TritiumBlock BLOCK);
    };
}

#endif
