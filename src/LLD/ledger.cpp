/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/Operation/include/enum.h>

#include <TAO/Register/include/enum.h>
#include <TAO/Register/include/constants.h>

#include <TAO/Ledger/types/transaction.h>
#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/include/constants.h>
#include <TAO/Ledger/types/state.h>
#include <TAO/Ledger/types/mempool.h>

#include <tuple>

namespace LLD
{

    /** The Database Constructor. To determine file location and the Bytes per Record. **/
    LedgerDB::LedgerDB(const uint8_t nFlagsIn, const uint32_t nBucketsIn, const uint32_t nCacheIn)
    : SectorDatabase(std::string("ledger")
    , nFlagsIn
    , nBucketsIn
    , nCacheIn)

    , MEMORY_MUTEX()
    , mapProofs()
    , mapClaims()
    {
    }


    /* Default Destructor */
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
        /* Check for Tritium transaction. */
        if(hashTx.GetType() == TAO::Ledger::TRITIUM)
        {
            /* Get the transaction. */
            TAO::Ledger::Transaction tx;
            if(!ReadTx(hashTx, tx, nFlags))
                throw debug::exception(FUNCTION, "failed to read contract");

            /* Get const reference for read-only access. */
            const TAO::Ledger::Transaction& ref = tx;
            return ref[nContract];
        }

