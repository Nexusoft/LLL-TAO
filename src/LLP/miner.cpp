/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People
____________________________________________________________________________________________*/


#include <LLP/types/miner.h>
#include <LLP/templates/events.h>
#include <LLP/templates/ddos.h>

#include <LLD/include/global.h>

#include <TAO/Ledger/include/constants.h>

#include <Legacy/include/create.h>
#include <Legacy/types/legacy.h>
#include <Legacy/wallet/wallet.h>
#include <Legacy/wallet/reservekey.h>

#include <Util/include/convert.h>

namespace LLP
{

    /** Default Constructor **/
    Miner::Miner()
    : Connection()
    , mapBlocks()
    , pMiningKey(nullptr)
    , pBaseBlock(nullptr)
    , nBestHeight(0)
    , nSubscribed(0)
    , nChannel(0)
    {
        pBaseBlock = new Legacy::LegacyBlock();
        pMiningKey = new Legacy::ReserveKey(&Legacy::Wallet::GetInstance());
    }


    /** Constructor **/
    Miner::Miner(Socket SOCKET_IN, DDOS_Filter* DDOS_IN, bool isDDOS)
    : Connection(SOCKET_IN, DDOS_IN, isDDOS)
    , mapBlocks()
    , pMiningKey(nullptr)
    , pBaseBlock(nullptr)
    , nBestHeight(0)
    , nSubscribed(0)
    , nChannel(0)
    {
        pBaseBlock = new Legacy::LegacyBlock();
        pMiningKey = new Legacy::ReserveKey(&Legacy::Wallet::GetInstance());
    }


    /** Constructor **/
    Miner::Miner(DDOS_Filter* DDOS_IN, bool isDDOS)
    : Connection(DDOS_IN, isDDOS)
    , mapBlocks()
    , pMiningKey(nullptr)
    , pBaseBlock(nullptr)
    , nBestHeight(0)
    , nSubscribed(0)
    , nChannel(0)
    {
        pBaseBlock = new Legacy::LegacyBlock();
        pMiningKey = new Legacy::ReserveKey(&Legacy::Wallet::GetInstance());
    }
    

    /** Default Destructor **/
    Miner::~Miner()
    {
        if(pBaseBlock)
            delete pBaseBlock;

        if(pMiningKey)
            delete pMiningKey;
    }


    /** Event
     *
     *  Handle custom message events.
     *
     **/
    void Miner::Event(uint8_t EVENT, uint32_t LENGTH)
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
                if(nSubscribed == 0)
                    return;

