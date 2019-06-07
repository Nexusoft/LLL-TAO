/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People
____________________________________________________________________________________________*/


#include <LLP/types/legacy_miner.h>
#include <LLP/templates/events.h>
#include <LLP/templates/ddos.h>

#include <LLD/include/global.h>

#include <TAO/Ledger/include/constants.h>
#include <TAO/Ledger/include/supply.h>
#include <TAO/Ledger/include/chainstate.h>

#include <Legacy/include/create.h>
#include <Legacy/types/legacy.h>
#include <Legacy/wallet/wallet.h>
#include <Legacy/wallet/reservekey.h>

#include <Util/include/convert.h>


namespace LLP
{

    /** Default Constructor **/
    BaseMiner::BaseMiner()
    : Connection()
    , CoinbaseTx()
    , MUTEX()
    , mapBlocks()
    , nBestHeight(0)
    , nSubscribed(0)
    , nChannel(0)
    , nBlockIterator(0)
    {
    }


    /** Constructor **/
    BaseMiner::BaseMiner(const Socket& SOCKET_IN, DDOS_Filter* DDOS_IN, bool isDDOS)
    : Connection(SOCKET_IN, DDOS_IN, isDDOS)
    , CoinbaseTx()
    , MUTEX()
    , mapBlocks()
    , nBestHeight(0)
    , nSubscribed(0)
    , nChannel(0)
    , nBlockIterator(0)
    {
    }


    /** Constructor **/
    BaseMiner::BaseMiner(DDOS_Filter* DDOS_IN, bool isDDOS)
    : Connection(DDOS_IN, isDDOS)
    , CoinbaseTx()
    , MUTEX()
    , mapBlocks()
    , nBestHeight(0)
    , nSubscribed(0)
    , nChannel(0)
    , nBlockIterator(0)
    {
    }


    /** Default Destructor **/
    BaseMiner::~BaseMiner()
    {
        LOCK(MUTEX);

        clear_map();
    }


    /** Event
     *
     *  Handle custom message events.
     *
     **/
    void BaseMiner::Event(uint8_t EVENT, uint32_t LENGTH)
    {

        /* Handle any DDOS Packet Filters. */
        switch(EVENT)
        {
            case EVENT_HEADER:
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
            case EVENT_PACKET:
                return;

            /* On Generic Event, Broadcast new block if flagged. */
            case EVENT_GENERIC:
            {
                /* On generic events, return if no workers subscribed. */
                if(nSubscribed.load() == 0)
                    return;

                /* Check for a new round. */
                bool fNewRound = false;
                {
                  LOCK(MUTEX);
                  fNewRound = check_best_height();
                }

                /* Push new message to workers on new round. */
                if(!fNewRound)
                {
                    /* Alert workers of new round. */
                    respond(NEW_ROUND);

                    uint1024_t hashBlock;
                    std::vector<uint8_t> vData;
                    TAO::Ledger::Block *pBlock = nullptr;

                    for(uint16_t i = 0; i < nSubscribed.load(); ++i)
                    {
                        {
                            LOCK(MUTEX);

                            /* Get a new block for the subscriber */
                            pBlock = new_block();

                            /* Handle if block cfeation failed. */
                            if(!pBlock)
                            {
                                debug::log(2, FUNCTION, "Failed to create block.");
                                return;
                            }

                            /* Serialize the block vData */
                            vData = pBlock->Serialize();

                            /* Get the block hash for display purposes */
                            hashBlock = pBlock->GetHash();
                        }

                        /* Create and send a packet response */
                        respond(BLOCK_DATA, vData);


                        debug::log(2, FUNCTION, "Sent Block ",
                            hashBlock.ToString().substr(0, 20), " to Worker.");
                    }


                }
                return;
            }

            /* On Connect Event, Assign the Proper Daemon Handle. */
            case EVENT_CONNECT:
            {
                /* Debut output. */
                debug::log(2, FUNCTION, "New Connection from ", GetAddress().ToStringIP());
                return;
            }


            /* On Disconnect Event, Reduce the Connection Count for Daemon */
            case EVENT_DISCONNECT:
            {
                /* Debut output. */
                uint32_t reason = LENGTH;
                std::string strReason;

                switch(reason)
                {
                    case DISCONNECT_TIMEOUT:
                        strReason = "DISCONNECT_TIMEOUT";
                        break;
                    case DISCONNECT_ERRORS:
                        strReason = "DISCONNECT_ERRORS";
                        break;
                    case DISCONNECT_DDOS:
                        strReason = "DISCONNECT_DDOS";
                        break;
                    case DISCONNECT_FORCE:
                        strReason = "DISCONNECT_FORCE";
                        break;
                    default:
                        strReason = "UNKNOWN";
                        break;
                }
                debug::log(2, FUNCTION, "Disconnecting ", GetAddress().ToStringIP(), " (", strReason, ")");
                return;
            }
        }

    }


