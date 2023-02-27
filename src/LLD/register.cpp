/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2021

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <LLP/include/global.h>

#include <LLD/types/register.h>

#include <TAO/Register/include/enum.h>
#include <TAO/Register/include/unpack.h>

#include <TAO/Ledger/include/constants.h>

namespace LLD
{

    /** The Database Constructor. To determine file location and the Bytes per Record. **/
    RegisterDB::RegisterDB(const uint8_t nFlagsIn, const uint32_t nBucketsIn, const uint32_t nCacheIn)
    : SectorDatabase(std::string("_REGISTER")
    , nFlagsIn
    , nBucketsIn
    , nCacheIn)

    , MEMORY  ( )
    , pMemory (nullptr)
    , pMiner  (nullptr)
    , pCommit (new RegisterTransaction())
    , pLookup (nullptr)
    {
        /* Add a register cache if in client mode. */
        if(config::fClient.load())
            pLookup = new RegisterCache();
    }


    /* Default Destructor */
    RegisterDB::~RegisterDB()
    {
        /* Cleanup memory transactions. */
        if(pMemory)
            delete pMemory;

        /* Cleanup memory transactions. */
        if(pMiner)
            delete pMiner;

        /* Cleanup commited states. */
        if(pCommit)
            delete pCommit;

        /* Cleanup lookup states. */
        if(pLookup)
            delete pLookup;
    }


    /*  Writes a state register to the register database.
     *  If MEMPOOL flag is set, this will write state into a temporary
     *  memory to handle register state sequencing before blocks commit. */
    bool RegisterDB::WriteState(const uint256_t& hashRegister, const TAO::Register::State& state, const uint8_t nFlags)
    {
        /* Memory mode for pre-database commits. */
        if(nFlags == TAO::Ledger::FLAGS::MEMPOOL)
        {
            LOCK(MEMORY);

            /* Check for memory mode. */
            if(pMemory)
            {
                /* Check erase queue. */
                pMemory->setErase.erase(hashRegister);
                pMemory->mapStates[hashRegister] = state;

                return true;
            }

            /* Otherwise commit like normal. */
            pCommit->mapStates[hashRegister] = state;

            return true;
        }
        else if(nFlags == TAO::Ledger::FLAGS::LOOKUP)
        {
            LOCK(MEMORY);

            /* Check for lookup mode. */
            if(pLookup)
            {
                /* Insert into lookup map. */
                if(!pLookup->count(hashRegister))
                {
                    /* Insert into our memory-only cache if we do not have it yet. */
                    pLookup->insert(std::make_pair(hashRegister, std::make_pair(state, runtime::unifiedtimestamp())));

                    return true;
                }

                /* Update our cache if we do have it. */
                pLookup->at(hashRegister) = std::make_pair(state, runtime::unifiedtimestamp());

                return true;
            }
        }
        else if(nFlags == TAO::Ledger::FLAGS::MINER)
        {
            LOCK(MEMORY);

            /* Check for memory mode. */
            if(pMiner)
                pMiner->mapStates[hashRegister] = state;

            return true;
        }
        else if(nFlags == TAO::Ledger::FLAGS::BLOCK || nFlags == TAO::Ledger::FLAGS::ERASE)
        {
            LOCK(MEMORY);

            /* Remove the memory state if writing the disk state. */
            if(pCommit->mapStates.count(hashRegister))
            {
                /* Check for most recent memory state, and remove if writing it. */
                const TAO::Register::State& stateCheck = pCommit->mapStates[hashRegister];
                if(stateCheck == state || nFlags == TAO::Ledger::FLAGS::ERASE)
                {
                    /* Erase if transaction. */
                    if(pMemory)
                    {
                        pMemory->setErase.insert(hashRegister);
                        pMemory->mapStates.erase(hashRegister);
                    }
                    else
                        pCommit->mapStates.erase(hashRegister);
                }
            }

            /* Check for lookup state that will be overwritten. */
            if(pLookup && pLookup->count(hashRegister))
                pLookup->erase(hashRegister);

            /* Quit when erasing. */
            if(nFlags == TAO::Ledger::FLAGS::ERASE)
                return true;
        }

        /* Check for register address index. */
        if(config::GetBoolArg("-indexaddress"))
        {
            /* We include address if we are indexing by address. */
            return Write(
                std::make_pair(std::string("state"), hashRegister),
                std::make_pair(hashRegister, state), get_address_type(hashRegister) + "_address"
            );
        }


        /* Write the state to the register database */
        return Write(std::make_pair(std::string("state"), hashRegister),
            state, get_address_type(hashRegister));
    }


