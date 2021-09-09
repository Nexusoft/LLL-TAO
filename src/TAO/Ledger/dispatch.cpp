/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2019

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To The Voice of The People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <LLP/include/global.h>
#include <LLP/types/tritium.h>

#include <TAO/Operation/include/enum.h>

#include <TAO/Ledger/include/dispatch.h>

#include <Util/include/mutex.h>
#include <Util/include/debug.h>
#include <Util/include/runtime.h>
#include <Util/include/string.h>

#include <Legacy/include/evaluate.h>

#include <functional>

/* Global TAO namespace. */
namespace TAO::Ledger
{

    /* Default Constructor. */
    Dispatch::Dispatch()
    : DISPATCH_QUEUE  (new std::queue<uint1024_t>())
    , DISPATCH_THREAD (std::bind(&Dispatch::Relay, this))
    , CONDITION       ( )
    {
    }


    /* Default destructor. */
    Dispatch::~Dispatch()
    {
        /* Cleanup our dispatch thread. */
        CONDITION.notify_all();
        if(DISPATCH_THREAD.joinable())
            DISPATCH_THREAD.join();
    }


    /*  Dispatch a new block hash to relay thread.*/
    void Dispatch::PushRelay(const uint1024_t& hashBlock)
    {
        DISPATCH_QUEUE->push(hashBlock);
        CONDITION.notify_one();
    }


