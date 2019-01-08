/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People
____________________________________________________________________________________________*/


#include <LLP/types/miner.h>

#include <LLD/include/global.h>

#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/types/state.h>

#include <Util/include/convert.h>

namespace LLP
{

    /** Event
     *
     *  Handle custom message events.
     *
     **/
    void Miner::Event(uint8_t EVENT, uint32_t LENGTH)
    {
        /* Handle any DDOS Packet Filters. */
        if(EVENT == EVENT_HEADER)
        {
            if(fDDOS)
            {
                Packet PACKET   = this->INCOMING;
                if(PACKET.HEADER == BLOCK_DATA)
                    DDOS->Ban();

                if(PACKET.HEADER == SUBMIT_BLOCK && PACKET.LENGTH > 72)
                    DDOS->Ban();

                if(PACKET.HEADER == BLOCK_HEIGHT)
                    DDOS->Ban();

                if(PACKET.HEADER == SET_CHANNEL && PACKET.LENGTH > 4)
                    DDOS->Ban();

                if(PACKET.HEADER == BLOCK_REWARD)
                    DDOS->Ban();

                if(PACKET.HEADER == SET_COINBASE && PACKET.LENGTH > 20 * 1024)
                    DDOS->Ban();

                if(PACKET.HEADER == GOOD_BLOCK)
                    DDOS->Ban();

                if(PACKET.HEADER == ORPHAN_BLOCK)
                    DDOS->Ban();

                if(PACKET.HEADER == CHECK_BLOCK && PACKET.LENGTH > 128)
                    DDOS->Ban();

                if(PACKET.HEADER == SUBSCRIBE && PACKET.LENGTH > 4)
                    DDOS->Ban();

                if(PACKET.HEADER == BLOCK_ACCEPTED)
                    DDOS->Ban();

                if(PACKET.HEADER == BLOCK_REJECTED)
                    DDOS->Ban();

                if(PACKET.HEADER == COINBASE_SET)
                    DDOS->Ban();

                if(PACKET.HEADER == COINBASE_FAIL)
                    DDOS->Ban();

                if(PACKET.HEADER == NEW_ROUND)
                    DDOS->Ban();

                if(PACKET.HEADER == OLD_ROUND)
                    DDOS->Ban();

            }
        }


        /* Handle for a Packet Data Read. */
        if(EVENT == EVENT_PACKET)
            return;


        /* On Generic Event, Broadcast new block if flagged. */
        if(EVENT == EVENT_GENERIC)
            return;


        /* On Connect Event, Assign the Proper Daemon Handle. */
        if(EVENT == EVENT_CONNECT)
            return;

        /* On Disconnect Event, Reduce the Connection Count for Daemon */
        if(EVENT == EVENT_DISCONNECT)
            return;

    }