    /* Read a state register from the register database. */
    bool RegisterDB::ReadState(const uint256_t& hashRegister, TAO::Register::State& state, const uint8_t nFlags)
    {
        /* Memory mode for pre-database commits. */
        if(nFlags == TAO::Ledger::FLAGS::MEMPOOL ||
           nFlags == TAO::Ledger::FLAGS::LOOKUP  ||
          (nFlags == TAO::Ledger::FLAGS::FORCED && !config::fClient.load())) //we want FLAGS::FORCED to act like FLAGS::MEMPOOL for non -clients
        {
            LOCK(MEMORY);

            /* Check for a memory transaction first */
            if(pMemory && pMemory->mapStates.count(hashRegister))
            {
                /* Get the state from temporary transaction. */
                state = pMemory->mapStates[hashRegister];

                return true;
            }

            /* Check for state in memory map. */
            if(pCommit->mapStates.count(hashRegister))
            {
                /* Get the state from commited memory. */
                state = pCommit->mapStates[hashRegister];

                return true;
            }
        }
        else if(nFlags == TAO::Ledger::FLAGS::MINER)
        {
            LOCK(MEMORY);

            /* Check for a memory transaction first */
            if(pMiner && pMiner->mapStates.count(hashRegister))
            {
                /* Get the state from temporary transaction. */
                state = pMiner->mapStates[hashRegister];

                return true;
            }
        }

        /* Check if we have a forced flag. */
        if(nFlags != TAO::Ledger::FLAGS::FORCED || !config::fClient.load()) //we want FLAGS::FORCED to act like FLAGS::MEMPOOL for non -clients
        {
            /* Special case for indexed addresses. */
            if(config::GetBoolArg("-indexaddress"))
            {
                /* Create a pair to use for reading the reference for return. */
                std::pair<uint256_t, TAO::Register::State&> pairResult =
                    std::make_pair(hashRegister, std::ref(state));

                /* Check if it is on disk with -indexaddress. */
                if(Read(std::make_pair(std::string("state"), hashRegister), pairResult))
                    return true;
            }

            /* Otherwise check that it is on disk without -indexaddress. */
            else if(Read(std::make_pair(std::string("state"), hashRegister), state))
                return true;
        }

        /* Handle lookup if requested. */
        if((nFlags == TAO::Ledger::FLAGS::LOOKUP || nFlags == TAO::Ledger::FLAGS::FORCED) && config::fClient.load())
        {
            /* Get current timestamp. */
            const uint64_t nTimestamp =
                runtime::unifiedtimestamp();

            /* Check if our cache has expired. */
            const bool fCached  = (pLookup && pLookup->count(hashRegister));
            const bool fExpired = (fCached && (pLookup->at(hashRegister).second + 600 < nTimestamp));

            /* Check for expired or missing. */
            if(fExpired || !fCached)
            {
                /* Only perform lookup when we have active servers. */
                if(LLP::LOOKUP_SERVER)
                {
                    /* Try to find a connection first. */
                    std::shared_ptr<LLP::LookupNode> pConnection = LLP::LOOKUP_SERVER->GetConnection();
                    if(pConnection == nullptr)
                    {
                        /* Check for genesis. */
                        if(LLP::TRITIUM_SERVER)
                        {
                            std::shared_ptr<LLP::TritiumNode> pNode = LLP::TRITIUM_SERVER->GetConnection();
                            if(pNode != nullptr)
                            {
                                /* Get our lookup address now. */
                                const std::string strAddress =
                                    pNode->GetAddress().ToStringIP();

                                /* Make our new connection now. */
                                if(!LLP::LOOKUP_SERVER->ConnectNode(strAddress, pConnection))
                                    return debug::error(FUNCTION, "no connections available...");

                            }
                        }
                    }

                    /* Check that we have active connections. */
                    if(pConnection == nullptr)
                        throw TAO::API::Exception(-11, "No Connections Available");

                    /* Handle expired. */
                    if(fExpired)
                        debug::warning(FUNCTION, "EXPIRED: Cache is out of date by ", (nTimestamp - pLookup->at(hashRegister).second), " seconds");

                    /* Debug output to console. */
                    debug::log(1, FUNCTION, "CLIENT MODE: Requesting ACTION::GET::REGISTER for ", hashRegister.SubString());
                    pConnection->BlockingLookup
                    (
                        10000,
                        LLP::LookupNode::REQUEST::DEPENDANT,
                        uint8_t(LLP::LookupNode::SPECIFIER::REGISTER), hashRegister
                    );
                    debug::log(1, FUNCTION, "CLIENT MODE: TYPES::REGISTER received for ", hashRegister.SubString());
                }
            }

            /* Check for state in lookup map. */
            if(pLookup && pLookup->count(hashRegister))
            {
                /* Get the state from lookup memory. */
                state = pLookup->at(hashRegister).first;

                return true;
            }
        }

        return false;
    }