    /** This function is necessary for a template LLP server. It handles your
        custom messaging system, and how to interpret it from raw packets. **/
    bool BaseMiner::ProcessPacket()
    {
        /* Get the incoming packet. */
        Packet PACKET   = this->INCOMING;

        /* TODO: add a check for the number of connections and return false if there
            are no connections */

        /* No mining when synchronizing. */
        if(TAO::Ledger::ChainState::Synchronizing())
            return debug::error(FUNCTION, "Cannot mine while ledger is synchronizing.");

        /* No mining when wallet is locked */
        if(is_locked())
            return debug::error(FUNCTION, "Cannot mine while wallet is locked.");


        /* Set the Mining Channel this Connection will Serve Blocks for. */
        switch(PACKET.HEADER)
        {
            case SET_CHANNEL:
            {
                nChannel = static_cast<uint8_t>(convert::bytes2uint(PACKET.DATA));

                switch (nChannel.load())
                {
                    case 1:
                    debug::log(2, FUNCTION, "Prime Channel Set.");
                    break;

                    case 2:
                    debug::log(2, FUNCTION, "Hash Channel Set.");
                    break;

                    /** Don't allow Mining LLP Requests for Proof of Stake Channel. **/
                    default:
                    return debug::error(2, FUNCTION, "Invalid PoW Channel (", nChannel.load(), ")");
                }

                return true;
            }

            /* Return a Ping if Requested. */
            case PING:
            {
                respond(PING);
                return true;
            }

            case SET_COINBASE:
            {
                uint64_t nMaxValue = TAO::Ledger::GetCoinbaseReward(TAO::Ledger::ChainState::stateBest.load(), nChannel.load(), 0);

                /** Deserialize the Coinbase Transaction. **/

                /** Bytes 1 - 8 is the Pool Fee for that Round. **/
                uint64_t nPoolFee  = convert::bytes2uint64(PACKET.DATA, 1);

                std::map<std::string, uint64_t> vOutputs;

                /** First byte of Serialization Packet is the Number of Records. **/
                uint32_t nSize = PACKET.DATA[0], nIterator = 9;

                /** Loop through every Record. **/
                for(uint32_t nIndex = 0; nIndex < nSize; ++nIndex)
                {
                    /** De-Serialize the Address String and uint64 nValue. **/
                    uint32_t nLength = PACKET.DATA[nIterator];

                    std::string strAddress = convert::bytes2string(std::vector<uint8_t>(PACKET.DATA.begin() + nIterator + 1, PACKET.DATA.begin() + nIterator + 1 + nLength));
                    uint64_t nValue = convert::bytes2uint64(std::vector<uint8_t>(PACKET.DATA.begin() + nIterator + 1 + nLength, PACKET.DATA.begin() + nIterator + 1 + nLength + 8));

                    /* Validate the address */
                    Legacy::NexusAddress address(strAddress);
                    if(!address.IsValid())
                    {
                        respond(COINBASE_FAIL);
                        debug::log(2, "Invalid Address in Coinbase Tx: ", strAddress) ;
                        return false; //disconnect immediately if an invalid address is provided
                    }
                    /** Add the Transaction as an Output. **/
                    vOutputs[strAddress] = nValue;

                    /** Increment the Iterator. **/
                    nIterator += (nLength + 9);
                }

                Legacy::Coinbase pCoinbase(vOutputs, nMaxValue, nPoolFee);

                if(!pCoinbase.IsValid())
                {
                    respond(COINBASE_FAIL);
                    debug::log(2, "Invalid Coinbase Tx") ;
                }
                else
                {
                    {
                        LOCK(MUTEX);

                        /* Set the global coinbase, null the base block, and
                           then call check_best_height which in turn will generate
                           a new base block using the new coinbase. */
                        CoinbaseTx = pCoinbase;

                        check_best_height();

                    }

                    respond(COINBASE_SET);
                    debug::log(2, "Coinbase Set") ;
                }

                return true;
            }

            /* Clear the Block Map if Requested by Client. */
            case CLEAR_MAP:
            {
                LOCK(MUTEX);

                clear_map();
                CoinbaseTx.SetNull();

                return true;
            }


            /* Respond to the miner with the new height. */
            case GET_HEIGHT:
            {
                {
                    /* Check the best height before responding. */
                    LOCK(MUTEX);
                    check_best_height();
                }

                /* Create the response packet and write. */
                respond(BLOCK_HEIGHT, convert::uint2bytes(nBestHeight + 1));

                return true;
            }

            /* Respond to a miner if it is a new round. */
            case GET_ROUND:
            {

                /* Check for a new round. */
                bool fNewRound = false;
                {
                    LOCK(MUTEX);
                    fNewRound = check_best_height();
                }

                /* If height was outdated, respond with old round, otherwise
                 * respond with a new round */
                if(fNewRound)
                    respond(NEW_ROUND);
                else
                    respond(OLD_ROUND);

                return true;
            }


            case GET_REWARD:
            {
                uint64_t nCoinbaseReward = TAO::Ledger::GetCoinbaseReward(TAO::Ledger::ChainState::stateBest.load(), nChannel.load(), 0);

                respond(BLOCK_REWARD, convert::uint2bytes64(nCoinbaseReward));

                debug::log(2, "***** Mining LLP: Sent Coinbase Reward of ", nCoinbaseReward);

                return true;
            }


            case SUBSCRIBE:
            {
                nSubscribed = static_cast<uint16_t>(convert::bytes2uint(PACKET.DATA));

                /** Don't allow mining llp requests for proof of stake channel **/
                if(nSubscribed == 0 || nChannel.load() == 0)
                    return false;

                debug::log(2, FUNCTION, "Subscribed to ", nSubscribed, " Blocks");

                return true;
            }


            /* Get a new block for the miner. */
            case GET_BLOCK:
            {
                TAO::Ledger::Block *pBlock = nullptr;

                /* Prepare the data to serialize on request. */
                std::vector<uint8_t> vData;
                {
                    LOCK(MUTEX);

                    /* Check for the best block height. */
                    check_best_height();

                    /* Create a new block */
                    pBlock = new_block();

                    /* Handle if the block failed to be created. */
                    if(!pBlock)
                    {
                        debug::log(2, FUNCTION, "Failed to create block.");
                        return true;
                    }

                    /* Store the new block in the memory map of recent blocks being worked on. */
                    mapBlocks[pBlock->hashMerkleRoot] = pBlock;

                    /* Serialize the block vData */
                    vData = pBlock->Serialize();
                }

                /* Create and write the response packet. */
                respond(BLOCK_DATA, vData);

                return true;
            }


            /* Submit a block using the merkle root as the key. */
            case SUBMIT_BLOCK:
            {
                uint512_t hashMerkleRoot;
                uint64_t nonce = 0;

                /* Get the merkle root. */
                hashMerkleRoot.SetBytes(std::vector<uint8_t>(PACKET.DATA.begin(), PACKET.DATA.end() - 8));

                /* Get the nonce */
                nonce = convert::bytes2uint64(std::vector<uint8_t>(PACKET.DATA.end() - 8, PACKET.DATA.end()));

                LOCK(MUTEX);

                /* Make sure the block was created by this mining server. */
                if(!find_block(hashMerkleRoot))
                {
                    respond(BLOCK_REJECTED);
                    return true;
                }

                /* Make sure there is no inconsistencies in signing block. */
                if(!sign_block(nonce, hashMerkleRoot))
                {
                    respond(BLOCK_REJECTED);
                    return true;
                }

                /* Make sure there is no inconsistencies in validating block. */
                if(!validate_block(hashMerkleRoot))
                {
                    respond(BLOCK_REJECTED);
                    return true;
                }

                /* Clear map on new block found. */
                clear_map();
                CoinbaseTx.SetNull();

                /* Generate an Accepted response. */
                respond(BLOCK_ACCEPTED);
                return true;

            }


            /** Check Block Command: Allows Client to Check if a Block is part of the Main Chain. **/
            case CHECK_BLOCK:
            {
                uint1024_t hashBlock;
                TAO::Ledger::BlockState state;

                /* Extract the block hash. */
                hashBlock.SetBytes(PACKET.DATA);

                /* Read the block state from disk. */
                if(LLD::Ledger->ReadBlock(hashBlock, state))
                {
                    /* If the block state is not in main chain, send a orphan response. */
                    if(!state.IsInMainChain())
                    {
                        respond(ORPHAN_BLOCK, PACKET.DATA);
                        return true;
                    }
                }

                /* Block state is in the main chain, send a good response */
                respond(GOOD_BLOCK, PACKET.DATA);
                return true;
            }
        }

        return false;
    }


