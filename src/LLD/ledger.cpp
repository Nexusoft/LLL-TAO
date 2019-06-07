/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/ledger.h>

#include <TAO/Operation/include/enum.h>

#include <TAO/Register/include/enum.h>

#include <TAO/Ledger/types/transaction.h>
#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/include/constants.h>
#include <TAO/Ledger/types/state.h>
#include <TAO/Ledger/types/mempool.h>

#include <tuple>

namespace LLD
{

    /** The Database Constructor. To determine file location and the Bytes per Record. **/
    LedgerDB::LedgerDB(uint8_t nFlagsIn)
    : SectorDatabase(std::string("ledger"), nFlagsIn)
    , MEMORY_MUTEX()
    , mapProofs()
    , mapClaims()
    {
    }


    /** Default Destructor **/
    LedgerDB::~LedgerDB()
    {
    }


    /* Writes the best chain pointer to the ledger DB. */
    bool LedgerDB::WriteBestChain(const uint1024_t& hashBest)
    {
        return Write(std::string("hashbestchain"), hashBest);
    }


    /* Reads the best chain pointer from the ledger DB. */
    bool LedgerDB::ReadBestChain(uint1024_t& hashBest)
    {
        return Read(std::string("hashbestchain"), hashBest);
    }


    /* Reads the best chain pointer from the ledger DB. */
    bool LedgerDB::ReadBestChain(memory::atomic<uint1024_t>& atomicBest)
    {
        uint1024_t hashBest = 0;
        if(!Read(std::string("hashbestchain"), hashBest))
            return false;

        atomicBest.store(hashBest);
        return true;
    }


    /* Reads a contract from the ledger DB. */
    TAO::Operation::Contract LedgerDB::ReadContract(const uint512_t& hashTx, const uint32_t nContract, const uint8_t nFlags)
    {
        /* Get the transaction. */
        TAO::Ledger::Transaction tx;
        if(!ReadTx(hashTx, tx, nFlags))
            throw std::runtime_error(debug::safe_printstr(FUNCTION, "failed to read contract"));

        /* Get const reference for read-only access. */
        const TAO::Ledger::Transaction& ref = tx;

        /* Check that the previous transaction is indexed. */
        if(!tx.IsConfirmed())
            throw std::runtime_error(debug::safe_printstr(FUNCTION, "previous transaction not confirmed"));

        /* Check flags. */
        if(nFlags == TAO::Ledger::FLAGS::BLOCK)
        {
            /* Check for coinbase transactions. */
            uint8_t nOP = 0;
            ref[nContract] >> nOP;

            /* Check for COINBASE. */
            if(nOP == TAO::Operation::OP::COINBASE)
            {
                /* Check for block. */
                TAO::Ledger::BlockState state;
                if(!ReadBlock(hashTx, state))
                    throw std::runtime_error(debug::safe_printstr(FUNCTION, "coinbase isn't included in block"));

                /* Check for overflows. */
                if(TAO::Ledger::ChainState::stateBest.load().nHeight < state.nHeight)
                    throw std::runtime_error(debug::safe_printstr(FUNCTION, "maturity overflow"));

                /* Check the intervals. */
                if((TAO::Ledger::ChainState::stateBest.load().nHeight - state.nHeight) <
                    (config::fTestNet ? TAO::Ledger::TESTNET_MATURITY_BLOCKS : TAO::Ledger::NEXUS_MATURITY_BLOCKS))
                    throw std::runtime_error(debug::safe_printstr(FUNCTION, "coinbase is immature"));
            }

            /* Reset the contract. */
            ref[nContract].Reset();
        }

        /* Get the contract. */
        return ref[nContract];
    }


    /* Writes a transaction to the ledger DB. */
    bool LedgerDB::WriteTx(const uint512_t& hashTx, const TAO::Ledger::Transaction& tx)
    {
        return Write(hashTx, tx);
    }


    /* Reads a transaction from the ledger DB. */
    bool LedgerDB::ReadTx(const uint512_t& hashTx, TAO::Ledger::Transaction& tx, const uint8_t nFlags)
    {
        /* Special check for memory pool. */
        if(nFlags == TAO::Ledger::FLAGS::MEMPOOL)
        {
            /* Get the transaction. */
            if(TAO::Ledger::mempool.Get(hashTx, tx))
                return true;
        }

        return Read(hashTx, tx);
    }