    /* Erase a state register from the register database. */
    bool RegisterDB::EraseState(const uint256_t& hashRegister, const uint8_t nFlags)
    {
        /* Check for memory transaction. */
        if(nFlags == TAO::Ledger::FLAGS::MEMPOOL)
        {
            LOCK(MEMORY);

            /* Check for available states. */
            if(pMemory)
            {
                pMemory->mapStates.erase(hashRegister);
                pMemory->setErase.insert(hashRegister);

                return true;
            }

            /* Erase state from mempool. */
            pCommit->mapStates.erase(hashRegister);

            return true;
        }
        else if(nFlags == TAO::Ledger::FLAGS::BLOCK || nFlags == TAO::Ledger::FLAGS::ERASE)
        {
            LOCK(MEMORY);

            /* Erase from memory if on block. */
            if(pCommit->mapStates.count(hashRegister))
            {
                /* Check for transction. */
                if(pMemory)
                {
                    pMemory->setErase.insert(hashRegister);
                    pMemory->mapStates.erase(hashRegister);
                }
                else
                    pCommit->mapStates.erase(hashRegister);
            }

            /* Break on erase.  */
            if(nFlags == TAO::Ledger::FLAGS::ERASE)
                return true;
        }

        /* Special case for indexed addresses. */
        if(config::GetBoolArg("-indexaddress"))
            return Erase(std::make_pair(std::string("state"), hashRegister));

        return Erase(std::make_pair(std::string("state"), hashRegister));
    }


    /* Read an object register from the register database. */
    bool RegisterDB::ReadObject(const uint256_t& hashRegister, TAO::Register::Object& object, const uint8_t nFlags)
    {
        /* Try to read the state here. */
        if(!ReadState(hashRegister, object, nFlags))
            return false;

        /* Attempt to parse the object. */
        if(object.nType == TAO::Register::REGISTER::OBJECT && !object.Parse())
            return false;

        return true;
    }


    /* Index a genesis to a register address. */
    bool RegisterDB::IndexTrust(const uint256_t& hashGenesis, const uint256_t& hashRegister)
    {
        /* We have our own logic here for indexing keys. */
        if(config::GetBoolArg("-indexaddress"))
            return Index(std::make_pair(std::string("genesis"), hashGenesis), std::make_pair(std::string("state"), hashRegister));

        return Index(std::make_pair(std::string("genesis"), hashGenesis), std::make_pair(std::string("state"), hashRegister));
    }


    /* Check that a genesis doesn't already exist. */
    bool RegisterDB::HasTrust(const uint256_t& hashGenesis)
    {
        return Exists(std::make_pair(std::string("genesis"), hashGenesis));
    }


    /* Write a genesis to a register address. */
    bool RegisterDB::WriteTrust(const uint256_t& hashGenesis, const TAO::Register::State& state)
    {
        /* Get trust account address for contract caller */
        const uint256_t hashRegister =
            TAO::Register::Address(std::string("trust"), hashGenesis, TAO::Register::Address::TRUST);

        {
            LOCK(MEMORY);

            /* Remove the memory state if writing the disk state. */
            if(pCommit->mapStates.count(hashRegister))
                pCommit->mapStates.erase(hashRegister);
        }

        /* We want to write the state like normal, but ensure we wipe memory states. */
        return WriteState(hashRegister, state);
    }


    /* Index a genesis to a register address. */
    bool RegisterDB::ReadTrust(const uint256_t& hashGenesis, TAO::Register::State& state)
    {
        /* Get trust account address for contract caller */
        const uint256_t hashRegister =
            TAO::Register::Address(std::string("trust"), hashGenesis, TAO::Register::Address::TRUST);

        /* Memory mode for pre-database commits. */
        if(nFlags == TAO::Ledger::FLAGS::MEMPOOL)
        {
            LOCK(MEMORY);

            /* Check for a memory transaction first */
            if(pMemory && pMemory->mapStates.count(hashRegister))
            {
                /* Get the state from temporary transaction. */
                state = pMemory->mapStates[hashRegister];

                return true;
            }

            /* Check for state in memory map. */
            if(pCommit->mapStates.count(hashRegister))
            {
                /* Get the state from commited memory. */
                state = pCommit->mapStates[hashRegister];

                return true;
            }
        }
        else if(nFlags == TAO::Ledger::FLAGS::MINER)
        {
            LOCK(MEMORY);

            /* Check for a memory transaction first */
            if(pMiner && pMiner->mapStates.count(hashRegister))
            {
                /* Get the state from temporary transaction. */
                state = pMiner->mapStates[hashRegister];

                return true;
            }
        }

        return ReadState(hashRegister, state);
    }


