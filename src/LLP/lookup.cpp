/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2021

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People
____________________________________________________________________________________________*/

#include <LLP/types/lookup.h>
#include <LLP/types/tritium.h>
#include <LLP/templates/events.h>
#include <LLP/include/global.h>

#include <LLD/include/global.h>

#include <Legacy/types/transaction.h>
#include <Legacy/types/merkle.h>

#include <TAO/API/types/indexing.h>

#include <TAO/Operation/include/enum.h>

#include <TAO/Ledger/types/client.h>
#include <TAO/Ledger/types/transaction.h>
#include <TAO/Ledger/types/merkle.h>

namespace LLP
{

    /* Queue to handle dispatch requests. */
    util::atomic::lock_unique_ptr<std::set<uint64_t>> LookupNode::setRequests = new std::set<uint64_t>();


    /* Our global processing lock for dependants. */
    std::mutex LookupNode::DEPENDANT_MUTEX;


    /** Constructor **/
    LookupNode::LookupNode()
    : Connection ( )
    {
    }


    /** Constructor **/
    LookupNode::LookupNode(Socket SOCKET_IN, DDOS_Filter* DDOS_IN, bool fDDOSIn)
    : Connection (SOCKET_IN, DDOS_IN, fDDOSIn)
    {
    }


    /** Constructor **/
    LookupNode::LookupNode(DDOS_Filter* DDOS_IN, bool fDDOSIn)
    : Connection (DDOS_IN, fDDOSIn)
    {
    }


    /* Virtual destructor. */
    LookupNode::~LookupNode()
    {
    }


    /* Virtual Functions to Determine Behavior of Message LLP. */
    void LookupNode::Event(uint8_t EVENT, uint32_t LENGTH)
    {
        /* Handle any DDOS Packet Filters. */
        if(EVENT == EVENTS::HEADER)
        {
            /* Checks for incoming connections only. */
            if(fDDOS.load() && Incoming())
            {
                /* Track our current packet. */
                Packet PACKET   = this->INCOMING;

                /* Incoming connection should never send time data. */
                if(!REQUEST::VALID(PACKET.HEADER))
                    DDOS->Ban("INVALID HEADER: OUT OF RANGE");
            }
        }

        /* Handle for a Packet Data Read. */
        if(EVENT == EVENTS::PACKET)
            return;

        /* On Generic Event, Broadcast new block if flagged. */
        if(EVENT == EVENTS::GENERIC)
            return;

        /* On Connect Event, Assign the Proper Daemon Handle. */
        if(EVENT == EVENTS::CONNECT)
        {
            /* Initialize our conneciton now. */
            if(!Incoming())
                PushMessage(REQUEST::CONNECT, LLP::SESSION_ID);

            return;
        }

        /* On Disconnect Event, Reduce the Connection Count for Daemon */
        if(EVENT == EVENTS::DISCONNECT)
            return;
    }


