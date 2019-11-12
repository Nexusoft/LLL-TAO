/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People
____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <LLP/include/global.h>
#include <LLP/types/miner.h>
#include <LLP/templates/events.h>
#include <LLP/templates/ddos.h>
#include <LLP/types/tritium.h>
#include <LLP/types/legacy.h>


#include <TAO/API/include/global.h>

#include <TAO/Ledger/include/difficulty.h>
#include <TAO/Ledger/include/create.h>
#include <TAO/Ledger/include/constants.h>
#include <TAO/Ledger/include/supply.h>
#include <TAO/Ledger/include/prime.h>
#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/include/timelocks.h>
#include <TAO/Ledger/include/process.h>
#include <TAO/Ledger/types/mempool.h>
#include <TAO/Ledger/types/sigchain.h>
#include <TAO/Ledger/types/transaction.h>
#include <TAO/Ledger/types/tritium.h>

#include <Legacy/include/create.h>
#include <Legacy/types/legacy.h>
#include <Legacy/wallet/wallet.h>
#include <Legacy/types/reservekey.h>

#include <Util/include/config.h>
#include <Util/include/convert.h>
#include <Util/include/args.h>


namespace LLP
{
    /* The last height that the notifications processor was run at.  This is used to ensure that events are only processed once
        across all threads when the height changes */
    std::atomic<uint32_t> Miner::nLastNotificationsHeight(0);


    /* The block iterator to act as extra nonce. */
    std::atomic<uint32_t> Miner::nBlockIterator(0);

    /* Default Constructor */
    Miner::Miner()
    : Connection()
    , CoinbaseTx()
    , mapBlocks()
    , nBestHeight(0)
    , nSubscribed(0)
    , nChannel(0)
    , pMiningKey(nullptr)
    , nHashLast(0)
    {
        pMiningKey = new Legacy::ReserveKey(&Legacy::Wallet::GetInstance());
    }


    /* Constructor */
    Miner::Miner(const Socket& SOCKET_IN, DDOS_Filter* DDOS_IN, bool isDDOS)
    : Connection(SOCKET_IN, DDOS_IN, isDDOS)
    , CoinbaseTx()
    , MUTEX()
    , mapBlocks()
    , nBestHeight(0)
    , nSubscribed(0)
    , nChannel(0)
    , pMiningKey(nullptr)
    , nHashLast(0)
    {
        pMiningKey = new Legacy::ReserveKey(&Legacy::Wallet::GetInstance());
    }


    /* Constructor */
    Miner::Miner(DDOS_Filter* DDOS_IN, bool isDDOS)
    : Connection(DDOS_IN, isDDOS)
    , CoinbaseTx()
    , MUTEX()
    , mapBlocks()
    , nBestHeight(0)
    , nSubscribed(0)
    , nChannel(0)
    , pMiningKey(nullptr)
    , nHashLast(0)
    {
        pMiningKey = new Legacy::ReserveKey(&Legacy::Wallet::GetInstance());
    }


    /* Default Destructor */
    Miner::~Miner()
    {
        LOCK(MUTEX);
        clear_map();

        if(pMiningKey)
        {
            pMiningKey->ReturnKey();
            delete pMiningKey;
        }

        /* Send a notification to wake up sleeping thread to finish shutdown process. */
        this->NotifyEvent();
    }