    /* Erase a genesis from a register address. */
    bool RegisterDB::EraseTrust(const uint256_t& hashGenesis)
    {
        /* Get trust account address for contract caller */
        uint256_t hashRegister =
            TAO::Register::Address(std::string("trust"), hashGenesis, TAO::Register::Address::TRUST);

        return Erase(std::make_pair(std::string("genesis"), hashGenesis), true); //true for keychain only erase
    }


    /* Determines if a state exists in the register database. */
    bool RegisterDB::HasState(const uint256_t& hashRegister, const uint8_t nFlags)
    {
        /* Memory mode for pre-database commits. */
        if(nFlags == TAO::Ledger::FLAGS::MEMPOOL || nFlags == TAO::Ledger::FLAGS::LOOKUP)
        {
            LOCK(MEMORY);

            /* Check internal memory state. */
            if(pMemory && pMemory->mapStates.count(hashRegister))
                return true;

            /* Check for state in memory map. */
            if(pCommit->mapStates.count(hashRegister))
                return true;
        }
        else if(nFlags == TAO::Ledger::FLAGS::MINER)
        {
            LOCK(MEMORY);

            /* Check internal memory state. */
            if(pMiner && pMiner->mapStates.count(hashRegister))
                return true;
        }

        /* Special case for indexed addresses. */
        if(config::GetBoolArg("-indexaddress"))
            return Exists(std::make_pair(std::string("state"), hashRegister));

        return Exists(std::make_pair(std::string("state"), hashRegister));
    }


    /* Flag to determine if address indexing has completed. For -indexaddress flag. */
    void RegisterDB::IndexAddress()
    {
        /* Check for address indexing flag. */
        if(Exists(std::string("reindexed")) && !config::GetBoolArg("-forcereindex"))
        {
            /* Check there is no argument supplied. */
            if(!config::HasArg("-indexaddress"))
            {
                /* Warn that -indexheight is persistent. */
                debug::notice(FUNCTION, "-indexaddress enabled from valid indexes");

                /* Set indexing argument now. */
                RECURSIVE(config::ARGS_MUTEX);
                config::mapArgs["-indexaddress"] = "1";
            }

            return;
        }

        /* Check there is no argument supplied. */
        if(!config::GetBoolArg("-indexaddress"))
            return;

        /* Start a timer to track. */
        runtime::timer timer;
        timer.Start();

        /* Get our starting hash. */
        uint1024_t hashBegin = TAO::Ledger::hashTritium;

        /* Check for hybrid mode. */
        if(config::fHybrid.load())
            LLD::Ledger->ReadHybridGenesis(hashBegin);

        /* Read the first tritium block. */
        TAO::Ledger::BlockState state;
        if(!LLD::Ledger->ReadBlock(hashBegin, state))
        {
            debug::warning(FUNCTION, "No tritium blocks available ", hashBegin.SubString());
            return;
        }

        /* Keep track of our total count. */
        uint32_t nScannedCount = 0;

        /* Keep track of already processed addresses. */
        std::set<uint256_t> setScanned;

        /* Start our scan. */
        debug::notice(FUNCTION, "Indexing addresses from block ", state.GetHash().SubString());
        while(!config::fShutdown.load())
        {
            /* Loop through found transactions. */
            for(uint32_t nIndex = 0; nIndex < state.vtx.size(); ++nIndex)
            {
                /* We only care about tritium transactions. */
                if(state.vtx[nIndex].first != TAO::Ledger::TRANSACTION::TRITIUM)
                    continue;

                /* Read the transaction from disk. */
                TAO::Ledger::Transaction tx;
                if(!LLD::Ledger->ReadTx(state.vtx[nIndex].second, tx))
                    continue;

                /* Iterate the transaction contracts. */
                for(uint32_t nContract = 0; nContract < tx.Size(); ++nContract)
                {
                    /* Grab contract reference. */
                    const TAO::Operation::Contract& rContract = tx[nContract];

                    /* Unpack the address we will be working on. */
                    uint256_t hashAddress;
                    if(!TAO::Register::Unpack(rContract, hashAddress))
                        continue;

                    /* Check if already in set. */
                    if(setScanned.count(hashAddress))
                        continue;

                    /* Check fo register in database. */
                    TAO::Register::State rState;
                    if(!config::GetBoolArg("-forcereindex"))
                    {
                        if(!Read(std::make_pair(std::string("state"), hashAddress), rState))
                            continue;
                    }

                    /* Handle a forced reindex if needed. */
                    else
                    {
                        std::pair<uint256_t, TAO::Register::State&> pairRead = std::make_pair(0, std::ref(rState));
                        if(!Read(std::make_pair(std::string("state"), hashAddress), pairRead))
                            continue;
                    }


                    /* Erase our record from the database. */
                    if(!Erase(std::make_pair(std::string("state"), hashAddress)))
                        continue;

                    /* Create our new register record. */
                    if(!Write(std::make_pair(std::string("state"), hashAddress),
                        std::make_pair(hashAddress, rState), get_address_type(hashAddress) + "_address"))
                        continue;

                    /* Add to completed set. */
                    setScanned.insert(hashAddress);
                }

                /* Update the scanned count for meters. */
                ++nScannedCount;

                /* Meter for output. */
                if(nScannedCount % 100000 == 0)
                {
                    /* Get the time it took to rescan. */
                    uint32_t nElapsedSeconds = timer.Elapsed();
                    debug::log(0, FUNCTION, "Re-indexed ", nScannedCount, " in ", nElapsedSeconds, " seconds (",
                        std::fixed, (double)(nScannedCount / (nElapsedSeconds > 0 ? nElapsedSeconds : 1 )), " tx/s)");
                }
            }

            /* Iterate to the next block in the queue. */
            state = state.Next();
            if(!state)
                break;
        }

        /* Write our last index now. */
        Write(std::string("reindexed"));

        debug::notice(FUNCTION, "Complated scanning ", nScannedCount, " tx with ", setScanned.size(), " registers in ", timer.Elapsed(), " seconds");
    }