    /* Erases a transaction from the ledger DB. */
    bool LedgerDB::EraseTx(const uint512_t& hashTx)
    {
        return Erase(hashTx);
    }


    /* Writes a partial to the ledger DB. */
    bool LedgerDB::WriteClaimed(const uint512_t& hashTx,
                                const uint32_t nContract, const uint64_t nClaimed, const uint8_t nFlags)
    {
        /* Memory mode for pre-database commits. */
        if(nFlags == TAO::Ledger::FLAGS::MEMPOOL)
        {
            LOCK(MEMORY_MUTEX);

            /* Write the new proof state. */
            mapClaims[std::make_pair(hashTx, nContract)] = nClaimed;

            return true;
        }
        else if(nFlags == TAO::Ledger::FLAGS::BLOCK)
        {
            LOCK(MEMORY_MUTEX);

            /* Erase memory proof if they exist. */
            if(mapClaims.count(std::make_pair(hashTx, nContract)))
               mapClaims.erase(std::make_pair(hashTx, nContract));
        }

        return Write(std::make_pair(hashTx, nContract), nClaimed);
    }


    /* Read a partial to the ledger DB. */
    bool LedgerDB::ReadClaimed(const uint512_t& hashTx,
                               const uint32_t nContract, uint64_t& nClaimed, const uint8_t nFlags)
    {
        /* Memory mode for pre-database commits. */
        if(nFlags == TAO::Ledger::FLAGS::MEMPOOL)
        {
            LOCK(MEMORY_MUTEX);

            /* Read the new proof state. */
            if(mapClaims.count(std::make_pair(hashTx, nContract)))
                nClaimed = mapClaims[std::make_pair(hashTx, nContract)];

            return true;
        }

        return Read(std::make_pair(hashTx, nContract), nClaimed);
    }


    /* Determine if a transaction has already been indexed. */
    bool LedgerDB::HasIndex(const uint512_t& hashTx)
    {
        return Exists(std::make_pair(std::string("index"), hashTx));
    }


    /* Index a transaction hash to a block in keychain. */
    bool LedgerDB::IndexBlock(const uint512_t& hashTx, const uint1024_t& hashBlock)
    {
        return Index(std::make_pair(std::string("index"), hashTx), hashBlock);
    }


    /* Index a block height to a block in keychain. */
    bool LedgerDB::IndexBlock(const uint32_t& nBlockHeight, const uint1024_t& hashBlock)
    {
        return Index(std::make_pair(std::string("height"), nBlockHeight), hashBlock);
    }


    /* Erase a foreign index form the keychain */
    bool LedgerDB::EraseIndex(const uint512_t& hashTx)
    {
        return Erase(std::make_pair(std::string("index"), hashTx));
    }

    /* Erase a foreign index form the keychain */
    bool LedgerDB::EraseIndex(const uint32_t& nBlockHeight)
    {
        return Erase(std::make_pair(std::string("height"), nBlockHeight));
    }


    /*  Recover if an index is not found.
     *  Fixes a corrupted database with a linear search for the hash tx up
     *  to the chain height. */
    bool LedgerDB::RepairIndex(const uint512_t& hashTx, const TAO::Ledger::BlockState &state)
    {
        debug::log(0, FUNCTION, "repairing index for ", hashTx.ToString().substr(0, 20));

        TAO::Ledger::BlockState currState = state;
        uint1024_t hashBlock;

        /* Loop until it is found. */
        while(!config::fShutdown.load() && !currState.IsNull())
        {
            /* Give debug output of status. */
            if(currState.nHeight % 100000 == 0)
                debug::log(0, FUNCTION, "repairing index..... ", currState.nHeight);

            /* Get the hash of the current block. */
            hashBlock = currState.GetHash();

            /* Check the state vtx size. */
            if(currState.vtx.size() == 0)
                debug::error(FUNCTION, "block ", hashBlock.ToString().substr(0, 20), " has no transactions");

            /* Check for the transaction. */
            for(const auto& tx : currState.vtx)
            {
                /* If the transaction is found, write the index. */
                if(tx.second == hashTx)
                {
                    /* Repair the index once it is found. */
                    if(!IndexBlock(hashTx, hashBlock))
                        return false;

                    return true;
                }
            }

            currState = currState.Prev();
        }

        return false;
    }