                if(!check_best_height())
                {
                    respond(NEW_ROUND);

                    if(pBaseBlock->IsNull())
                        return;

                    uint1024_t proof_hash;

                    for(uint32_t nSubscribedLoop = 0; nSubscribedLoop < nSubscribed; ++nSubscribedLoop)
                    {
                        uint32_t s = static_cast<uint32_t>(mapBlocks.size());

                        /*  make a copy of the base block before making the hash  unique for this requst*/
                        Legacy::LegacyBlock new_block = *pBaseBlock;


                        /* We need to make the block hash unique for each subscribed miner so that they are not
                            duplicating their work.  To achieve this we take a copy of pBaseblock and then modify
                            the scriptSig to be unique for each subscriber, before rebuilding the merkle tree.

                            We need to drop into this for loop at least once to set the unique hash, but we will iterate
                            indefinitely for the prime channel until the generated hash meets the min prime origins
                            and is less than 1024 bits*/
                        for(uint32_t i = s; ; ++i)
                        {
                            new_block.vtx[0].vin[0].scriptSig = (Legacy::Script() <<  (uint64_t)((nSubscribedLoop + i + 1) * 513513512151));

                            /* Rebuild the merkle tree. */
                            std::vector<uint512_t> vMerkleTree;
                            for(const auto& tx : new_block.vtx)
                                vMerkleTree.push_back(tx.GetHash());
                            new_block.hashMerkleRoot = new_block.BuildMerkleTree(vMerkleTree);

                            /* Update the time. */
                            new_block.UpdateTime();

                            /* skip if not prime channel or version less than 5 */
                            if(nChannel != 1 || new_block.nVersion >= 5)
                                break;

                            proof_hash = new_block.ProofHash();

                            /* exit loop when the block is above minimum prime origins and less than
                                1024-bit hashes */
                            if(proof_hash > TAO::Ledger::bnPrimeMinOrigins.getuint1024() && !proof_hash.high_bits(0x80000000))
                                break;
                        }

                        /* Store the new block in the map */
                        mapBlocks[new_block.hashMerkleRoot] = new_block;

                        /* Serialize the block data */
                        std::vector<uint8_t> data = SerializeBlock(new_block);
                        uint32_t len = static_cast<uint32_t>(data.size());

                        /* Create and send a packet response */
                        respond(BLOCK_DATA, len, data);

                        debug::log(2, FUNCTION, "***** Mining LLP: Sent Block ",
                            new_block.GetHash().ToString().substr(0, 20), " to Worker.");
                    }


                }
                return;
            }

            /* On Connect Event, Assign the Proper Daemon Handle. */
            case EVENT_CONNECT:
                return;

            /* On Disconnect Event, Reduce the Connection Count for Daemon */
            case EVENT_DISCONNECT:
                return;
        }

    }


    /** This function is necessary for a template LLP server. It handles your
        custom messaging system, and how to interpret it from raw packets. **/
    bool Miner::ProcessPacket()
    {
        /* Get the incoming packet. */
        Packet PACKET   = this->INCOMING;

        /* TODO: add a check for the number of connections and return false if there
            are no connections */

        /* No mining when synchronizing. */
        if(TAO::Ledger::ChainState::Synchronizing())
            return debug::error(FUNCTION, Name(), " Cannot mine while ledger is synchronizing.");

        /* No mining when wallet is locked */
        if(Legacy::Wallet::GetInstance().IsLocked())
            return debug::error(FUNCTION, Name(), " Cannot mine while wallet is locked.");


        /* Set the Mining Channel this Connection will Serve Blocks for. */
        switch(PACKET.HEADER)
        {
            case SET_CHANNEL:
            {
                nChannel = static_cast<uint8_t>(bytes2uint(PACKET.DATA));

                switch (nChannel)
                {
                    case 1:
                    debug::log(2, FUNCTION, Name(), " Prime Channel Set.");
                    break;

                    case 2:
                    debug::log(2, FUNCTION, Name(), " Hash Channel Set.");
                    break;

                    /** Don't allow Mining LLP Requests for Proof of Stake Channel. **/
                    default:
                    return debug::error(2, FUNCTION, Name(), " Invalid PoW Channel (", nChannel, ")");
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
                //TODO: INTEGRATE THIS
                return true;
            }

            /* Clear the Block Map if Requested by Client. */
            case CLEAR_MAP:
            {
                clear_map();
                return true;
            }


            /* Respond to the miner with the new height. */
            case GET_HEIGHT:
            {

                /* Clear the Maps if Requested Height that is a New Best Block. */
                check_best_height();

                /* Create the response packet and write. */
                respond(BLOCK_HEIGHT, 4, uint2bytes(nBestHeight + 1));


                return true;
            }

            /* Respond to a miner if it is a new round. */
            case GET_ROUND:
            {
                /* If height was outdated, respond with old round, otherwise
                 * respond with a new round */
                if(!check_best_height())
                    respond(OLD_ROUND);
                else
                    respond(NEW_ROUND);

                return true;
            }

            case GET_REWARD:
            {
                uint64_t nCoinbaseReward = 0;

                //TODO: get the coinbase reward and return it here.

                respond(BLOCK_REWARD, 8, uint2bytes64(nCoinbaseReward));

                debug::log(2, "***** Mining LLP: Sent Coinbase Reward of ", nCoinbaseReward);

                return true;
            }

            case SUBSCRIBE:
            {
                nSubscribed = bytes2uint(PACKET.DATA);

                /** Don't allow mining llp requests for proof of stake channel **/
                if(nSubscribed == 0 || nChannel == 0)
                    return false;

                debug::log(2, FUNCTION, "***** Mining LLP: Subscribed to ", nSubscribed, " Blocks");

                return true;
            }
            /* Get a new block for the miner. */
            case GET_BLOCK:
            {

                uint1024_t proof_hash;
                uint32_t s = static_cast<uint32_t>(mapBlocks.size());

                /*  make a copy of the base block before making the hash  unique for this requst*/
                Legacy::LegacyBlock new_block = *pBaseBlock;


                /* We need to make the block hash unique for each subsribed miner so that they are not
                    duplicating their work.  To achieve this we take a copy of pBaseblock and then modify
                    the scriptSig to be unique for each subscriber, before rebuilding the merkle tree.

                    We need to drop into this for loop at least once to set the unique hash, but we will iterate
                    indefinitely for the prime channel until the generated hash meets the min prime origins
                    and is less than 1024 bits*/
                for(uint32_t i = s; ; ++i)
                {
                    new_block.vtx[0].vin[0].scriptSig = (Legacy::Script() <<  (uint64_t)((i+1) * 513513512151));

                    /* Rebuild the merkle tree. */
                    std::vector<uint512_t> vMerkleTree;
                    for(const auto& tx : new_block.vtx)
                        vMerkleTree.push_back(tx.GetHash());
                    new_block.hashMerkleRoot = new_block.BuildMerkleTree(vMerkleTree);

                    /* Update the time. */
                    new_block.UpdateTime();

                    /* skip if not prime channel or version less than 5 */
                    if(nChannel != 1 || new_block.nVersion >= 5)
                        break;

                    proof_hash = new_block.ProofHash();

                    /* exit loop when the block is above minimum prime origins and less than
                        1024-bit hashes */
                    if(proof_hash > TAO::Ledger::bnPrimeMinOrigins.getuint1024() && !proof_hash.high_bits(0x80000000))
                        break;
                }

                debug::log(2, FUNCTION, "***** Mining LLP: Created new Block ",
                    new_block.hashMerkleRoot.ToString().substr(0, 20));

                /* Store the new block in the memory map of recent blocks being worked on. */
                mapBlocks[new_block.hashMerkleRoot] = new_block;


                /* Serialize the block data */
                std::vector<uint8_t> data = SerializeBlock(new_block);
                uint32_t len = static_cast<uint32_t>(data.size());

                /* Create and write the response packet. */
                respond(BLOCK_DATA, len, data);

                return true;
            }

            /* Submit a block using the merkle root as the key. */
            case SUBMIT_BLOCK:
            {
                /* Get the merkle root. */
                uint512_t hashMerkleRoot;
                hashMerkleRoot.SetBytes(std::vector<uint8_t>(PACKET.DATA.begin(), PACKET.DATA.end() - 8));

                /* Check that the block exists. */
                if(!mapBlocks.count(hashMerkleRoot))
                {
                    /* If not found, send rejected message. */
                    respond(BLOCK_REJECTED);

                    debug::log(2, FUNCTION, "***** Mining LLP: Block Not Found ", hashMerkleRoot.ToString().substr(0, 20));

                    return true;
                }

                /* Create the pointer to the heap. */
                Legacy::LegacyBlock *pBlock = &mapBlocks[hashMerkleRoot];
                pBlock->nNonce = bytes2uint64(std::vector<uint8_t>(PACKET.DATA.end() - 8, PACKET.DATA.end()));
                pBlock->UpdateTime();
                pBlock->print();

                /* Sign the submitted block */
                if(!Legacy::SignBlock(*pBlock, Legacy::Wallet::GetInstance()))
                {
                    respond(BLOCK_REJECTED);

                    debug::log(2, "***** Mining LLP: Unable to Sign block ", hashMerkleRoot.ToString().substr(0, 20));

                    return true;
                }

                /* Check the Proof of Work for submitted block. */
                if(!Legacy::CheckWork(*pBlock, Legacy::Wallet::GetInstance()))
                {
                    respond(BLOCK_REJECTED);

                    debug::log(2, "***** Mining LLP: Invalid Work for block ", hashMerkleRoot.ToString().substr(0, 20));

                    return true;
                }

                /* Clear map on new block found. */
                check_best_height();

                /* Tell the wallet to keep this key */
                pMiningKey->KeepKey();

                /* Generate a response message. */
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
    std::vector<uint8_t> Miner::SerializeBlock(const TAO::Ledger::Block &BLOCK)
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


    void Miner::respond(uint8_t header_response, uint32_t length, const std::vector<uint8_t> &data)
    {
        Packet RESPONSE;

        RESPONSE.HEADER = header_response;
        RESPONSE.LENGTH = length;
        RESPONSE.DATA = data;

        this->WritePacket(RESPONSE);
    }

    /*  Checks the current height index and updates best height. It will clear
     *  the block map if the height is outdated. */
    bool Miner::check_best_height()
    {
        if(nBestHeight != TAO::Ledger::ChainState::nBestHeight || pBaseBlock->IsNull())
        {
            debug::log(2, FUNCTION, "Mining best height changed.  Creating new base block.");

            clear_map();
            nBestHeight = TAO::Ledger::ChainState::nBestHeight;

            /*create a new base block */
            if(!Legacy::CreateLegacyBlock(*pMiningKey, nChannel, 1, *pBaseBlock))
            {
                debug::error(FUNCTION, "Failed to create a new block.");
                return false;
            }

            return true;
        }

        return false;
    }


    /*  Clear the blocks map. */
    void Miner::clear_map()
    {
        mapBlocks.clear();

        debug::log(2, FUNCTION, "Cleared map of blocks");
    }

}
