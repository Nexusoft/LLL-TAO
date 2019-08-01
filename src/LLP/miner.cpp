/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People
____________________________________________________________________________________________*/

#include <LLD/include/global.h>

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
#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/include/timelocks.h>
#include <TAO/Ledger/types/mempool.h>
#include <TAO/Ledger/types/sigchain.h>
#include <TAO/Ledger/types/transaction.h>
#include <TAO/Ledger/types/tritium.h>

#include <Legacy/include/create.h>
#include <Legacy/types/legacy.h>
#include <Legacy/wallet/wallet.h>
#include <Legacy/wallet/reservekey.h>

#include <Util/include/config.h>
#include <Util/include/convert.h>
#include <Util/include/args.h>


namespace LLP
{

    /** Default Constructor **/
    Miner::Miner()
    : Connection()
    , CoinbaseTx()
    , MUTEX()
    , mapBlocks()
    , nBestHeight(0)
    , nSubscribed(0)
    , nChannel(0)
    , nBlockIterator(0)
    , pMiningKey(nullptr)
    {
        pMiningKey = new Legacy::ReserveKey(&Legacy::Wallet::GetInstance());
    }


    /** Constructor **/
    Miner::Miner(const Socket& SOCKET_IN, DDOS_Filter* DDOS_IN, bool isDDOS)
    : Connection(SOCKET_IN, DDOS_IN, isDDOS)
    , CoinbaseTx()
    , MUTEX()
    , mapBlocks()
    , nBestHeight(0)
    , nSubscribed(0)
    , nChannel(0)
    , nBlockIterator(0)
    , pMiningKey(nullptr)
    {
        pMiningKey = new Legacy::ReserveKey(&Legacy::Wallet::GetInstance());
    }


    /** Constructor **/
    Miner::Miner(DDOS_Filter* DDOS_IN, bool isDDOS)
    : Connection(DDOS_IN, isDDOS)
    , CoinbaseTx()
    , MUTEX()
    , mapBlocks()
    , nBestHeight(0)
    , nSubscribed(0)
    , nChannel(0)
    , nBlockIterator(0)
    , pMiningKey(nullptr)
    {
        pMiningKey = new Legacy::ReserveKey(&Legacy::Wallet::GetInstance());
    }