    /*  Recover the block height index.
     *  Adds or fixes th block height index by iterating forward from the genesis block */
    bool LedgerDB::RepairIndexHeight()
    {
        runtime::timer timer;
        timer.Start();
        debug::log(0, FUNCTION, "block height index missing or incomplete");

        /* Get the best block state to start from. */
        TAO::Ledger::BlockState state = TAO::Ledger::ChainState::stateGenesis;

        /* Loop until it is found. */
        while(!config::fShutdown.load() && !state.IsNull())
        {
            /* Give debug output of status. */
            if(state.nHeight % 100000 == 0)
                debug::log(0, FUNCTION, "repairing block height index..... ", state.nHeight);

            if(!IndexBlock(state.nHeight, state.GetHash()))
                return false;

            state = state.Next();
        }

        uint32_t nElapsed = timer.Elapsed();
        timer.Stop();
        debug::log(0, FUNCTION, "Block height indexing complete in ", nElapsed, "s");

        return true;
    }


    /* Reads a block state from disk from a tx index. */
    bool LedgerDB::ReadBlock(const uint512_t& hashTx, TAO::Ledger::BlockState& state)
    {
        return Read(std::make_pair(std::string("index"), hashTx), state);
    }


    /* Reads a block state from disk from a tx index. */
    bool LedgerDB::ReadBlock(const uint32_t& nBlockHeight, TAO::Ledger::BlockState& state)
    {
        return Read(std::make_pair(std::string("height"), nBlockHeight), state);
    }


    /* Checks LedgerDB if a transaction exists. */
    bool LedgerDB::HasTx(const uint512_t& hashTx)
    {
        return Exists(hashTx);
    }


    /* Writes a new sequence event to the ledger database. */
    bool LedgerDB::WriteSequence(const uint256_t& hashAddress, const uint32_t nSequence)
    {
        return Write(std::make_pair(std::string("sequence"), hashAddress), nSequence);
    }


    /* Reads a new sequence from the ledger database */
    bool LedgerDB::ReadSequence(const uint256_t& hashAddress, uint32_t& nSequence)
    {
        return Read(std::make_pair(std::string("sequence"), hashAddress), nSequence);
    }


    /* Write a new event to the ledger database of current txid. */
    bool LedgerDB::WriteEvent(const uint256_t& hashAddress, const uint512_t& hashTx)
    {
        /* Get the current sequence number. */
        uint32_t nSequence = 0;
        if(!ReadSequence(hashAddress, nSequence))
            nSequence = 0; //reset value just in case

        /* Write the new sequence number iterated by one. */
        if(!WriteSequence(hashAddress, nSequence + 1))
            return false;

        return Index(std::make_pair(hashAddress, nSequence), hashTx);
    }


    /* Erase an event from the ledger database. */
    bool LedgerDB::EraseEvent(const uint256_t& hashAddress)
    {
        /* Get the current sequence number. */
        uint32_t nSequence = 0;
        if(!ReadSequence(hashAddress, nSequence))
            return false;

        /* Write the new sequence number iterated by one. */
        if(!WriteSequence(hashAddress, nSequence - 1))
            return false;

        return Erase(std::make_pair(hashAddress, nSequence - 1));
    }


    /*  Reads a new event to the ledger database of foreign index.
     *  This is responsible for knowing foreign sigchain events that correlate to your own. */
    bool LedgerDB::ReadEvent(const uint256_t& hashAddress, const uint32_t nSequence, TAO::Ledger::Transaction& tx)
    {
        return Read(std::make_pair(hashAddress, nSequence), tx);
    }


    /* Writes the last txid of sigchain to disk indexed by genesis. */
    bool LedgerDB::WriteLast(const uint256_t& hashGenesis, const uint512_t& hashLast)
    {
        return Write(std::make_pair(std::string("last"), hashGenesis), hashLast);
    }


    /* Erase the last txid of sigchain to disk indexed by genesis. */
    bool LedgerDB::EraseLast(const uint256_t& hashGenesis)
    {
        return Erase(std::make_pair(std::string("last"), hashGenesis));
    }


    /* Reads the last txid of sigchain to disk indexed by genesis. */
    bool LedgerDB::ReadLast(const uint256_t& hashGenesis, uint512_t& hashLast, const uint8_t nFlags)
    {
        /* If the caller has requested to include mempool transactions then check there first*/
        if(nFlags == TAO::Ledger::FLAGS::MEMPOOL)
        {
            TAO::Ledger::Transaction mempoolTx;
            if(TAO::Ledger::mempool.Get(hashGenesis, mempoolTx))
            {
                hashLast = mempoolTx.GetHash();

                return true;
            }
        }

        /* If we haven't checked the mempool or haven't found one in the mempool then read the last from the ledger DB */
        return Read(std::make_pair(std::string("last"), hashGenesis), hashLast);
    }