    /* Handle custom message events. */
    void Miner::Event(uint8_t EVENT, uint32_t LENGTH)
    {
        /* Handle any DDOS Packet Filters. */
        switch(EVENT)
        {
            /* Handle for a Packet Header Read. */
            case EVENT_HEADER:
            {
                if(DDOS && Incoming())
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
                uint32_t count = nSubscribed.load();
                if(count == 0)
                    return;

                /* Check for a new round. */
                {
                    LOCK(MUTEX);
                    if(check_best_height())
                        return;
                }

                /* Alert workers of new round. */
                respond(NEW_ROUND);

                uint1024_t hashBlock;
                std::vector<uint8_t> vData;
                TAO::Ledger::Block *pBlock = nullptr;

                for(uint32_t i = 0; i < count; ++i)
                {
                    {
                        LOCK(MUTEX);

                        /* Create a new block */
                        pBlock = new_block();

                        /* Handle if the block failed to be created. */
                        if(!pBlock)
                        {
                            debug::log(2, FUNCTION, "Failed to create block.");
                            return;
                        }

                        /* Store the new block in the memory map of recent blocks being worked on. */
                        mapBlocks[pBlock->hashMerkleRoot] = pBlock;

                        /* Serialize the block vData */
                        vData = pBlock->Serialize();

                        /* Get the block hash for display purposes */
                        hashBlock = pBlock->GetHash();
                    }

                    /* Create and send a packet response */
                    respond(BLOCK_DATA, vData);

                    /* Debug output. */
                    debug::log(2, FUNCTION, "Sent Block ", hashBlock.SubString(), " to Worker.");
                }
                return;
            }


            /* On Connect Event, Assign the Proper Daemon Handle. */
            case EVENT_CONNECT:
            {
                /* If version 7 or above then cache then cache the last transaction ID of the sig chain so that we can detect if
                   new transactions enter the mempool for this sig chain. */
                if(TAO::Ledger::VersionActive(runtime::unifiedtimestamp(), 7) || TAO::Ledger::CurrentVersion() > 7)
                    LLD::Ledger->ReadLast(TAO::API::users->GetGenesis(0), nHashLast, TAO::Ledger::FLAGS::MEMPOOL);

                /* Debug output. */
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


    /* This function is necessary for a template LLP server. It handles a custom message system, interpreting from raw packets. */
    bool Miner::ProcessPacket()
    {
        /* Get the incoming packet. */
        Packet PACKET = this->INCOMING;

        /* Make sure the mining server has a connection. (skip check if running local testnet) */
        bool fLocalTestnet = config::fTestNet.load() && !config::GetBoolArg("-dns", true);

        /* Total number of peer connections */
        uint16_t nConnections = (LEGACY_SERVER ? LEGACY_SERVER->GetConnectionCount() : 0)
                                + (TRITIUM_SERVER ? TRITIUM_SERVER->GetConnectionCount() : 0);

        if(!fLocalTestnet && nConnections == 0)
            return debug::error(FUNCTION, "No network connections.");

        /* No mining when synchronizing. */
        if(TAO::Ledger::ChainState::Synchronizing())
            return debug::error(FUNCTION, "Cannot mine while ledger is synchronizing.");

        /* No mining when wallet is locked */
        if(is_locked())
            return debug::error(FUNCTION, "Cannot mine while wallet is locked.");

        /* Obtain timelock and timestamp. */
        uint64_t nTimeLock = TAO::Ledger::CurrentTimelock();
        uint64_t nTimeStamp = runtime::unifiedtimestamp();

        /* Print a message explaining how many minutes until timelock activation. */
        if(nTimeStamp < nTimeLock)
        {
            uint64_t nSeconds =  nTimeLock - nTimeStamp;

            if(nSeconds % 60 == 0)
                debug::log(0, FUNCTION, "Timelock ", TAO::Ledger::CurrentVersion(), " activation in ", nSeconds / 60, " minutes. ");
        }

        /* Evaluate the packet header to determine what to do. */
        switch(PACKET.HEADER)
        {

            /* Set the Mining Channel this Connection will Serve Blocks for. */
            case SET_CHANNEL:
            {
                nChannel = convert::bytes2uint(PACKET.DATA);

                switch (nChannel.load())
                {
                    case 1:
                    debug::log(2, FUNCTION, "Prime Channel Set.");
                    break;

                    case 2:
                    debug::log(2, FUNCTION, "Hash Channel Set.");
                    break;

                    /* Don't allow Mining LLP Requests for Proof of Stake, or any other Channel. */
                    default:
                    return debug::error(FUNCTION, "Invalid PoW Channel (", nChannel.load(), ")");
                }

                return true;
            }


            /* Return a Ping if Requested. */
            case PING:
            {
                respond(PING);
                return true;
            }


            /* Setup a coinbase reward for potentially many outputs. */
            case SET_COINBASE:
            {
                /* The maximum coinbase reward for a block. */
                uint64_t nMaxValue = TAO::Ledger::GetCoinbaseReward(TAO::Ledger::ChainState::stateBest.load(), nChannel.load(), 0);

                /* Make sure there is a coinbase reward. */
                if(nMaxValue == 0)
                {
                    respond(COINBASE_FAIL);
                    return debug::error(FUNCTION, "Invalid coinbase reward.");
                }

                /* Byte 0 is the number of records. */
                uint8_t nSize = PACKET.DATA[0];

                /* Bytes 1 - 8 is the Wallet Operator Fee for that Round. */
                uint64_t nWalletFee  = convert::bytes2uint64(PACKET.DATA, 1);

                /* Iterator offset for map deserialization. */
                uint32_t nIterator = 9;

                /* The map of outputs for this coinbase transaction. */
                std::map<std::string, uint64_t> vOutputs;

                /* Loop through every Record. */
                for(uint8_t nIndex = 0; nIndex < nSize; ++nIndex)
                {
                    /* Get the string length. */
                    uint32_t nLength = PACKET.DATA[nIterator];

                    /* Get the string address for coinbase output. */
                    std::vector<uint8_t> vAddress = std::vector<uint8_t>(
                                             PACKET.DATA.begin() + nIterator + 1,
                                             PACKET.DATA.begin() + nIterator + 1 + nLength);

                    /* Get the value for the coinbase output. */
                    uint64_t nValue = convert::bytes2uint64(
                        std::vector<uint8_t>(PACKET.DATA.begin() + nIterator + 1 + nLength,
                                             PACKET.DATA.begin() + nIterator + 1 + nLength + 8));

                    /* Check value for coinbase output. */
                     if(nValue == 0 || nValue > nMaxValue)
                     {
                         respond(COINBASE_FAIL);
                         return debug::error(FUNCTION, "Invalid coinbase recipient reward.");
                     }

                     /* Get the string address. */
                     std::string strAddress = convert::bytes2string(vAddress);

                     /* Validate the address. Disconnect immediately if an invalid address is provided. */
                     if(TAO::Ledger::VersionActive(runtime::unifiedtimestamp(), 7) || TAO::Ledger::CurrentVersion() > 7)
                     {
                         uint256_t hashGenesis(strAddress);

                         if(!LLD::Ledger->HasGenesis(hashGenesis))
                         {
                             respond(COINBASE_FAIL);
                             return debug::error(FUNCTION, "Invalid Tritium Address in Coinbase Tx: ", strAddress);
                         }
                     }
                     else
                     {
                         Legacy::NexusAddress address;
                         address.SetPubKey(vAddress);

                         if(!address.IsValid())
                         {
                             respond(COINBASE_FAIL);
                             return debug::error(FUNCTION, "Invalid Legacy Address in Coinbase Tx: ", strAddress);
                         }
                     }


                    /* Add the transaction as an output. */
                    vOutputs[strAddress] = nValue;

                    /* Increment the iterator. */
                    nIterator += (nLength + 9);
                }

                /* Lock the coinbase transaction object. */
                LOCK(MUTEX);

                /* Update the coinbase transaction. */
                CoinbaseTx = Legacy::Coinbase(vOutputs, nMaxValue, nWalletFee);

                /* Check the consistency of the coibase transaction. */
                if(!CoinbaseTx.IsValid())
                {
                    CoinbaseTx.Print();
                    CoinbaseTx.SetNull();
                    respond(COINBASE_FAIL);
                    return debug::error(FUNCTION, "Invalid Coinbase Tx");
                }

                /* Send a coinbase set message. */
                respond(COINBASE_SET);

                /* Verbose output. */
                debug::log(2, FUNCTION, " Set Coinbase Reward of ", nMaxValue);
                if(config::GetArg("-verbose", 0 ) >= 3)
                    CoinbaseTx.Print();

                return true;
            }


            /* Clear the Block Map if Requested by Client. */
            case CLEAR_MAP:
            {
                LOCK(MUTEX);
                clear_map();
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

                /* Flag indicating the current round is no longer valid or there is a new block */
                bool fNewRound = false;
                {
                    LOCK(MUTEX);
                    fNewRound = !check_round() || check_best_height();
                }

                /* If height was outdated, respond with old round, otherwise respond with a new round */
                if(fNewRound)
                    respond(NEW_ROUND);
                else
                    respond(OLD_ROUND);

                return true;
            }


            /* Respond with the block reward in a given round. */
            case GET_REWARD:
            {
                /* Get the mining reward amount for the channel currently set. */
                uint64_t nReward = TAO::Ledger::GetCoinbaseReward(TAO::Ledger::ChainState::stateBest.load(), nChannel.load(), 0);

                /* Check to make sure the reward is greater than zero. */
                if(nReward == 0)
                    return debug::error(FUNCTION, "No coinbase reward.");


                /* Respond with BLOCK_REWARD message. */
                respond(BLOCK_REWARD, convert::uint2bytes64(nReward));

                /* Debug output. */
                debug::log(2, FUNCTION, "Sent Coinbase Reward of ", nReward);
                return true;
            }


            /* Set the number of subscribed blocks. */
            case SUBSCRIBE:
            {
                /* Don't allow mining llp requests for proof of stake channel */
                if(nChannel.load() == 0)
                    return debug::error(FUNCTION, "Cannot subscribe to Stake Channel.");

                /* Get the number of subscribed blocks. */
                nSubscribed = convert::bytes2uint(PACKET.DATA);

                /* Check for zero blocks. */
                if(nSubscribed.load() == 0)
                    return debug::error(FUNCTION, "No blocks subscribed.");

                /* Debug output. */
                debug::log(2, FUNCTION, "Subscribed to ", nSubscribed.load(), " Blocks");
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
                uint512_t hashMerkle;
                uint64_t nonce = 0;

                /* Get the merkle root. */
                hashMerkle.SetBytes(std::vector<uint8_t>(PACKET.DATA.begin(), PACKET.DATA.end() - 8));

                /* Get the nonce */
                nonce = convert::bytes2uint64(std::vector<uint8_t>(PACKET.DATA.end() - 8, PACKET.DATA.end()));

                LOCK(MUTEX);

                /* Make sure the block was created by this mining server. */
                if(!find_block(hashMerkle))
                {
                    respond(BLOCK_REJECTED);
                    return true;
                }

                /* Make sure there is no inconsistencies in signing block. */
                if(!sign_block(nonce, hashMerkle))
                {
                    respond(BLOCK_REJECTED);
                    return true;
                }

                /* Make sure there is no inconsistencies in validating block. */
                if(!validate_block(hashMerkle))
                {
                    respond(BLOCK_REJECTED);
                    return true;
                }

                /* Generate an Accepted response. */
                respond(BLOCK_ACCEPTED);
                return true;
            }


            /* Allows a client to check if a block is part of the main chain. */
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


    /* Sends a packet response. */
    void Miner::respond(uint8_t nHeader, const std::vector<uint8_t>& vData)
    {
        Packet RESPONSE;

        RESPONSE.HEADER = nHeader;
        RESPONSE.LENGTH = vData.size();
        RESPONSE.DATA   = vData;

        this->WritePacket(RESPONSE);
    }


    /* For Tritium, this checks the mempool to make sure that there are no new transactions that would be orphaned */
    bool Miner::check_round()
    {
        /* Only need to check the round for version 7 and above */
        if(TAO::Ledger::VersionActive(runtime::unifiedtimestamp(), 7) || TAO::Ledger::CurrentVersion() > 7)
        {
            /* Get the hash genesis. */
            uint256_t hashGenesis = TAO::API::users->GetGenesis(0);

            /* Read hashLast from hashGenesis' sigchain and also check mempool. */
            uint512_t hashLast;

            /* Check to see whether there are any new transactions in the mempool for the sig chain */
            if(TAO::Ledger::mempool.Has(hashGenesis))
            {
                /* Get the last hash of the last transaction created by the sig chain */
                LLD::Ledger->ReadLast(hashGenesis, hashLast, TAO::Ledger::FLAGS::MEMPOOL);

                /* Update nHashLast if it changed. */
                if(nHashLast != hashLast)
                {
                    nHashLast = hashLast;

                    clear_map();

                    debug::log(2, FUNCTION, "Block producer will orphan new sig chain transactions, resetting blocks");

                    return false;
                }
            }

        }

        return true;
    }


    /* Checks the current height index and updates best height. Clears the block map if the height is outdated or stale. */
    bool Miner::check_best_height()
    {
        uint32_t nChainStateHeight = TAO::Ledger::ChainState::nBestHeight.load();

        /* Introduced as part of Tritium upgrade. We can't rely on existing mining software to use the GET_ROUND to check that the
           the current round is still valid, so we additionally check the round whenever the height is checked.  If we find that it
           is not valid, the only way we can force miners to request new block data is to send through a height change. So here we
           set the height to 0 which will trigger miners to stop and request new block data, and then immediately the height will be
           set to the correct height again so they can carry on with the new block data.
           NOTE: there is no need to check the round if the height has changed as this obviously  will result in a new block*/
        if(nBestHeight == nChainStateHeight && !check_round())
        {
            /* Set the height temporarily to 0 */
            nBestHeight = 0;

            return true;
        }

        /* Return early if the height doesn't change. */
        if(nBestHeight == nChainStateHeight)
            return false;

        /* Clear the map of blocks if a new block has been accepted. */
        clear_map();

        /* Set the new best height. */
        nBestHeight = nChainStateHeight;
        debug::log(2, FUNCTION, "Mining best height changed to ", nBestHeight);

        /* make sure the notifications processor hasn't been run already at this height */
        if(nLastNotificationsHeight.load() != nBestHeight)
        {
            nLastNotificationsHeight.store(nBestHeight);

            /* Wake up events processor and wait for a signal to guarantee added transactions won't orphan a mined block. */
            if(TAO::API::users && TAO::API::users->CanProcessNotifications())
            {
                TAO::API::users->NotifyEvent();
                WaitEvent();
            }

            /* If we detected a block height change, update the cached last hash of the logged in sig chain.  NOTE this is done AFTER
            the notifications processor has finished, in case it added new transactions to the mempool  */
            if(TAO::Ledger::VersionActive(runtime::unifiedtimestamp(), 7) || TAO::Ledger::CurrentVersion() > 7)
                LLD::Ledger->ReadLast(TAO::API::users->GetGenesis(0), nHashLast, TAO::Ledger::FLAGS::MEMPOOL);
        }

        return true;
    }


    /*  Clear the blocks map. */
    void Miner::clear_map()
    {
        /* Delete the dynamically allocated blocks in the map. */
        for(auto &block : mapBlocks)
        {
            if(block.second)
                delete block.second;
        }
        mapBlocks.clear();

        /* Reset the coinbase transaction. */
        CoinbaseTx.SetNull();

        /* Set the block iterator back to zero so we can iterate new blocks next round. */
        nBlockIterator = 0;

        debug::log(2, FUNCTION, "Cleared map of blocks");
    }


    /*  Determines if the block exists. */
    bool Miner::find_block(const uint512_t& hashMerkleRoot)
    {
        /* Check that the block exists. */
        if(!mapBlocks.count(hashMerkleRoot))
        {
            debug::log(2, FUNCTION, "Block Not Found ", hashMerkleRoot.SubString());

            return false;
        }

        return true;
    }


    /*  Adds a new block to the map. */
   TAO::Ledger::Block *Miner::new_block()
   {
       /* If the primemod flag is set, take the hash proof down to 1017-bit to maximize prime ratio as much as possible. */
       uint32_t nBitMask = config::GetBoolArg(std::string("-primemod"), false) ? 0xFE000000 : 0x80000000;


       /* Create Tritium blocks if version 7 or above active. */
       if(TAO::Ledger::VersionActive(runtime::unifiedtimestamp(), 7) || TAO::Ledger::CurrentVersion() > 7)
       {
           /* Attempt to unlock the account. */
           if(TAO::API::users->Locked())
           {
               debug::error(FUNCTION, "No unlocked account available");
               return nullptr;
           }

           /* Get the sigchain and the PIN. */
           SecureString PIN = TAO::API::users->GetActivePin();

           /* Attempt to get the sigchain. */
           memory::encrypted_ptr<TAO::Ledger::SignatureChain>& pSigChain = TAO::API::users->GetAccount(0);
           if(!pSigChain)
           {
               debug::error(FUNCTION, "Couldn't get the unlocked sigchain");
               return nullptr;
           }

           /* Check that the account is unlocked for mining */
           if(!TAO::API::users->CanMine())
           {
               debug::error(FUNCTION, "Account has not been unlocked for mining");
               return nullptr;
           }

           /* Allocate memory for the new block. */
           TAO::Ledger::TritiumBlock *pBlock = new TAO::Ledger::TritiumBlock();

           /* Create a new block and loop for prime channel if minimum bit target length isn't met */
           while(TAO::Ledger::CreateBlock(pSigChain, PIN, nChannel.load(), *pBlock, ++nBlockIterator, &CoinbaseTx))
           {
               /* Break out of loop when block is ready for prime mod. */
               if(is_prime_mod(nBitMask, pBlock))
                   break;
           }

           /* Output debug info and return the newly created block. */
           debug::log(2, FUNCTION, "Created new Tritium Block ", pBlock->ProofHash().SubString(), " nVersion=", pBlock->nVersion);
           return pBlock;

       }

       /* Create Legacy blocks if version 7 not active. */
       Legacy::LegacyBlock *pBlock = new Legacy::LegacyBlock();

       /* Create a new block and loop for prime channel if minimum bit target length isn't met */
       while(Legacy::CreateBlock(*pMiningKey, CoinbaseTx, nChannel.load(), ++nBlockIterator, *pBlock))
       {
           /* Break out of loop when block is ready for prime mod. */
           if(is_prime_mod(nBitMask, pBlock))
                break;
       }

       /* Output debug info and return the newly created block. */
       debug::log(2, FUNCTION, "Created new Legacy Block ", pBlock->ProofHash().SubString(), " nVersion=", pBlock->nVersion);
       return pBlock;
   }


   /*  signs the block. */
  bool Miner::sign_block(uint64_t nNonce, const uint512_t& hashMerkleRoot)
  {

      TAO::Ledger::Block *pBaseBlock = mapBlocks[hashMerkleRoot];

      /* Update block with the nonce and time. */
      if(pBaseBlock)
          pBaseBlock->nNonce = nNonce;

      /* If the block dynamically casts to a legacy block, validate the legacy block. */
      {
          Legacy::LegacyBlock *pBlock = dynamic_cast<Legacy::LegacyBlock *>(pBaseBlock);
          if(pBlock)
          {
              /* Update the block's timestamp. */
              pBlock->UpdateTime();

              /* Sign the block with a key from wallet. */
              if(!Legacy::SignBlock(*pBlock, Legacy::Wallet::GetInstance()))
                  return debug::error(FUNCTION, "Unable to Sign Legacy Block ", hashMerkleRoot.SubString());

              return true;
          }
      }

      /* If the block dynamically casts to a tritium block, validate the tritium block. */
      TAO::Ledger::TritiumBlock *pBlock = dynamic_cast<TAO::Ledger::TritiumBlock *>(pBaseBlock);
      if(pBlock)
      {
          /* Update the block's timestamp. */
          pBlock->UpdateTime();

          /* Calculate prime offsets before signing. */
          TAO::Ledger::GetOffsets(pBlock->GetPrime(), pBlock->vOffsets);

          /* Check that the account is unlocked for minting */
          if(!TAO::API::users->CanMine())
              return debug::error(FUNCTION, "Account has not been unlocked for mining");

          /* Get the sigchain and the PIN. */
          SecureString PIN = TAO::API::users->GetActivePin();

          /* Attempt to get the sigchain. */
          memory::encrypted_ptr<TAO::Ledger::SignatureChain>& pSigChain = TAO::API::users->GetAccount(0);
          if(!pSigChain)
              return debug::error(FUNCTION, "Couldn't get the unlocked sigchain");

          /* Sign the submitted block */
          std::vector<uint8_t> vBytes = pSigChain->Generate(pBlock->producer.nSequence, PIN).GetBytes();
          LLC::CSecret vchSecret(vBytes.begin(), vBytes.end());

          /* Switch based on signature type. */
          switch(pBlock->producer.nKeyType)
          {
              /* Support for the FALCON signature scheeme. */
              case TAO::Ledger::SIGNATURE::FALCON:
              {
                  /* Create the FL Key object. */
                  LLC::FLKey key;

                  /* Set the secret parameter. */
                  if(!key.SetSecret(vchSecret))
                      return debug::error(FUNCTION, "FLKey::SetSecret failed for ", hashMerkleRoot.SubString());

                  /* Generate the signature. */
                  if(!pBlock->GenerateSignature(key))
                      return debug::error(FUNCTION, "GenerateSignature failed for Tritium Block ", hashMerkleRoot.SubString());

                  break;
              }

              /* Support for the BRAINPOOL signature scheme. */
              case TAO::Ledger::SIGNATURE::BRAINPOOL:
              {
                  /* Create EC Key object. */
                  LLC::ECKey key = LLC::ECKey(LLC::BRAINPOOL_P512_T1, 64);

                  /* Set the secret parameter. */
                  if(!key.SetSecret(vchSecret, true))
                      return debug::error(FUNCTION, "ECKey::SetSecret failed for ", hashMerkleRoot.SubString());

                  /* Generate the signature. */
                  if(!pBlock->GenerateSignature(key))
                      return debug::error(FUNCTION, "GenerateSignature failed for Tritium Block ", hashMerkleRoot.SubString());

                  break;
              }

              default:
                  return debug::error(FUNCTION, "Unknown signature type");
          }

          return true;

      }

      /* If we get here, the block is null or doesn't exist. */
      return debug::error(FUNCTION, "null block");
  }



    /*  validates the block. */
   bool Miner::validate_block(const uint512_t& hashMerkleRoot)
   {
       /* If the block dynamically casts to a legacy block, validate the legacy block. */
       {
           Legacy::LegacyBlock *pBlock = dynamic_cast<Legacy::LegacyBlock *>(mapBlocks[hashMerkleRoot]);

           if(pBlock)
           {
               debug::log(2, FUNCTION, "Legacy");
               pBlock->print();

               /* Check the Proof of Work for submitted block. */
               if(!Legacy::CheckWork(*pBlock, Legacy::Wallet::GetInstance()))
                   return false;

               /* Block is valid - Tell the wallet to keep this key. */
               pMiningKey->KeepKey();

               return true;
           }
       }

       /* If the block dynamically casts to a tritium block, validate the tritium block. */
       TAO::Ledger::TritiumBlock *pBlock = dynamic_cast<TAO::Ledger::TritiumBlock*>(mapBlocks[hashMerkleRoot]);
       if(pBlock)
       {
           debug::log(2, FUNCTION, "Tritium");
           pBlock->print();

           /* Log block found */
           if(config::nVerbose > 0)
           {
               std::string strTimestamp(convert::DateTimeStrFormat(runtime::unifiedtimestamp()));
               if(pBlock->nChannel == 1)
                   debug::log(1, FUNCTION, "new prime block found at unified time ", strTimestamp);
               else
                   debug::log(1, FUNCTION, "new hash block found at unified time ", strTimestamp);
           }

           //TODO: check if block will orphan any transactions

           /* Check if the block is stale. */
          // if(pBlock->hashPrevBlock != TAO::Ledger::ChainState::hashBestChain.load())
            //   return false;

           /* Attempt to get the sigchain. */
           memory::encrypted_ptr<TAO::Ledger::SignatureChain>& pSigChain = TAO::API::users->GetAccount(0);
           if(!pSigChain)
               return debug::error(FUNCTION, "Couldn't get the unlocked sigchain");

           /* Lock the sigchain that is being mined. */
           LOCK(TAO::API::users->CREATE_MUTEX);

           /* Process the block and relay to network if it gets accepted into main chain. */
           uint8_t nStatus = 0;
           TAO::Ledger::Process(*pBlock, nStatus);

           /* Check the statues. */
           if(!(nStatus & TAO::Ledger::PROCESS::ACCEPTED))
               return false;

           return true;
       }

       /* If we get here, the block is null or doesn't exist. */
       return false;
   }


    /*  Determines if the mining wallet is unlocked. */
   bool Miner::is_locked()
   {
       /* Check if mining should use tritium or legacy for wallet locked check. */
       if(TAO::Ledger::VersionActive(runtime::unifiedtimestamp(), 7) || TAO::Ledger::CurrentVersion() > 7)
            return TAO::API::users->Locked();

       return Legacy::Wallet::GetInstance().IsLocked();
   }


   /*  Helper function used for prime channel modification rule in loop.
    *  Returns true if the condition is satisfied, false otherwise. */
    bool Miner::is_prime_mod(uint32_t nBitMask, TAO::Ledger::Block *pBlock)
    {
        /* Get the proof hash. */
        uint1024_t hashProof = pBlock->ProofHash();

        /* Skip if not prime channel or version less than 5 */
        if(nChannel.load() != 1 || pBlock->nVersion < 5)
            return true;

         /* Exit loop when the block is above minimum prime origins and less than 1024-bit hashes */
         if(hashProof > TAO::Ledger::bnPrimeMinOrigins.getuint1024() && !hashProof.high_bits(nBitMask))
             return true;

        /* Otherwise keep looping. */
        return false;
    }


}
