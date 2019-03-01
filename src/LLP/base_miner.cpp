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
                if(nSubscribed.load() == 0)
                    return;

                bool new_round = false;

                {
                  LOCK(MUTEX);
                  new_round = check_best_height();
                }

                if(!new_round)
                {
                    respond(NEW_ROUND);

                    uint1024_t block_hash;
                    std::vector<uint8_t> data;
                    TAO::Ledger::Block *pBlock = nullptr;
                    uint32_t len = 0;
                    uint16_t subscribed = nSubscribed.load();

                    for(uint16_t i = 0; i < subscribed; ++i)
                    {
                        {
                            LOCK(MUTEX);

                            /* Get a new block for the subscriber */
                            pBlock = new_block();

                            if(!pBlock)
                            {
                              debug::log(2, FUNCTION, "Failed to create block.");
                              return;
                            }


                            /* Serialize the block data */
                            data = SerializeBlock(*pBlock);
                            len = static_cast<uint32_t>(data.size());

                            /* Get the block hash for display purposes */
                            block_hash = pBlock->GetHash();
                        }

                        /* Create and send a packet response */
                        respond(BLOCK_DATA, len, data);


                        debug::log(2, FUNCTION, "Mining LLP: Sent Block ",
                            block_hash.ToString().substr(0, 20), " to Worker.");
                    }


                }
                return;
            }

            /* On Connect Event, Assign the Proper Daemon Handle. */
            case EVENT_CONNECT:
            {
                /* Debut output. */
                debug::log(2, FUNCTION, "Mining LLP: New Connection from ", GetAddress().ToStringIP());
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
                debug::log(2, FUNCTION, "Mining LLP: Disconnecting ", GetAddress().ToStringIP(), " (", strReason, ")");
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
            return debug::error(FUNCTION, " Cannot mine while ledger is synchronizing.");

        /* No mining when wallet is locked */
        if(is_locked())
            return debug::error(FUNCTION, " Cannot mine while wallet is locked.");


        /* Set the Mining Channel this Connection will Serve Blocks for. */
        switch(PACKET.HEADER)
        {
            case SET_CHANNEL:
            {
                nChannel = static_cast<uint8_t>(convert::bytes2uint(PACKET.DATA));

                switch (nChannel.load())
                {
                    case 1:
                    debug::log(2, FUNCTION, " Prime Channel Set.");
                    break;

                    case 2:
                    debug::log(2, FUNCTION, " Hash Channel Set.");
                    break;

                    /** Don't allow Mining LLP Requests for Proof of Stake Channel. **/
                    default:
                    return debug::error(2, FUNCTION, " Invalid PoW Channel (", nChannel.load(), ")");
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
                        debug::log(2, "***** Mining LLP: Invalid Address in Coinbase Tx: ", strAddress) ;
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
                    debug::log(2, "***** Mining LLP: Invalid Coinbase Tx") ;
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
                    debug::log(2, "***** Mining LLP: Coinbase Set") ;
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
                /* Create the response packet and write. */
                respond(BLOCK_HEIGHT, 4, convert::uint2bytes(nBestHeight + 1));

                /* Prevent mining clients from spamming with GET_HEIGHT messages*/
                runtime::sleep(500);

                LOCK(MUTEX);

                check_best_height();

                return true;
            }

            /* Respond to a miner if it is a new round. */
            case GET_ROUND:
            {

              bool new_round = false;

              {
                LOCK(MUTEX);
                new_round = check_best_height();
              }

                /* If height was outdated, respond with old round, otherwise
                 * respond with a new round */
                if(new_round)
                    respond(NEW_ROUND);
                else
                    respond(OLD_ROUND);

                return true;
            }

            case GET_REWARD:
            {
                uint64_t nCoinbaseReward = TAO::Ledger::GetCoinbaseReward(TAO::Ledger::ChainState::stateBest.load(), nChannel.load(), 0);

                respond(BLOCK_REWARD, 8, convert::uint2bytes64(nCoinbaseReward));

                debug::log(2, "***** Mining LLP: Sent Coinbase Reward of ", nCoinbaseReward);

                return true;
            }

            case SUBSCRIBE:
            {
                nSubscribed = static_cast<uint16_t>(convert::bytes2uint(PACKET.DATA));

                /** Don't allow mining llp requests for proof of stake channel **/
                if(nSubscribed == 0 || nChannel.load() == 0)
                    return false;

                debug::log(2, FUNCTION, "***** Mining LLP: Subscribed to ", nSubscribed, " Blocks");

                return true;
            }
            /* Get a new block for the miner. */
            case GET_BLOCK:
            {
                TAO::Ledger::Block *pBlock = nullptr;
                std::vector<uint8_t> data;
                uint32_t len = 0;

                {
                    LOCK(MUTEX);

                    check_best_height();

                    /* Create a new block */
                    pBlock = new_block();

                    if(!pBlock)
                    {
                      debug::log(2, FUNCTION, "Failed to create block.");
                      return true;
                    }

                    /* Store the new block in the memory map of recent blocks being worked on. */
                    mapBlocks[pBlock->hashMerkleRoot] = pBlock;

                    /* Serialize the block data */
                    data = SerializeBlock(*pBlock);
                    len = static_cast<uint32_t>(data.size());
                }

                /* Create and write the response packet. */
                respond(BLOCK_DATA, len, data);

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

                bool rejected = true;

                {
                  LOCK(MUTEX);

                  /* Find, sign, and validate the submitted block in order to
                     not be rejected. */
                  rejected = !find_block(hashMerkleRoot)
                          || !sign_block(nonce, hashMerkleRoot)
                          || !validate_block(hashMerkleRoot);
                }


                /* Generate a Rejected response. */
                if(rejected)
                {
                    respond(BLOCK_REJECTED);
                    return true;
                }

                {
                    LOCK(MUTEX);

                    /* Clear map on new block found. */
                    clear_map();
                    CoinbaseTx.SetNull();
                }

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
                if(LLD::legDB->ReadBlock(hashBlock, state))
                {
                    /*If the block state is in the main chain send a good response. */
                    if(state.IsInMainChain())
                    {
                        respond(GOOD_BLOCK, PACKET.LENGTH, PACKET.DATA);
                        return true;
                    }
                }

                /* Block state is not in the main chain, send an orphan response */
                respond(ORPHAN_BLOCK, PACKET.LENGTH, PACKET.DATA);
                return true;
            }
        }

        return false;
    }


    /*  Convert the Header of a Block into a Byte Stream for
     *  Reading and Writing Across Sockets. */
    std::vector<uint8_t> BaseMiner::SerializeBlock(const TAO::Ledger::Block &BLOCK)
    {
        std::vector<uint8_t> VERSION  = convert::uint2bytes(BLOCK.nVersion);
        std::vector<uint8_t> PREVIOUS = BLOCK.hashPrevBlock.GetBytes();
        std::vector<uint8_t> MERKLE   = BLOCK.hashMerkleRoot.GetBytes();
        std::vector<uint8_t> CHANNEL  = convert::uint2bytes(BLOCK.nChannel);
        std::vector<uint8_t> HEIGHT   = convert::uint2bytes(BLOCK.nHeight);
        std::vector<uint8_t> BITS     = convert::uint2bytes(BLOCK.nBits);
        std::vector<uint8_t> NONCE    = convert::uint2bytes64(BLOCK.nNonce);

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


    void BaseMiner::respond(uint8_t header_response, uint32_t length, const std::vector<uint8_t> &data)
    {
        Packet RESPONSE;

        RESPONSE.HEADER = header_response;
        RESPONSE.LENGTH = length;
        RESPONSE.DATA = data;

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
        for(auto it = mapBlocks.begin(); it != mapBlocks.end(); ++it)
        {
            if(it->second)
                delete it->second;
        }
        mapBlocks.clear();

        debug::log(2, FUNCTION, "Cleared map of blocks");
    }


    /*  Determines if the block exists. */
    bool BaseMiner::find_block(const uint512_t &merkle_root)
    {
        /* Check that the block exists. */
        if(!mapBlocks.count(merkle_root))
        {
            debug::log(2, FUNCTION, "***** Mining LLP: Block Not Found ", merkle_root.ToString().substr(0, 20));

            return false;
        }

        return true;
    }

}