    /* Handle relays of all events for LLP when processing block. */
    void Dispatch::Relay()
    {
        std::mutex CONDITION_MUTEX;
        while(!config::fShutdown.load())
        {
            /* Wait for entries in the queue. */
            std::unique_lock<std::mutex> CONDITION_LOCK(CONDITION_MUTEX);
            CONDITION.wait(CONDITION_LOCK,
            [this]
            {
                /* Check for shutdown. */
                if(config::fShutdown.load())
                    return true;

                //LOCK(DISPATCH_MUTEX); XXX: run with RACE_CHECK=1 to make sure this is thread-safe
                return DISPATCH_QUEUE->size() != 0;
            });

            /* Check for shutdown. */
            if(config::fShutdown.load())
                return;

            /* Start a stopwatch. */
            runtime::stopwatch swTimer;
            swTimer.start();

            /* Grab the next entry in the queue. */
            const uint1024_t hashBlock = DISPATCH_QUEUE->front();
            DISPATCH_QUEUE->pop();

            /* Read the block from disk. */
            BlockState block;
            if(!LLD::Ledger->ReadBlock(hashBlock, block))
            {
                /* Debug output. */
                debug::log(0, FUNCTION, "Relay ",
                    ANSI_COLOR_BRIGHT_RED, "FAILED ", ANSI_COLOR_RESET,
                    " for ", hashBlock.SubString(), ": block not on disk"
                );

                continue;
            }

            /* Relay the block and bestchain. */
            LLP::TRITIUM_SERVER->Relay
            (
                LLP::TritiumNode::ACTION::NOTIFY,

                /* Relay BLOCK notification. */
                uint8_t(LLP::TritiumNode::TYPES::BLOCK),
                hashBlock,

                /* Relay BESTCHAIN notification. */
                uint8_t(LLP::TritiumNode::TYPES::BESTCHAIN),
                hashBlock,

                /* Relay BESTHEIGHT notification. */
                uint8_t(LLP::TritiumNode::TYPES::BESTHEIGHT),
                block.nHeight
            );

            #ifndef IPHONE //block notify disabled for IOS, missing system command

            /* Check for block notify command if applicable. */
            std::string strCmd = config::GetArg("-blocknotify", "");
            if(!strCmd.empty())
            {
                /* %s is replaced with block hash in command. */
                ReplaceAll(strCmd, "%s", hashBlock.GetHex());

                /* Track our return value and log block notify success. */
                const int32_t nRet = std::system(strCmd.c_str());
                debug::log(0, FUNCTION, "Block Notify Executed with code ", nRet);
            }
            #endif

            /* Let's process all the transactios now. */
            DataStream ssRelay(SER_NETWORK, LLP::PROTOCOL_VERSION);
            for(const auto& proof : block.vtx)
            {
                /* Only work on tritium transactions for now. */
                if(proof.first == TRANSACTION::TRITIUM)
                {
                    /* Get the transaction hash. */
                    const uint512_t& hash = proof.second;

                    /* Make sure the transaction is on disk. */
                    TAO::Ledger::Transaction tx;
                    if(!LLD::Ledger->ReadTx(hash, tx))
                        continue;

                    /* Check all the tx contracts. */
                    for(uint32_t n = 0; n < tx.Size(); ++n)
                    {
                        /* Grab reference of our contract. */
                        const TAO::Operation::Contract& rContract = tx[n];

                        /* Skip to our primitive. */
                        rContract.SeekToPrimitive();

                        /* Check the contract's primitive. */
                        uint8_t nOP = 0;
                        rContract >> nOP;
                        switch(nOP)
                        {
                            case TAO::Operation::OP::TRANSFER:
                            case TAO::Operation::OP::DEBIT:
                            {
                                /* Seek to recipient. */
                                uint256_t hashTo;
                                rContract.Seek(32,  TAO::Operation::Contract::OPERATIONS);
                                rContract >> hashTo;

                                /* Read the owner of register. (check this for MEMPOOL, too) */
                                TAO::Register::State state;
                                if(!LLD::Register->ReadState(hashTo, state))
                                    continue;

                                /* Fire off our event. */
                                ssRelay << uint8_t(LLP::TritiumNode::TYPES::SIGCHAIN) << state.hashOwner << hash;

                                debug::log(2, FUNCTION, (nOP == TAO::Operation::OP::TRANSFER ? "TRANSFER: " : "DEBIT: "),
                                    hash.SubString(), " for genesis ", state.hashOwner.SubString());

                                break;
                            }

                            case TAO::Operation::OP::COINBASE:
                            {
                                /* Get the genesis. */
                                uint256_t hashGenesis;
                                rContract >> hashGenesis;

                                /* Commit to disk. */
                                if(tx[n].Caller() != hashGenesis)
                                {
                                    /* Fire off our event. */
                                    ssRelay << uint8_t(LLP::TritiumNode::TYPES::SIGCHAIN) << hashGenesis << hash;

                                    debug::log(2, FUNCTION, "COINBASE: ", hash.SubString(), " for genesis ", hashGenesis.SubString());
                                }

                                break;
                            }
                        }
                    }

                    /* Notify the sender as well. */
                    ssRelay << uint8_t(LLP::TritiumNode::TYPES::SIGCHAIN) << tx.hashGenesis << hash;
                }
                else if(proof.first == TRANSACTION::LEGACY)
                {
                    /* Get the transaction hash. */
                    const uint512_t& hash = proof.second;

                    /* Make sure the transaction isn't on disk. */
                    Legacy::Transaction tx;
                    if(!LLD::Legacy->ReadTx(hash, tx))
                        continue;

                    /* Check the outputs for send to register. */
                    for(const auto& out : tx.vout)
                    {
                        /* Check outputs for possible events. */
                        uint256_t hashTo;
                        if(Legacy::ExtractRegister(out.scriptPubKey, hashTo))
                        {
                            /* Read the owner of register. (check this for MEMPOOL, too) */
                            TAO::Register::State state;
                            if(!LLD::Register->ReadState(hashTo, state))
                                continue;

                            /* Fire off our event. */
                            ssRelay << uint8_t(LLP::TritiumNode::SPECIFIER::LEGACY) << uint8_t(LLP::TritiumNode::TYPES::SIGCHAIN) << state.hashOwner << hash;

                            debug::log(2, FUNCTION, "LEGACY: ", hash.SubString(), " for genesis ", state.hashOwner.SubString());
                        }
                    }
                }
            }

            /* Relay all of our SIGCHAIN events. */
            LLP::TRITIUM_SERVER->_Relay(LLP::TritiumNode::ACTION::NOTIFY, ssRelay);
        }
    }
}