    /** This function is necessary for a template LLP server. It handles your
        custom messaging system, and how to interpret it from raw packets. **/
    bool Miner::ProcessPacket()
    {
        /* Get the incoming packet. */
        Packet PACKET   = this->INCOMING;

        /* No mining when synchronizing. */
        if(TAO::Ledger::ChainState::Synchronizing())
        {
            debug::error(FUNCTION "cannot mine while synchronizing");

            return false;
        }


        /* Set the Mining Channel this Connection will Serve Blocks for. */
        if(PACKET.HEADER == SET_CHANNEL)
        {
            nChannel = bytes2uint(PACKET.DATA);

            /** Don't allow Mining LLP Requests for Proof of Stake Channel. **/
            if(nChannel == 0)
                return false;

            if(config::GetArg("-verbose", 0) >= 2)
                printf("***** Mining LLP: Channel Set %u\n", nChannel);

            return true;
        }


        /* Return a Ping if Requested. */
        if(PACKET.HEADER == PING){ Packet PACKET; PACKET.HEADER = PING; this->WritePacket(PACKET); return true; }


        /* Clear the Block Map if Requested by Client. */
        if(PACKET.HEADER == CLEAR_MAP)
        {
            Clear();

            return true;
        }


        /* Respond to the miner with the new height. */
        if(PACKET.HEADER == GET_HEIGHT)
        {
            /* Create the response packet. */
            Packet RESPONSE;
            RESPONSE.HEADER = BLOCK_HEIGHT;
            RESPONSE.LENGTH = 4;
            RESPONSE.DATA   = uint2bytes(TAO::Ledger::ChainState::nBestHeight + 1);

            /* Write the response packet. */
            this->WritePacket(RESPONSE);

            /* Clear the Maps if Requested Height that is a New Best Block. */
            if(hashBestChain != TAO::Ledger::ChainState::hashBestChain)
            {
                Clear();

                hashBestChain = TAO::Ledger::ChainState::hashBestChain;
            }

            return true;
        }

        /* Respond to a miner if it is a new round. */
        if(PACKET.HEADER == GET_ROUND)
        {
            /* Create the response packet. */
            Packet RESPONSE;
            RESPONSE.HEADER = OLD_ROUND;

            /* Clear the Maps if Requested Height that is a New Best Block. */
            if(hashBestChain != TAO::Ledger::ChainState::hashBestChain)
            {
                Clear();

                hashBestChain = TAO::Ledger::ChainState::hashBestChain;
            }

            /* Write the response packet. */
            this->WritePacket(RESPONSE);

            return true;
        }


        /* Get a new block for the miner. */
        if(PACKET.HEADER == GET_BLOCK)
        {
            /* Create new block from base block by changing the input script to search from new merkle root. */
            TAO::Ledger::TritiumBlock block;// = Core::CreateNewBlock(*pMiningKey, pwalletMain, nChannel, 1, pCoinbaseTx);

            /* Store the new block in the memory map of recent blocks being worked on. */
            mapBlocks[block.hashMerkleRoot] = block;

            /* Construct a response packet by serializing the Block. */
            Packet RESPONSE;
            RESPONSE.HEADER = BLOCK_DATA;
            RESPONSE.DATA   = SerializeBlock(block);
            RESPONSE.LENGTH = RESPONSE.DATA.size();

            /* Write the response packet. */
            this->WritePacket(RESPONSE);

            return true;
        }


        /* Submit a block using the merkle root as the key. */
        if(PACKET.HEADER == SUBMIT_BLOCK)
        {
            /* Get the merkle root. */
            uint512_t hashMerkleRoot;
            hashMerkleRoot.SetBytes(std::vector<unsigned char>(PACKET.DATA.begin(), PACKET.DATA.end() - 8));

            /* Check that the block exists. */
            if(!mapBlocks.count(hashMerkleRoot))
            {
                /* If not found, send rejected message. */
                Packet RESPONSE;
                RESPONSE.HEADER = BLOCK_REJECTED;

                /* Write the response packet. */
                this->WritePacket(RESPONSE);

                /* Logging on verbose 2 */
                debug::log(2, FUNCTION "block not found %s\n", hashMerkleRoot.ToString().substr(0, 20).c_str());

                return true;
            }

            /* Create the pointer on the heap. */
            TAO::Ledger::TritiumBlock block = mapBlocks[hashMerkleRoot];
            block.nNonce = bytes2uint64(std::vector<unsigned char>(PACKET.DATA.end() - 8, PACKET.DATA.end()));
            block.UpdateTime();
            block.print();

            //validate the new block here.

            /* Generate a response message. */
            Packet RESPONSE;
            RESPONSE.HEADER = BLOCK_ACCEPTED;

            /* Clear map on new block found. */
            Clear();

            /* Write the response packet. */
            this->WritePacket(RESPONSE);

            return true;
        }


        /** Check Block Command: Allows Client to Check if a Block is part of the Main Chain. **/
        if(PACKET.HEADER == CHECK_BLOCK)
        {
            /* Extract the block hash. */
            uint1024_t hashBlock;
            hashBlock.SetBytes(PACKET.DATA);

            /* Create the response packet. */
            Packet RESPONSE;
            RESPONSE.LENGTH = PACKET.LENGTH;
            RESPONSE.DATA   = PACKET.DATA;

            /* Read the block state from disk. */
            TAO::Ledger::BlockState state;
            if(!LLD::legDB->ReadBlock(hashBlock, state))
                RESPONSE.HEADER = ORPHAN_BLOCK;
            else if(state.IsInMainChain())
                RESPONSE.HEADER = GOOD_BLOCK;

            /* Write the response packet. */
            this->WritePacket(RESPONSE);

            return true;
        }

        return false;
    }


    std::vector<uint8_t> Miner::SerializeBlock(TAO::Ledger::TritiumBlock BLOCK)
    {
        std::vector<uint8_t> VERSION  = uint2bytes(BLOCK.nVersion);
        std::vector<uint8_t> PREVIOUS = BLOCK.hashPrevBlock.GetBytes();
        std::vector<uint8_t> MERKLE   = BLOCK.hashMerkleRoot.GetBytes();
        std::vector<uint8_t> CHANNEL  = uint2bytes(BLOCK.nChannel);
        std::vector<uint8_t> HEIGHT   = uint2bytes(BLOCK.nHeight);
        std::vector<uint8_t> BITS     = uint2bytes(BLOCK.nBits);
        std::vector<uint8_t> NONCE    = uint2bytes64(BLOCK.nNonce);

        std::vector<uint8_t> DATA;
        DATA.insert(DATA.end(), VERSION.begin(),   VERSION.end());
        DATA.insert(DATA.end(), PREVIOUS.begin(), PREVIOUS.end());
        DATA.insert(DATA.end(), MERKLE.begin(),     MERKLE.end());
        DATA.insert(DATA.end(), CHANNEL.begin(),   CHANNEL.end());
        DATA.insert(DATA.end(), HEIGHT.begin(),     HEIGHT.end());
        DATA.insert(DATA.end(), BITS.begin(),         BITS.end());
        DATA.insert(DATA.end(), NONCE.begin(),       NONCE.end());

        return DATA;
    }

}