        /* Check for Legacy transaction. */
        else if(hashTx.GetType() == TAO::Ledger::LEGACY)
        {
            /* Get the transaction. */
            Legacy::Transaction tx;
            if(!LLD::Legacy->ReadTx(hashTx, tx, nFlags))
                throw debug::exception(FUNCTION, "failed to get legacy transaction");

            /* Check boundaries. */
            if(nContract >= tx.vout.size())
                throw debug::exception(FUNCTION, "contract output out of bounds");

            /* Check script size. */
            if(tx.vout[nContract].scriptPubKey.size() != 34)
                throw debug::exception(FUNCTION, "invalid script size ", tx.vout[nContract].scriptPubKey.size());

            /* Get the script output. */
            uint256_t hashAccount;
            std::copy((uint8_t*)&tx.vout[nContract].scriptPubKey[1], (uint8_t*)&tx.vout[nContract].scriptPubKey[1] + 32, (uint8_t*)&hashAccount);

            /* Check for OP::RETURN. */
            if(tx.vout[nContract].scriptPubKey[33] != Legacy::OP_RETURN)
                throw debug::exception(FUNCTION, "last OP has to be OP_RETURN");

            /* Create Contract. */
            TAO::Operation::Contract contract;
            contract << uint8_t(TAO::Operation::OP::DEBIT) << TAO::Register::WILDCARD_ADDRESS << hashAccount << uint64_t(tx.vout[nContract].nValue) << uint64_t(0);

            return contract;
        }
        else
            throw debug::exception(FUNCTION, "invalid txid type");
    }


    /* Writes a transaction to the ledger DB. */
    bool LedgerDB::WriteTx(const uint512_t& hashTx, const TAO::Ledger::Transaction& tx)
    {
        return Write(hashTx, tx, "tx");
    }


    /* Reads a transaction from the ledger DB. */
    bool LedgerDB::ReadTx(const uint512_t& hashTx, TAO::Ledger::Transaction& tx, const uint8_t nFlags)
    {
        /* Special check for memory pool. */
        if(nFlags >= TAO::Ledger::FLAGS::MEMPOOL)
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
    bool LedgerDB::WriteClaimed(const uint512_t& hashTx, const uint32_t nContract, const uint64_t nClaimed, const uint8_t nFlags)
    {
        /* Get the key pair. */
        const std::pair<uint512_t, uint32_t> pair = std::make_pair(hashTx, nContract);

        /* Memory mode for pre-database commits. */
        if(nFlags == TAO::Ledger::FLAGS::MEMPOOL)
        {
            LOCK(MEMORY_MUTEX);

            /* Set the state in the memory map. */
            mapClaims[pair] = nClaimed;

            return true;
        }
        else if(nFlags == TAO::Ledger::FLAGS::BLOCK)
        {
            LOCK(MEMORY_MUTEX);

            /* Erase memory proof if they exist. */
            if(mapClaims.count(pair))
               mapClaims.erase(pair);
        }

        return Write(pair, nClaimed);
    }


    /* Read a partial to the ledger DB. */
    bool LedgerDB::ReadClaimed(const uint512_t& hashTx, const uint32_t nContract, uint64_t& nClaimed, const uint8_t nFlags)
    {
        /* Get the key pair. */
        const std::pair<uint512_t, uint32_t> pair = std::make_pair(hashTx, nContract);

        /* Memory mode for pre-database commits. */
        if(nFlags >= TAO::Ledger::FLAGS::MEMPOOL)
        {
            LOCK(MEMORY_MUTEX);

            /* Read the new proof state. */
            if(mapClaims.count(pair))
            {
                /* Set claimed from memory. */
                nClaimed = mapClaims[pair];

                return true;
            }
        }

        return Read(pair, nClaimed);
    }


    /* Read if a transaction is mature. */
    bool LedgerDB::ReadMature(const uint512_t &hashTx, const TAO::Ledger::BlockState* pblock)
    {
        /* Read the transaction so we can check type. Transaction must be in a block or it is immature by default. */
        TAO::Ledger::Transaction tx;
        if(!ReadTx(hashTx, tx))
            return false;

        /* Get the total confirmations. */
        uint32_t nConfirms = 0;
        if(!ReadConfirmations(hashTx, nConfirms, pblock))
            return false;

        /* Switch for coinbase. */
        if(tx.IsCoinBase())
            return nConfirms >= TAO::Ledger::MaturityCoinBase();

        /* Switch for coinstake. */
        else if(tx.IsCoinStake())
            return nConfirms >= TAO::Ledger::MaturityCoinStake();

        else
            return true; //non-producer transactions have no maturity requirement
    }


    /* Read the number of confirmations a transaction has. */
    bool LedgerDB::ReadConfirmations(const uint512_t& hashTx, uint32_t &nConfirms, const TAO::Ledger::BlockState* pblock)
    {
        /* Read a block state for this transaction. */
        TAO::Ledger::BlockState state;
        if(!ReadBlock(hashTx, state))
        {
            nConfirms = 0;
            return false;
        }

        /* Check for overflows. */
        uint32_t nHeight = (pblock ? pblock->nHeight : TAO::Ledger::ChainState::nBestHeight.load());
        if(nHeight < state.nHeight)
        {
            nConfirms = 0;
            return debug::error(FUNCTION, "height overflow catch");
        }

        /* Set the number of confirmations based on chainstate/blockstate height. */
        nConfirms = nHeight - state.nHeight + 1;

        return true;
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
        debug::log(0, FUNCTION, "repairing index for ", hashTx.SubString());

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
                debug::error(FUNCTION, "block ", hashBlock.SubString(), " has no transactions");

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

            /* Move onto the next block if there is one */
            if(state.hashNextBlock != 0)
                state = state.Next();
            else
                break;
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
    bool LedgerDB::HasTx(const uint512_t& hashTx, const uint8_t nFlags)
    {
        /* Special check for memory pool. */
        if(nFlags >= TAO::Ledger::FLAGS::MEMPOOL)
        {
            /* Get the transaction. */
            if(TAO::Ledger::mempool.Has(hashTx))
                return true;
        }

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
    bool LedgerDB::ReadLast(const uint256_t& hashGenesis, uint512_t &hashLast, const uint8_t nFlags)
    {
        /* If the caller has requested to include mempool transactions then check there first*/
        if(nFlags >= TAO::Ledger::FLAGS::MEMPOOL)
        {
            TAO::Ledger::Transaction tx;
            if(TAO::Ledger::mempool.Get(hashGenesis, tx))
            {
                hashLast = tx.GetHash();

                return true;
            }
        }

        /* If we haven't checked the mempool or haven't found one in the mempool then read the last from the ledger DB */
        return Read(std::make_pair(std::string("last"), hashGenesis), hashLast);
    }


    /* Writes the last stake transaction of sigchain to disk indexed by genesis. */
    bool LedgerDB::WriteStake(const uint256_t& hashGenesis, const uint512_t& hashLast)
    {
        return Write(std::make_pair(std::string("stake"), hashGenesis), hashLast);
    }


    /* Erase the last stake transaction of sigchain. */
    bool LedgerDB::EraseStake(const uint256_t& hashGenesis)
    {
        return Erase(std::make_pair(std::string("stake"), hashGenesis));
    }


    /* Reads the last stake transaction of sigchain. */
    bool LedgerDB::ReadStake(const uint256_t& hashGenesis, uint512_t& hashLast, const uint8_t nFlags)
    {
        /* If we haven't checked the mempool or haven't found one in the mempool then read the last from the ledger DB */
        return Read(std::make_pair(std::string("stake"), hashGenesis), hashLast);
    }


    /* Writes a proof to disk. Proofs are used to keep track of spent temporal proofs. */
    bool LedgerDB::WriteProof(const uint256_t& hashProof, const uint512_t& hashTx,
                              const uint32_t nContract, const uint8_t nFlags)
    {
        /* Get the key typle. */
        const std::tuple<uint256_t, uint512_t, uint32_t> tuple = std::make_tuple(hashProof, hashTx, nContract);

        /* Memory mode for pre-database commits. */
        if(nFlags == TAO::Ledger::FLAGS::MEMPOOL)
        {
            LOCK(MEMORY_MUTEX);

            /* Write internal memory. */
            mapProofs[tuple] = 0;

            return true;
        }
        else if(nFlags == TAO::Ledger::FLAGS::BLOCK)
        {
            LOCK(MEMORY_MUTEX);

            /* Erase memory proof if they exist. */
            if(mapProofs.count(tuple))
               mapProofs.erase(tuple);
        }

        return Write(tuple);
    }


    /* Checks if a proof exists. Proofs are used to keep track of spent temporal proofs. */
    bool LedgerDB::HasProof(const uint256_t& hashProof, const uint512_t& hashTx,
                            const uint32_t nContract, const uint8_t nFlags)
    {
        /* Get the key pair. */
        const std::tuple<uint256_t, uint512_t, uint32_t> tuple = std::make_tuple(hashProof, hashTx, nContract);

        /* Memory mode for pre-database commits. */
        if(nFlags == TAO::Ledger::FLAGS::MEMPOOL)
        {
            LOCK(MEMORY_MUTEX);

            /* Check internal memory. */
            if(mapProofs.count(tuple))
                return true;
        }

        return Exists(tuple);
    }


    /* Remove a temporal proof from the database. */
    bool LedgerDB::EraseProof(const uint256_t& hashProof, const uint512_t& hashTx,
                              const uint32_t nContract, const uint8_t nFlags)
    {
        /* Get the key pair. */
        const std::tuple<uint256_t, uint512_t, uint32_t> tuple = std::make_tuple(hashProof, hashTx, nContract);

        /* Memory mode for pre-database commits. */
        if(nFlags == TAO::Ledger::FLAGS::MEMPOOL)
        {
            LOCK(MEMORY_MUTEX);

            /* Erase memory proof if they exist. */
            if(mapProofs.count(tuple))
                mapProofs.erase(tuple);

            return true;
        }

        return Erase(std::make_tuple(hashProof, hashTx, nContract));
    }


    /* Writes a block state object to disk. */
    bool LedgerDB::WriteBlock(const uint1024_t& hashBlock, const TAO::Ledger::BlockState& state)
    {
        return Write(hashBlock, state, "block");
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