    /** Default Destructor **/
    Miner::~Miner()
    {
        LOCK(MUTEX);
        clear_map();

        if(pMiningKey)
        {
            pMiningKey->ReturnKey();
            delete pMiningKey;
        }
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
                /* On generic events, return if no workers subscribed. */
                uint16_t count = nSubscribed.load();
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

                for(uint16_t i = 0; i < count; ++i)
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


    /** This function is necessary for a template LLP server. It handles your
        custom messaging system, and how to interpret it from raw packets. **/
    bool Miner::ProcessPacket()
    {
        /* Get the incoming packet. */
        Packet PACKET = this->INCOMING;

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
                /* The maximum coinbase reward for a block. */
                uint64_t nMaxValue = TAO::Ledger::GetCoinbaseReward(TAO::Ledger::ChainState::stateBest.load(), nChannel.load(), 0);

                /* Bytes 1 - 8 is the Pool Fee for that Round. */
                uint64_t nPoolFee  = convert::bytes2uint64(PACKET.DATA, 1);

                /* The map of outputs for this coinbase transaction. */
                std::map<std::string, uint64_t> vOutputs;

                /* First byte of Serialization Packet is the Number of Records. */
                uint32_t nIterator = 9;
                uint8_t nSize = PACKET.DATA[0];

                /* Loop through every Record. */
                for(uint8_t nIndex = 0; nIndex < nSize; ++nIndex)
                {
                    /* Get the length. */
                    uint32_t nLength = PACKET.DATA[nIterator];

                    /* Get the string address for coinbase output. */
                    std::string strAddress = convert::bytes2string(
                        std::vector<uint8_t>(PACKET.DATA.begin() + nIterator + 1,
                                             PACKET.DATA.begin() + nIterator + 1 + nLength));

                    /* Get the value for the coinbase output. */
                    uint64_t nValue = convert::bytes2uint64(
                        std::vector<uint8_t>(PACKET.DATA.begin() + nIterator + 1 + nLength,
                                             PACKET.DATA.begin() + nIterator + 1 + nLength + 8));

                    /* Validate the address */
                    Legacy::NexusAddress address(strAddress);
                    if(!address.IsValid())
                    {
                        /* Disconnect immediately if an invalid address is provided. */
                        respond(COINBASE_FAIL);
                        return debug::error(FUNCTION, "Invalid Address in Coinbase Tx: ", strAddress);
                    }

                    /* Add the transaction as an output. */
                    vOutputs[strAddress] = nValue;

                    /* Increment the iterator. */
                    nIterator += (nLength + 9);
                }

                /* Update the coinbase transaction. */
                CoinbaseTx = Legacy::Coinbase(vOutputs, nMaxValue, nPoolFee);

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
                debug::log(2, "Coinbase Set");

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
                /* Check for the best block height. */
                {
                    LOCK(MUTEX);
                    check_best_height();
                }

                /* Get the mining reward amount for the channel currently set. */
                uint64_t nReward = TAO::Ledger::GetCoinbaseReward(TAO::Ledger::ChainState::stateBest.load(), nChannel.load(), 0);

                /* Respond with BLOCK_REWARD message. */
                respond(BLOCK_REWARD, convert::uint2bytes64(nReward));

                /* Debug output. */
                debug::log(2, FUNCTION, "Sent Coinbase Reward of ", nReward);
                return true;
            }


            case SUBSCRIBE:
            {
                nSubscribed = static_cast<uint16_t>(convert::bytes2uint(PACKET.DATA));

                /** Don't allow mining llp requests for proof of stake channel **/
                if(nSubscribed == 0 || nChannel.load() == 0)
                    return false;

                /* Debug output. */
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
                    /* Set best height to 0 to force miners to request new blocks, since we know that the current block
                       will fail to be submitted */
                    nBestHeight = 0;

                    respond(BLOCK_REJECTED);

                    return true;
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


    void Miner::respond(uint8_t nHeader, const std::vector<uint8_t>& vData)
    {
        Packet RESPONSE;

        RESPONSE.HEADER = nHeader;
        RESPONSE.LENGTH = vData.size();
        RESPONSE.DATA   = vData;

        this->WritePacket(RESPONSE);
    }


    /*  Checks the current height index and updates best height. It will clear the block map if the height is outdated. */
    bool Miner::check_best_height()
    {

        if(nBestHeight == TAO::Ledger::ChainState::nBestHeight)
            return false;

        /* Clear the map of blocks if a new block has been accepted. */
        clear_map();
        CoinbaseTx.SetNull();

        /* Set the new best height. */
        nBestHeight = TAO::Ledger::ChainState::nBestHeight.load();
        debug::log(2, FUNCTION, "Mining best height changed to ", nBestHeight);

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
       /*  Make a copy of the base block before making the hash unique for this request*/
       uint1024_t hashProof = 0;

       /* If the primemod flag is set, take the hashProof down to 1017-bit to maximize prime ratio as much as possible. */
       uint32_t nBitMask = config::GetBoolArg(std::string("-primemod"), false) ? 0xFE000000 : 0x80000000;

       /* Base class block pointer to derived class block. */
       TAO::Ledger::Block *pBlock = nullptr;

       /* Create Tritium blocks if version 7 active. */
       if(TAO::Ledger::VersionActive(runtime::unifiedtimestamp(), 7))
       {
           /* Create a new Tritium Block. */
           pBlock = new TAO::Ledger::TritiumBlock();

           /* Set it to a null state */
           pBlock->SetNull();

           /* Get the sigchain and the PIN. */
           SecureString PIN;

           /* Attempt to unlock the account. */
           if(TAO::API::users->Locked())
           {
               debug::error(FUNCTION, "No unlocked account available");
               return nullptr;
           }
           else
               PIN = TAO::API::users->GetActivePin();

           /* Attempt to get the sigchain. */
           memory::encrypted_ptr<TAO::Ledger::SignatureChain>& pSigChain = TAO::API::users->GetAccount(0);
           if(!pSigChain)
           {
               debug::error(FUNCTION, "Couldn't get the unlocked sigchain");
               return nullptr;
           }

           /* Check that the account is unlocked for minting */
           if(!TAO::API::users->CanMint())
           {
               debug::error(FUNCTION, "Account has not been unlocked for minting");
               return nullptr;
           }

           /* Create a new block and loop for prime channel if minimum bit target length isn't met */
           while(true)
           {
               /* Create the Tritium block with the corresponding sigchain and pin. */
               if(!TAO::Ledger::CreateBlock(pSigChain, PIN, nChannel, *(TAO::Ledger::TritiumBlock *)pBlock, ++nBlockIterator))
               {
                   debug::error(FUNCTION, "Failed to create a new Tritium Block.");
                   return nullptr;
               }

               /* Update the time. */
               pBlock->UpdateTime();

               /* Get the proof hash. */
               hashProof = pBlock->ProofHash();

               /* Skip if not prime channel or version less than 5. */
               if(nChannel != 1 || pBlock->nVersion < 5)
                   break;

               /* Exit loop when the block is above minimum prime origins and less than 1024-bit hashes */
               if(hashProof > TAO::Ledger::bnPrimeMinOrigins.getuint1024() && !hashProof.high_bits(nBitMask))
                   break;
           }

           debug::log(2, FUNCTION, "Created new Tritium Block ", hashProof.SubString(), " nVersion=", pBlock->nVersion);

       }

       /* Create Legacy blocks if version 7 not active. */
       else
       {
           /* Create a new Legacy Block. */
           pBlock = new Legacy::LegacyBlock();

           /* Set it to a null state */
           pBlock->SetNull();

           /* Create a new block and loop for prime channel if minimum bit target length isn't met */
           while(true)
           {
               if(!Legacy::CreateBlock(*pMiningKey, CoinbaseTx, nChannel.load(), ++nBlockIterator, *(Legacy::LegacyBlock *)pBlock))
                   debug::error(FUNCTION, "Failed to create a new Legacy Block.");

               /* Get the proof hash. */
               hashProof = pBlock->ProofHash();

               /* Skip if not prime channel or version less than 5 */
               if(nChannel.load() != 1 || pBlock->nVersion < 5)
                   break;

                /* Exit loop when the block is above minimum prime origins and less than 1024-bit hashes */
                if(hashProof > TAO::Ledger::bnPrimeMinOrigins.getuint1024() && !hashProof.high_bits(nBitMask))
                    break;
           }

           debug::log(2, FUNCTION, "Created new Legacy Block ", hashProof.SubString(), " nVersion=", pBlock->nVersion);

       }


       /* Return a pointer to the heap memory */
       return pBlock;
   }


   /*  signs the block. */
  bool Miner::sign_block(uint64_t nNonce, const uint512_t& hashMerkleRoot)
  {

      /* If the block dynamically casts to a legacy block, validate the legacy block. */
      {
          Legacy::LegacyBlock *pBlock = dynamic_cast<Legacy::LegacyBlock *>(mapBlocks[hashMerkleRoot]);

          if(pBlock)
          {
              pBlock->nNonce = nNonce;
              pBlock->UpdateTime();

              if(!Legacy::SignBlock(*pBlock, Legacy::Wallet::GetInstance()))
                  return debug::error(FUNCTION, "Unable to Sign Legacy Block ", hashMerkleRoot.SubString());

              return true;
          }
      }

      /* If the block dynamically casts to a tritium block, validate the tritium block. */
      TAO::Ledger::TritiumBlock *pBlock = dynamic_cast<TAO::Ledger::TritiumBlock *>(mapBlocks[hashMerkleRoot]);

      if(pBlock)
      {
          pBlock->nNonce = nNonce;
          pBlock->UpdateTime();

          /* Check that the account is unlocked for minting */
          if(!TAO::API::users->CanMint())
              return debug::error(FUNCTION, "Account has not been unlocked for minting");

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
                  if(!key.SetSecret(vchSecret, true))
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
               {
                   debug::log(2, FUNCTION, "Invalid Work for Legacy Block ", hashMerkleRoot.SubString());

                   return false;
               }

               /* Block is valid - Tell the wallet to keep this key. */
               pMiningKey->KeepKey();

               return true;
           }
       }

       /* If the block dynamically casts to a tritium block, validate the tritium block. */
       TAO::Ledger::TritiumBlock *pBlock = dynamic_cast<TAO::Ledger::TritiumBlock *>(mapBlocks[hashMerkleRoot]);

       if(pBlock)
       {
           debug::log(2, FUNCTION, "Tritium");
           pBlock->print();

           /* Log block found */
           if(config::GetArg("-verbose", 0) > 0)
           {
               std::string strTimestamp(convert::DateTimeStrFormat(runtime::unifiedtimestamp()));

               if(pBlock->nChannel == 1)
               {
                   debug::log(1, FUNCTION, "Nexus Tritium Miner: new Prime channel block found at unified time ", strTimestamp);
                   debug::log(1, "  blockHash: ", pBlock->ProofHash().SubString(30), " block height: ", pBlock->nHeight);
                   debug::log(1, "  prime cluster verified of size ", TAO::Ledger::GetDifficulty(pBlock->nBits, 1));
               }
               else if(pBlock->nChannel == 2)
               {
                   uint1024_t hashTarget = LLC::CBigNum().SetCompact(pBlock->nBits).getuint1024();

                   debug::log(1, FUNCTION, "Nexus Tritium Miner: new Hash channel block found at unified time ", strTimestamp);
                   debug::log(1, "  blockHash: ", pBlock->ProofHash().SubString(30), " block height: ", pBlock->nHeight);
                   debug::log(1, "  target: ", hashTarget.SubString(30));
               }
           }

           if(pBlock->hashPrevBlock != TAO::Ledger::ChainState::hashBestChain.load())
           {
               debug::log(0, FUNCTION, "Generated block is stale");
               return false;
           }

           /* Attempt to get the sigchain. */
           memory::encrypted_ptr<TAO::Ledger::SignatureChain>& pSigChain = TAO::API::users->GetAccount(0);
           if(!pSigChain)
               return debug::error(FUNCTION, "Couldn't get the unlocked sigchain");

           /* Lock the sigchain that is being mined. */
           LOCK(TAO::API::users->CREATE_MUTEX);

           /* Process the block and relay to network if it gets accepted into main chain. */
           if(!TritiumNode::Process(*pBlock, nullptr))
           {
               debug::log(0, FUNCTION, "Generated block not accepted");
               return false;
           }

           return true;
       }

       /* If we get here, the block is null or doesn't exist. */
       return debug::error(FUNCTION, "null block");
   }


    /*  Determines if the mining wallet is unlocked. */
   bool Miner::is_locked()
   {
       /* Check if mining should use tritium or legacy for wallet locked check. */
       if(TAO::Ledger::VersionActive(runtime::unifiedtimestamp(), 7))
            return TAO::API::users->Locked();

       return Legacy::Wallet::GetInstance().IsLocked();
   }


}