    /* Main message handler once a packet is recieved. */
    bool LookupNode::ProcessPacket()
    {
        /* Deserialize the packeet from incoming packet payload. */
        DataStream ssPacket(INCOMING.DATA, SER_NETWORK, PROTOCOL_VERSION);

        /* Get our request-id. */
        uint64_t nRequestID = 0;

        /* Get our request-id if not connecting. */
        if(INCOMING.HEADER != REQUEST::CONNECT)
            ssPacket >> nRequestID;

        /* Switch based on our incoming message. */
        switch(INCOMING.HEADER)
        {
            /* Standard handle for a merkle transaction. */
            case RESPONSE::MERKLE:
            {
                /* Check that we made this request. */
                if(!setRequests->count(nRequestID))
                    return debug::drop(NODE, "unsolicted response-id ", nRequestID);

                /* Get the specifier for dependant. */
                uint8_t nSpecifier;
                ssPacket >> nSpecifier;

                /* Switch based on our specifier. */
                switch(nSpecifier)
                {
                    /* Standard type for register in form of merkle transaction. */
                    case SPECIFIER::REGISTER:
                    case SPECIFIER::TRITIUM:
                    {
                        /* Get the transction from the stream. */
                        TAO::Ledger::MerkleTx tx;
                        ssPacket >> tx;

                        /* Cache the txid. */
                        const uint512_t hashTx = tx.GetHash();

                        /* Check for empty merkle tx. */
                        if(tx.hashBlock == 0)
                            return debug::drop(NODE, "FLAGS::LOOKUP: ", hashTx.SubString(), " REJECTED: no merkle branch for tx ", hashTx.SubString());

                        /* Grab the block to check merkle path. */
                        TAO::Ledger::ClientBlock block;
                        if(!LLD::Client->ReadBlock(tx.hashBlock, block))
                            return debug::drop(NODE, "FLAGS::LOOKUP: ", hashTx.SubString(), " REJECTED: missing block ", tx.hashBlock.SubString());

                        /* Check transaction contains valid information. */
                        if(!tx.Check())
                            return debug::drop(NODE, "FLAGS::LOOKUP: ", hashTx.SubString(), " REJECTED: ", debug::GetLastError());

                        /* Check the merkle branch. */
                        if(!tx.CheckMerkleBranch(block.hashMerkleRoot))
                            return debug::drop(NODE, "FLAGS::LOOKUP: ", hashTx.SubString(), " REJECTED: merkle transaction has invalid path");

                        /* Handle for regular dependant specifier. */
                        if(nSpecifier == SPECIFIER::TRITIUM)
                        {
                            LOCK(DEPENDANT_MUTEX);

                            /* Terminate early if we have already indexed this transaction. */
                            if(LLD::Client->HasIndex(hashTx))
                                return debug::drop(NODE, "FLAGS::LOOKUP: ", hashTx.SubString(), " REJECTED: index already exists");

                            /* Commit transaction to disk. */
                            LLD::TxnBegin(TAO::Ledger::FLAGS::BLOCK);
                            if(!LLD::Client->WriteTx(hashTx, tx))
                            {
                                LLD::TxnAbort(TAO::Ledger::FLAGS::BLOCK);
                                return debug::drop(NODE, "FLAGS::LOOKUP: ", hashTx.SubString(), " REJECTED: failed to write transaction");
                            }

                            /* Index the transaction to it's block. */
                            if(!LLD::Client->IndexBlock(hashTx, tx.hashBlock))
                            {
                                LLD::TxnAbort(TAO::Ledger::FLAGS::BLOCK);
                                return debug::drop(NODE, "FLAGS::LOOKUP: ", hashTx.SubString(), " REJECTED: failed to write block indexing entry");
                            }

                            /* Connect transaction in memory. */
                            if(!tx.Connect(TAO::Ledger::FLAGS::BLOCK))
                            {
                                LLD::TxnAbort(TAO::Ledger::FLAGS::BLOCK);
                                return debug::drop(NODE, "FLAGS::LOOKUP: ", hashTx.SubString(), " REJECTED: ", debug::GetLastError());
                            }

                            /* Flush to disk and clear mempool. */
                            LLD::TxnCommit(TAO::Ledger::FLAGS::BLOCK);
                        }

                        /* Connect transaction in memory if register specifier. */
                        else if(!tx.Connect(TAO::Ledger::FLAGS::LOOKUP))
                            return debug::drop(NODE, "tx ", hashTx.SubString(), " REJECTED: ", debug::GetLastError());

                        /* Add an indexing event. */
                        TAO::API::Indexing::PushTransaction(hashTx);

                        debug::log(0, "FLAGS::LOOKUP: ", hashTx.SubString(), " ACCEPTED");

                        break;
                    }

                    /* Standard type for register in form of merkle transaction. */
                    case SPECIFIER::LEGACY:
                    {
                        /* Get the transction from the stream. */
                        Legacy::MerkleTx tx;
                        ssPacket >> tx;

                        /* Cache the txid. */
                        uint512_t hashTx = tx.GetHash();

                        /* Check if we have this transaction already. */
                        if(LLD::Client->HasIndex(hashTx))
                            return debug::drop(NODE, "FLAGS::LOOKUP: ", hashTx.SubString(), " REJECTED: index already exists");

                        /* Grab the block to check merkle path. */
                        TAO::Ledger::ClientBlock block;
                        if(!LLD::Client->ReadBlock(tx.hashBlock, block))
                            return debug::drop(NODE, "FLAGS::LOOKUP: ", hashTx.SubString(), " REJECTED: missing block ", tx.hashBlock.SubString());

                        /* Check transaction contains valid information. */
                        if(!tx.Check())
                            return debug::drop(NODE, "FLAGS::LOOKUP: ", hashTx.SubString(), " REJECTED: ", debug::GetLastError());

                        /* Check the merkle branch. */
                        if(!tx.CheckMerkleBranch(block.hashMerkleRoot))
                            return debug::drop(NODE, "FLAGS::LOOKUP: ", hashTx.SubString(), " REJECTED: merkle transaction has invalid path");


                        LOCK(DEPENDANT_MUTEX);

                        /* Commit transaction to disk. */
                        LLD::TxnBegin(TAO::Ledger::FLAGS::BLOCK);
                        if(!LLD::Client->WriteTx(hashTx, tx))
                        {
                            LLD::TxnAbort(TAO::Ledger::FLAGS::BLOCK);
                            return debug::drop(NODE, "FLAGS::LOOKUP: ", hashTx.SubString(), " REJECTED: failed to write transaction");
                        }

                        /* Index the transaction to it's block. */
                        if(!LLD::Client->IndexBlock(hashTx, tx.hashBlock))
                        {
                            LLD::TxnAbort(TAO::Ledger::FLAGS::BLOCK);
                            return debug::drop(NODE, "FLAGS::LOOKUP: ", hashTx.SubString(), " REJECTED: failed to write block indexing entry");
                        }

                        /* Flush to disk and clear mempool. */
                        LLD::TxnCommit(TAO::Ledger::FLAGS::BLOCK);

                        /* Add an indexing event. */
                        TAO::API::Indexing::PushTransaction(hashTx);

                        /* Write Success to log. */
                        debug::log(0, "FLAGS::LOOKUP: ", hashTx.SubString(), " ACCEPTED");

                        break;
                    }

                    default:
                    {
                        return debug::drop(NODE, "invalid specifier message: ", std::hex, nSpecifier);
                    }
                }

                /* Let any blocking thread know we are finished processing now. */
                TriggerEvent(RESPONSE::MERKLE, nRequestID);

                /* Cleanup our requests set. */
                setRequests->erase(nRequestID);

                break;
            }

            /* Standard handle for a connection request. */
            case REQUEST::CONNECT:
            {
                /* Track our session-id to ensure we already have an active connection. */
                uint64_t nSessionID;
                ssPacket >> nSessionID;

                /* Check our session is active. */
                {
                    LOCK(TritiumNode::SESSIONS_MUTEX);

                    /* Check our internal sessions map. */
                    if(!TritiumNode::mapSessions.count(nSessionID))
                        return debug::drop(NODE, "Session ", nSessionID, " is not active");
                }

                break;
            }

            /* Standard handle to request a dependant transaction. */
            case REQUEST::DEPENDANT:
            {
                /* Get the specifier for dependant. */
                uint8_t nSpecifier;
                ssPacket >> nSpecifier;

                /* Switch based on our specifier. */
                switch(nSpecifier)
                {
                    /* Standard type for register in form of merkle transaction. */
                    case SPECIFIER::REGISTER:
                    {
                        /* Get the index of transaction. */
                        uint256_t hashRegister;
                        ssPacket >> hashRegister;

                        /* Check for existing localdb indexes. */
                        std::pair<uint512_t, uint64_t> pairIndex;
                        if(LLD::Local->ReadIndex(hashRegister, pairIndex))
                        {
                            /* Check for cache expiration. */
                            if(runtime::unifiedtimestamp() <= pairIndex.second)
                            {
                                /* Get the transaction from disk. */
                                TAO::Ledger::Transaction tx;
                                if(!LLD::Ledger->ReadTx(pairIndex.first, tx))
                                    break;

                                /* Build a markle transaction. */
                                TAO::Ledger::MerkleTx tMerkle =
                                    TAO::Ledger::MerkleTx(tx);

                                /* Build our tMerkle branch from the block. */
                                tMerkle.BuildMerkleBranch();

                                /* Send off the transaction to remote node. */
                                PushMessage(RESPONSE::MERKLE, nRequestID, uint8_t(SPECIFIER::REGISTER), tMerkle);

                                debug::log(0, NODE, "REQUEST::DEPENDANT::REGISTER: Using INDEX CACHE for ", hashRegister.SubString());

                                break;
                            }
                        }

                        /* Get the register from disk. */
                        TAO::Register::State state;
                        if(!LLD::Register->ReadState(hashRegister, state))
                            break;

                        /* If register is in the middle of a transfer, hashOwner will be owned by system. Detect and continue.*/
                        uint256_t hashOwner = state.hashOwner;
                        if(hashOwner.GetType() == TAO::Ledger::GENESIS::SYSTEM)
                            hashOwner.SetType(TAO::Ledger::GENESIS::UserType());

                        /* Read the last hash of owner. */
                        uint512_t hashLast = 0;
                        if(!LLD::Ledger->ReadLast(hashOwner, hashLast))
                            break;

                        /* Iterate through sigchain for register updates. */
                        while(hashLast != 0)
                        {
                            /* Get the transaction from disk. */
                            TAO::Ledger::Transaction tx;
                            if(!LLD::Ledger->ReadTx(hashLast, tx))
                                break;

                            /* Handle DDOS. */
                            if(fDDOS.load() && DDOS)
                                DDOS->rSCORE += 1;

                            /* Set the next last. */
                            hashLast = !tx.IsFirst() ? tx.hashPrevTx : 0;

                            /* Check through all the contracts. */
                            for(int32_t nContract = tx.Size() - 1; nContract >= 0; --nContract)
                            {
                                /* Get the contract. */
                                const TAO::Operation::Contract& rContract = tx[nContract];

                                /* Reset the operation stream position in case it was loaded from mempool and therefore still in previous state */
                                rContract.Reset();

                                /* Get the operation byte. */
                                uint8_t OPERATION = 0;
                                rContract >> OPERATION;

                                /* Check for conditional OP */
                                switch(OPERATION)
                                {
                                    case TAO::Operation::OP::VALIDATE:
                                    {
                                        /* Seek through validate. */
                                        rContract.Seek(68);
                                        rContract >> OPERATION;

                                        break;
                                    }

                                    case TAO::Operation::OP::CONDITION:
                                    {
                                        /* Get new operation. */
                                        rContract >> OPERATION;
                                    }
                                }

                                /* Check for key operations. */
                                switch(OPERATION)
                                {
                                    /* Break when at the register declaration. */
                                    case TAO::Operation::OP::WRITE:
                                    case TAO::Operation::OP::CREATE:
                                    case TAO::Operation::OP::APPEND:
                                    case TAO::Operation::OP::CLAIM:
                                    case TAO::Operation::OP::DEBIT:
                                    case TAO::Operation::OP::CREDIT:
                                    case TAO::Operation::OP::TRUST:
                                    case TAO::Operation::OP::GENESIS:
                                    case TAO::Operation::OP::LEGACY:
                                    case TAO::Operation::OP::FEE:
                                    {
                                        /* Seek past claim txid. */
                                        if(OPERATION == TAO::Operation::OP::CLAIM ||
                                           OPERATION == TAO::Operation::OP::CREDIT)
                                            rContract.Seek(68);

                                        /* Extract the address from the rContract. */
                                        TAO::Register::Address hashAddress;
                                        if(OPERATION == TAO::Operation::OP::TRUST ||
                                           OPERATION == TAO::Operation::OP::GENESIS)
                                        {
                                            hashAddress =
                                                TAO::Register::Address(std::string("trust"), state.hashOwner, TAO::Register::Address::TRUST);
                                        }
                                        else
                                            rContract >> hashAddress;

                                        /* Check for same address. */
                                        if(hashAddress != hashRegister)
                                            break;

                                        /* Build a markle transaction. */
                                        TAO::Ledger::MerkleTx tMerkle =
                                            TAO::Ledger::MerkleTx(tx);

                                        /* Build our merkle branch now. */
                                        tMerkle.BuildMerkleBranch();

                                        /* Send off the transaction to remote node. */
                                        PushMessage(RESPONSE::MERKLE, nRequestID, uint8_t(SPECIFIER::REGISTER), tMerkle);

                                        /* Build indexes for optimized processing. */
                                        std::pair<uint512_t, uint64_t> pairIndex = std::make_pair(tx.GetHash(), runtime::unifiedtimestamp() + 3600);
                                        if(!LLD::Local->WriteIndex(hashAddress, pairIndex)) //Index expires 1 hour after created
                                            break;

                                        /* Break out of main hash last. */
                                        hashLast = 0;

                                        /* Write update to our system logs. */
                                        debug::log(0, NODE, "REQUEST::DEPENDANT::REGISTER: Update INDEX for register ", hashAddress.SubString());

                                        break;
                                    }

                                    default:
                                        continue;
                                }
                            }
                        }

                        /* Debug output. */
                        debug::log(3, NODE, "REQUEST::DEPENDANT::REGISTER ", hashRegister.SubString());

                        break;
                    }


                    /* Handle for a raw tritium transaction. */
                    case SPECIFIER::TRITIUM:
                    {
                        /* Get the index of transaction. */
                        uint512_t hashTx;
                        ssPacket >> hashTx;

                        /* Check ledger database. */
                        TAO::Ledger::Transaction tx;
                        if(LLD::Ledger->ReadTx(hashTx, tx))
                        {
                            /* Build a markle transaction. */
                            TAO::Ledger::MerkleTx tMerkle = TAO::Ledger::MerkleTx(tx);

                            /* Build the tMerkle branch if the tx has been confirmed (i.e. it is not in the mempool) */
                            tMerkle.BuildMerkleBranch();

                            /* Send off the transaction to remote node. */
                            PushMessage(RESPONSE::MERKLE, nRequestID, uint8_t(SPECIFIER::TRITIUM), tMerkle);

                            /* Debug output. */
                            debug::log(3, NODE, "REQUEST::DEPENDANT::TRITIUM TRANSACTION", hashTx.SubString());
                        }
                        else if(DDOS)
                            DDOS->rSCORE += 10;

                        break;
                    }


                    /* Handle for a raw legacy transaction. */
                    case SPECIFIER::LEGACY:
                    {
                        /* Get the index of transaction. */
                        uint512_t hashTx;
                        ssPacket >> hashTx;

                        /* Check ledger database. */
                        Legacy::Transaction tx;
                        if(LLD::Legacy->ReadTx(hashTx, tx))
                        {
                            /* Build a markle transaction. */
                            Legacy::MerkleTx tMerkle = Legacy::MerkleTx(tx);

                            /* Build the tMerkle branch if the tx has been confirmed (i.e. it is not in the mempool) */
                            tMerkle.BuildMerkleBranch();

                            /* Send off the transaction to remote node. */
                            PushMessage(RESPONSE::MERKLE, nRequestID, uint8_t(SPECIFIER::LEGACY), tMerkle);

                            /* Debug output. */
                            debug::log(3, NODE, "REQUEST::DEPENDANT::LEGACY TRANSACTION", hashTx.SubString());
                        }
                        else if(DDOS)
                            DDOS->rSCORE += 10;

                        break;
                    }
                }

                break;
            }
        }

        return true;
    }
}