    void BaseMiner::respond(uint8_t nHeader, const std::vector<uint8_t>& vData)
    {
        Packet RESPONSE;

        RESPONSE.HEADER = nHeader;
        RESPONSE.LENGTH = vData.size();
        RESPONSE.DATA   = vData;

        this->WritePacket(RESPONSE);
    }

    /*  Checks the current height index and updates best height. It will clear
     *  the block map if the height is outdated. */
    bool BaseMiner::check_best_height()
    {

        bool fHeightChanged = false;

        if(nBestHeight != TAO::Ledger::ChainState::nBestHeight)
        {
            clear_map();

            nBestHeight = TAO::Ledger::ChainState::nBestHeight.load();

            debug::log(2, FUNCTION, "Mining best height changed to ", nBestHeight);

            fHeightChanged = true;
        }

        return fHeightChanged;
    }


    /*  Clear the blocks map. */
    void BaseMiner::clear_map()
    {
        /* Delete the dynamically allocated blocks in the map. */
        for(auto it = mapBlocks.begin(); it != mapBlocks.end(); ++it)
        {
            if(it->second)
                delete it->second;
        }
        mapBlocks.clear();

        /* Set the block iterator back to zero so we can iterate new blocks next round. */
        nBlockIterator = 0;

        debug::log(2, FUNCTION, "Cleared map of blocks");
    }


    /*  Determines if the block exists. */
    bool BaseMiner::find_block(const uint512_t& hashMerkleRoot)
    {
        /* Check that the block exists. */
        if(!mapBlocks.count(hashMerkleRoot))
        {
            debug::log(2, FUNCTION, "Block Not Found ", hashMerkleRoot.ToString().substr(0, 20));

            return false;
        }

        return true;
    }

}
