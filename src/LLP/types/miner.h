/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People
____________________________________________________________________________________________*/

#ifndef NEXUS_LLP_TYPES_MINER_H
#define NEXUS_LLP_TYPES_MINER_H

#include <LLP/templates/connection.h>
#include <TAO/Ledger/types/block.h>



namespace Legacy
{
    class ReserveKey;
    class LegacyBlock;
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
    private:
        /** The map to hold the list of blocks that are being mined. */
        std::map<uint512_t, Legacy::LegacyBlock> mapBlocks;

        /** the mining key for block rewards to send **/
        Legacy::ReserveKey *pMiningKey;

        /** block to get and iterate if requesting more than one block **/
        Legacy::LegacyBlock *pBaseBlock;

        /** The current best block. **/
        uint32_t nBestHeight;

        /** Subscribe to display how many blocks connection subscribed to **/
        uint16_t nSubscribed;

        /** The current channel mining for. */
        uint8_t nChannel;

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


    public:

        /** Name
         *
         *  Returns a string for the name of this type of Node.
         *
         **/
          static std::string Name() { return "Miner"; }


        /** Default Constructor **/
        Miner();


        /** Constructor **/
        Miner( Socket SOCKET_IN, DDOS_Filter* DDOS_IN, bool isDDOS = false );


        /** Default Destructor **/
        virtual ~Miner();


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


        /** SerializeBlock
         *
         *  Convert the Header of a Block into a Byte Stream for
         *  Reading and Writing Across Sockets.
         *
         *  @param[in] BLOCK A block to serialize.
         *
         **/
        std::vector<uint8_t> SerializeBlock(const TAO::Ledger::Block &BLOCK);

    private:

        /** respond
         *
         *  Sends a packet response.
         *
         *  @param[in] header_response The header message to send.
         *  @param[in] length The number of bytes for packet data.
         *  @param[in] data The packet data to send.
         *
         **/
        void respond(uint8_t header, uint32_t length = 0, const std::vector<uint8_t> &data = std::vector<uint8_t>());


        /** check_best_height
         *
         *  Checks the current height index and updates best height. It will clear
         *  the block map if the height is outdated.
         *
         *  @return Returns true if best height was outdated, false otherwise.
         *
         **/
         bool check_best_height();


         /** clear_map
          *
          *  Clear the blocks map.
          *
          **/
         void clear_map();

    };
}

#endif