    /* Writes a proof to disk. Proofs are used to keep track of spent temporal proofs. */
    bool LedgerDB::WriteProof(const uint256_t& hashProof, const uint512_t& hashTx,
                              const uint32_t nContract, const uint8_t nFlags)
    {
        /* Memory mode for pre-database commits. */
        if(nFlags == TAO::Ledger::FLAGS::MEMPOOL)
        {
            LOCK(MEMORY_MUTEX);

            /* Write the new proof state. */
            mapProofs[std::make_tuple(hashProof, hashTx, nContract)] = 0;
            return true;
        }
        else if(nFlags == TAO::Ledger::FLAGS::BLOCK)
        {
            LOCK(MEMORY_MUTEX);

            /* Erase memory proof if they exist. */
            if(mapProofs.count(std::make_tuple(hashProof, hashTx, nContract)))
               mapProofs.erase(std::make_tuple(hashProof, hashTx, nContract));
        }

        return Write(std::make_tuple(hashProof, hashTx, nContract));
    }


    /* Checks if a proof exists. Proofs are used to keep track of spent temporal proofs. */
    bool LedgerDB::HasProof(const uint256_t& hashProof, const uint512_t& hashTx,
                            const uint32_t nContract, const uint8_t nFlags)
    {
        /* Memory mode for pre-database commits. */
        if(nFlags == TAO::Ledger::FLAGS::MEMPOOL)
        {
            LOCK(MEMORY_MUTEX);

            /* If exists in memory, return true. */
            if(mapProofs.count(std::make_tuple(hashProof, hashTx, nContract)))
                return true;
        }

        return Exists(std::make_tuple(hashProof, hashTx, nContract));
    }


    /* Remove a temporal proof from the database. */
    bool LedgerDB::EraseProof(const uint256_t& hashProof, const uint512_t& hashTx,
                              const uint32_t nContract, const uint8_t nFlags)
    {
        /* Memory mode for pre-database commits. */
        if(nFlags == TAO::Ledger::FLAGS::MEMPOOL)
        {
            LOCK(MEMORY_MUTEX);

            /* Erase memory proof if they exist. */
            if(mapProofs.count(std::make_tuple(hashProof, hashTx, nContract)))
            {
                mapProofs.erase(std::make_tuple(hashProof, hashTx, nContract));

                return true;
            }
        }

        return Erase(std::make_tuple(hashProof, hashTx, nContract));
    }


    /* Writes a block state object to disk. */
    bool LedgerDB::WriteBlock(const uint1024_t& hashBlock, const TAO::Ledger::BlockState& state)
    {
        return Write(hashBlock, state);
    }


    /* Reads a block state object from disk. */
    bool LedgerDB::ReadBlock(const uint1024_t& hashBlock, TAO::Ledger::BlockState& state)
    {
        return Read(hashBlock, state);
    }


    /* Reads a block state object from disk for an atomic object. */
    bool LedgerDB::ReadBlock(const uint1024_t& hashBlock, memory::atomic<TAO::Ledger::BlockState>& atomicState)
    {
        TAO::Ledger::BlockState state;
        if(!Read(hashBlock, state))
            return false;

        atomicState.store(state);
        return true;
    }


    /* Checks if there is a block state object on disk. */
    bool LedgerDB::HasBlock(const uint1024_t& hashBlock)
    {
        return Exists(hashBlock);
    }


    /* Erase a block from disk. */
    bool LedgerDB::EraseBlock(const uint1024_t& hashBlock)
    {
        return Erase(hashBlock);
    }


    /* Checks if a genesis transaction exists. */
    bool LedgerDB::HasGenesis(const uint256_t& hashGenesis)
    {
        return Exists(std::make_pair(std::string("genesis"), hashGenesis));
    }


    /* Writes a genesis transaction-id to disk. */
    bool LedgerDB::WriteGenesis(const uint256_t& hashGenesis, const uint512_t& hashTx)
    {
        return Write(std::make_pair(std::string("genesis"), hashGenesis), hashTx);
    }


    /* Reads a genesis transaction-id from disk. */
    bool LedgerDB::ReadGenesis(const uint256_t& hashGenesis, uint512_t& hashTx)
    {
        return Read(std::make_pair(std::string("genesis"), hashGenesis), hashTx);
    }

}