    /* Begin a memory transaction following ACID properties. */
    void RegisterDB::MemoryBegin(const uint8_t nFlags)
    {
        LOCK(MEMORY);

        /* Check for miner. */
        if(nFlags == TAO::Ledger::FLAGS::MINER)
        {
            /* Set the pre-commit memory mode. */
            if(pMiner)
                delete pMiner;

            pMiner = new RegisterTransaction();

            return;
        }

        /* Set the pre-commit memory mode. */
        if(pMemory)
            delete pMemory;

        pMemory = new RegisterTransaction();
    }


    /* Abort a memory transaction following ACID properties. */
    void RegisterDB::MemoryRelease(const uint8_t nFlags)
    {
        LOCK(MEMORY);

        /* Check for miner. */
        if(nFlags == TAO::Ledger::FLAGS::MINER)
        {
            /* Set the pre-commit memory mode. */
            if(pMiner)
                delete pMiner;

            pMiner = nullptr;

            return;
        }

        /* Set the pre-commit memory mode. */
        if(pMemory)
            delete pMemory;

        pMemory = nullptr;
    }


    /* Commit a memory transaction following ACID properties. */
    void RegisterDB::MemoryCommit()
    {
        LOCK(MEMORY);

        /* Abort the current memory mode. */
        if(pMemory)
        {
            /* Loop through all new states and apply to commit data. */
            for(const auto& state : pMemory->mapStates)
                pCommit->mapStates[state.first] = state.second;

            /* Loop through values to erase. */
            for(const auto& erase : pMemory->setErase)
                pCommit->mapStates.erase(erase);

            /* Free the memory. */
            delete pMemory;
            pMemory = nullptr;
        }
    }


    /* Get a type string of the given address for sequential write keys. */
    std::string RegisterDB::get_address_type(const uint256_t& hashAddress)
    {
        /* Add sequential read keys for known address types. */
        switch(hashAddress.GetType())
        {
            /* Handle for account standard. */
            case TAO::Register::Address::ACCOUNT:
                return "account";

            /* Handle for append standard. */
            case TAO::Register::Address::APPEND:
                return "append";

            /* Handle for crypto standard. */
            case TAO::Register::Address::CRYPTO:
                return "crypto";

            /* Handle for name standard. */
            case TAO::Register::Address::NAME:
                return "name";

            /* Handle for namespace standard. */
            case TAO::Register::Address::NAMESPACE:
                return "namespace";

            /* Handle for object standard. */
            case TAO::Register::Address::OBJECT:
                return "object";

            /* Handle for raw standard. */
            case TAO::Register::Address::RAW:
                return "raw";

            /* Handle for readonly standard. */
            case TAO::Register::Address::READONLY:
                return "readonly";

            /* Handle for token standard. */
            case TAO::Register::Address::TOKEN:
                return "token";

            /* Handle for trust standard. */
            case TAO::Register::Address::TRUST:
                return "trust";
        }

        return "NONE";
    }
}
