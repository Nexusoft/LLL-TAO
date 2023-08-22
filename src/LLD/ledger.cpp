/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2023

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>
#include <LLP/include/global.h>

#include <TAO/API/types/transaction.h>

#include <TAO/Operation/include/enum.h>

#include <TAO/Register/include/enum.h>
#include <TAO/Register/include/constants.h>
#include <TAO/Register/include/unpack.h>

#include <TAO/Ledger/types/transaction.h>
#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/include/constants.h>
#include <TAO/Ledger/types/state.h>
#include <TAO/Ledger/types/merkle.h>
#include <TAO/Ledger/types/mempool.h>
#include <TAO/Ledger/types/client.h>

#include <tuple>

namespace LLD
{

    /** The Database Constructor. To determine file location and the Bytes per Record. **/
    LedgerDB::LedgerDB(const uint8_t nFlagsIn, const uint32_t nBucketsIn, const uint32_t nCacheIn)
    : SectorDatabase(std::string("_LEDGER")
    , nFlagsIn
    , nBucketsIn
    , nCacheIn)

    , MEMORY_MUTEX()
    , pMemory(nullptr)
    , pMiner(nullptr)
    , pCommit(new LedgerTransaction())
    {
    }


    /* Default Destructor */
    LedgerDB::~LedgerDB()
    {
        /* Free transaction memory. */
        if(pMemory)
            delete pMemory;

        /* Free miner memory. */
        if(pMiner)
            delete pMiner;

        /* Free commited memory. */
        if(pCommit)
            delete pCommit;
    }


    /* Writes the best chain pointer to the ledger DB. */
    bool LedgerDB::WriteBestChain(const uint1024_t& hashBest)
    {
        /* Check for client mode. */
        if(config::fClient.load())
            return Client->WriteBestChain(hashBest);

        return Write(std::string("hashbestchain"), hashBest);
    }


    /* Reads the best chain pointer from the ledger DB. */
    bool LedgerDB::ReadBestChain(uint1024_t &hashBest)
    {
        /* Check for client mode. */
        if(config::fClient.load())
            return Client->ReadBestChain(hashBest);

        return Read(std::string("hashbestchain"), hashBest);
    }


    /* Writes the hybrid chain pointer to the ledger DB. */
    bool LedgerDB::WriteHybridGenesis(const uint1024_t& hashBest)
    {
        return Write(std::string("hybrid"), hashBest);
    }


    /* Reads the hybrid chain pointer from the ledger DB. */
    bool LedgerDB::ReadHybridGenesis(uint1024_t &hashBest)
    {
        return Read(std::string("hybrid"), hashBest);
    }


    /* Reads the best chain pointer from the ledger DB. */
    bool LedgerDB::ReadBestChain(memory::atomic<uint1024_t> &atomicBest)
    {
        /* Check for client mode. */
        if(config::fClient.load())
        {
            uint1024_t hashBest = 0;
            if(!Client->ReadBestChain(hashBest))
                return false;

            atomicBest.store(hashBest);
            return true;
        }

        /* Check for best chain on disk. */
        uint1024_t hashBest = 0;
        if(!Read(std::string("hashbestchain"), hashBest))
            return false;

        atomicBest.store(hashBest);
        return true;
    }


    /* Reads a contract from the ledger DB. */
    const TAO::Operation::Contract LedgerDB::ReadContract(const uint512_t& hashTx, const uint32_t nContract, const uint8_t nFlags)
    {
        /* Check for Tritium transaction. */
        if(hashTx.GetType() == TAO::Ledger::TRITIUM)
        {
            /* Get the transaction. */
            TAO::Ledger::Transaction tx;
            if(!ReadTx(hashTx, tx, nFlags))
                throw debug::exception(FUNCTION, "failed to read contract");

            /* Get const reference for read-only access. */
            const TAO::Ledger::Transaction& rtx = tx;
            return rtx[nContract];
        }

        /* Check for Legacy transaction. */
        else if(hashTx.GetType() == TAO::Ledger::LEGACY)
        {
            /* Get the transaction. */
            Legacy::Transaction tx;
            if(!LLD::Legacy->ReadTx(hashTx, tx, nFlags))
                throw debug::exception(FUNCTION, "failed to read contract");

            return TAO::Operation::Contract(tx, nContract);
        }
        else
            throw debug::exception(FUNCTION, "invalid txid type ", hashTx.SubString());
    }


    /*  Reads a transaction from the ledger DB. */
    const TAO::Ledger::Transaction LedgerDB::ReadTx(const uint512_t& hashTx, const uint8_t nFlags)
    {
        /* Special check for memory pool. */
        TAO::Ledger::Transaction tx;
        if(nFlags == TAO::Ledger::FLAGS::MEMPOOL || nFlags == TAO::Ledger::FLAGS::MINER)
        {
            /* Get the transaction. */
            if(TAO::Ledger::mempool.Get(hashTx, tx))
                return tx;
        }

        /* Check for client mode. */
        if(config::fClient.load())
        {
            TAO::Ledger::MerkleTx mTX;
            if(!Client->ReadTx(hashTx, mTX, nFlags))
                throw debug::exception(FUNCTION, "failed to read -client tx");

            /* Set the internal transaction hash. */
            mTX.hashCache = hashTx;

            return mTX;
        }

        /* Check for failed read. */
        if(!Read(hashTx, tx))
            throw debug::exception(FUNCTION, "failed to read tx");

        /* Set the internal transaction hash. */
        tx.hashCache = hashTx;

        return tx;
    }


    /* Writes a transaction to the ledger DB. */
    bool LedgerDB::WriteTx(const uint512_t& hashTx, const TAO::Ledger::Transaction& tx)
    {
        return Write(hashTx, tx, "tx");
    }


    /* Reads a transaction from the ledger DB. */
    bool LedgerDB::ReadTx(const uint512_t& hashTx, TAO::Ledger::Transaction &tx, const uint8_t nFlags)
    {
        /* Special check for memory pool. */
        if(nFlags == TAO::Ledger::FLAGS::MEMPOOL || nFlags == TAO::Ledger::FLAGS::MINER)
        {
            /* Get the transaction. */
            if(TAO::Ledger::mempool.Get(hashTx, tx))
                return true;
        }

        /* Check for client mode. */
        if(config::fClient.load())
        {
            /* Get the merkle transaction from disk. */
            TAO::Ledger::MerkleTx mTX;
            if(!Client->ReadTx(hashTx, mTX, nFlags))
                return false;

            /* Set the return value. */
            tx           = mTX;
            tx.hashCache = hashTx;

            return true;
        }

        /* See if we can read it from the disk now. */
        if(Read(hashTx, tx))
        {
            /* Set the internal transaction hash. */
            tx.hashCache = hashTx;

            return true;
        }

        return false;
    }


    /* Reads a transaction from the ledger DB. */
    bool LedgerDB::ReadTx(const uint512_t& hashTx, TAO::API::Transaction &tx, const uint8_t nFlags)
    {
        /* Get a copy of our ledger transaction. */
        TAO::Ledger::Transaction tLedger;
        if(!ReadTx(hashTx, tLedger, nFlags))
            return false;

        /* Cast our ledger transaction to API transaction. */
        tx =
            static_cast<TAO::API::Transaction>(tLedger);

        return true;
    }


    /* Reads a transaction from the ledger DB. */
    bool LedgerDB::ReadTx(const uint512_t& hashTx, TAO::Ledger::Transaction &tx, bool &fConflicted, const uint8_t nFlags)
    {
        /* Special check for memory pool. */
        if(nFlags == TAO::Ledger::FLAGS::MEMPOOL || nFlags == TAO::Ledger::FLAGS::MINER)
        {
            /* Get the transaction. */
            if(TAO::Ledger::mempool.Get(hashTx, tx, fConflicted))
                return true;
        }

        /* Check for client mode. */
        if(config::fClient.load())
        {
            /* Get the merkle transaction from disk. */
            TAO::Ledger::MerkleTx mTX;
            if(!Client->ReadTx(hashTx, mTX, nFlags))
                return false;

            /* Set the return value. */
            tx           = mTX;
            tx.hashCache = hashTx;

            return true;
        }

        /* See if we can read it from the disk now. */
        if(Read(hashTx, tx))
        {
            /* Set the internal transaction hash. */
            tx.hashCache = hashTx;

            return true;
        }

        return false;
    }


    /* Reads a proof spending tx. Proofs are used to keep track of spent temporal proofs. */
    bool LedgerDB::ReadTx(const uint256_t& hashProof, const uint512_t& hashTx, const uint32_t nContract, TAO::Ledger::Transaction &tx)
    {
        /* Get the key tuple. */
        const std::tuple<uint256_t, uint512_t, uint32_t> tIndex =
            std::make_tuple(hashProof, hashTx, nContract);

        return Read(tIndex, tx);
    }


    /* Reads a contract spending tx. Contracts are used to keep track of contract validators. */
    bool LedgerDB::ReadTx(const uint512_t& hashTx, const uint32_t nContract, TAO::Ledger::Transaction &tx)
    {
        /* Get the key pair. */
        const std::tuple<std::string, uint512_t, uint32_t> tIndex =
            std::make_tuple("validated", hashTx, nContract);

        return Read(tIndex, tx);
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
        if(nFlags == TAO::Ledger::FLAGS::MEMPOOL || nFlags == TAO::Ledger::FLAGS::ERASE)
        {
            LOCK(MEMORY_MUTEX);

            /* Check for pending tranasaction. */
            if(pMemory)
            {
                pMemory->setEraseClaims.erase(pair);
                pMemory->mapClaims[pair] = nClaimed;

                return true;
            }

            /* Set the state in the memory map. */
            pCommit->mapClaims[pair] = nClaimed;

            return true;
        }
        else if(nFlags == TAO::Ledger::FLAGS::MINER)
        {
            LOCK(MEMORY_MUTEX);

            /* Check for pending tranasaction. */
            if(pMiner)
                pMiner->mapClaims[pair] = nClaimed;

            return true;
        }
        else if(nFlags == TAO::Ledger::FLAGS::BLOCK)
        {
            LOCK(MEMORY_MUTEX);

            /* Check for pending tranasaction. */
            if(pCommit->mapClaims.count(pair))
            {
                /* Check for last claimed amount. */
                const uint64_t nMemoryClaimed = pCommit->mapClaims[pair];
                if(nMemoryClaimed == nClaimed)
                {
                    /* Erase if a transaction. */
                    if(pMemory)
                    {
                        pMemory->mapClaims.erase(pair);
                        pMemory->setEraseClaims.insert(pair);
                    }
                    else
                        pCommit->mapClaims.erase(pair);
                }
            }
        }

        return Write(pair, nClaimed);
    }


    /* Read a partial to the ledger DB. */
    bool LedgerDB::ReadClaimed(const uint512_t& hashTx, const uint32_t nContract, uint64_t& nClaimed, const uint8_t nFlags)
    {
        /* Get the key pair. */
        const std::pair<uint512_t, uint32_t> pair = std::make_pair(hashTx, nContract);

        /* Memory mode for pre-database commits. */
        if(nFlags == TAO::Ledger::FLAGS::MEMPOOL || nFlags == TAO::Ledger::FLAGS::ERASE)
        {
            LOCK(MEMORY_MUTEX);

            /* Check for pending transaction. */
            if(pMemory && pMemory->mapClaims.count(pair))
            {
                /* Set claimed from memory. */
                nClaimed = pMemory->mapClaims[pair];

                return true;
            }

            /* Check for pending transaction. */
            if(pCommit->mapClaims.count(pair))
            {
                /* Set claimed from memory. */
                nClaimed = pCommit->mapClaims[pair];

                return true;
            }
        }
        else if(nFlags == TAO::Ledger::FLAGS::MINER)
        {
            LOCK(MEMORY_MUTEX);

            /* Check for pending transaction. */
            if(pMiner && pMiner->mapClaims.count(pair))
            {
                /* Set claimed from memory. */
                nClaimed = pMiner->mapClaims[pair];

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

        return ReadMature(tx, pblock);
    }


    /* Read if a transaction is mature. */
    bool LedgerDB::ReadMature(const TAO::Ledger::Transaction& tx, const TAO::Ledger::BlockState* pblock)
    {
        /* Get the total confirmations. */
        uint32_t nConfirms = 0;
        if(!ReadConfirmations(tx.GetHash(), nConfirms, pblock))
            return false;

        /* Switch for coinbase. */
        if(tx.IsCoinBase())
            return nConfirms >= TAO::Ledger::MaturityCoinBase((pblock ? *pblock : TAO::Ledger::ChainState::tStateBest.load()));

        /* Switch for coinstake. */
        else if(tx.IsCoinStake())
            return nConfirms >= TAO::Ledger::MaturityCoinStake((pblock ? *pblock : TAO::Ledger::ChainState::tStateBest.load()));

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
        /* Check indexes for -client mode. */
        if(config::fClient.load())
            return Client->HasIndex(hashTx);

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


    /* Reads a block state from disk from a tx index. */
    bool LedgerDB::ReadBlock(const uint512_t& hashTx, TAO::Ledger::BlockState &state)
    {
        /* Check for client mode. */
        if(config::fClient.load())
        {
            /* Get the merkle transaction from disk. */
            TAO::Ledger::ClientBlock block;
            if(!Client->ReadBlock(hashTx, block))
                return false;

            /* Set the return value. */
            state = block;
            return true;
        }

        return Read(std::make_pair(std::string("index"), hashTx), state);
    }


    /* Reads a block state from disk from a tx index. */
    bool LedgerDB::ReadBlock(const uint32_t& nBlockHeight, TAO::Ledger::BlockState &state)
    {
        //TODO: add -client switch for indexed blocks

        return Read(std::make_pair(std::string("height"), nBlockHeight), state);
    }


    /* Checks LedgerDB if a transaction exists. */
    bool LedgerDB::HasTx(const uint512_t& hashTx, const uint8_t nFlags)
    {
        /* Special check for memory pool. */
        if(nFlags == TAO::Ledger::FLAGS::MEMPOOL || nFlags == TAO::Ledger::FLAGS::MINER)
        {
            /* Get the transaction. */
            if(TAO::Ledger::mempool.Has(hashTx))
                return true;
        }

        /* Check indexes for -client mode. */
        if(config::fClient.load())
            return Client->HasTx(hashTx);

        return Exists(hashTx);
    }


    /* Writes a new sequence event to the ledger database. */
    bool LedgerDB::WriteSequence(const uint256_t& hashAddress, const uint32_t nSequence)
    {
        return Write(std::make_pair(std::string("sequence"), hashAddress), nSequence);
    }


    /* Reads a new sequence from the ledger database */
    bool LedgerDB::ReadSequence(const uint256_t& hashAddress, uint32_t &nSequence)
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

        /* Check for client mode. */
        if(config::fClient.load())
            return Client->Index(std::make_pair(hashAddress, nSequence), hashTx);

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

        /* Check for client mode. */
        if(config::fClient.load())
            return Client->Erase(std::make_pair(hashAddress, nSequence - 1));

        return Erase(std::make_pair(hashAddress, nSequence - 1));
    }

    /* Checks an event in the ledger database of foreign index. */
    bool LedgerDB::HasEvent(const uint256_t& hashAddress, const uint32_t nSequence)
    {
        /* Check for client mode. */
        if(config::fClient.load())
            return Client->Exists(std::make_pair(hashAddress, nSequence));

        return Exists(std::make_pair(hashAddress, nSequence));
    }


    /*  Reads a new event to the ledger database of foreign index.
     *  This is responsible for knowing foreign sigchain events that correlate to your own. */
    bool LedgerDB::ReadEvent(const uint256_t& hashAddress, const uint32_t nSequence, TAO::Ledger::Transaction &tx)
    {
        /* Check for client mode. */
        if(config::fClient.load())
            return Client->Read(std::make_pair(hashAddress, nSequence), tx);

        return Read(std::make_pair(hashAddress, nSequence), tx);
    }


    /* Reads the last event (highest sequence number) for the sig chain / register */
    bool LedgerDB::ReadLastEvent(const uint256_t& hashAddress, uint512_t& hashLast)
    {
        /* Get the last known event sequence for this address  */
        uint32_t nSequence = 0;
        ReadSequence(hashAddress, nSequence); // this can fail if no events exist yet, so dont check return value

        /* Read the transaction ID of the last event */
        if(nSequence > 0)
        {
            TAO::Ledger::Transaction tx;
            if(ReadEvent(hashAddress, nSequence -1, tx))
            {
                hashLast = tx.GetHash();
                return true;
            }
            else
                return false;
        }

        return true;
    }


    /* Writes the last txid of sigchain to disk indexed by genesis. */
    bool LedgerDB::WriteLast(const uint256_t& hashGenesis, const uint512_t& hashLast)
    {
        /* Check for client mode. */
        if(config::fClient.load())
            return Client->WriteLast(hashGenesis, hashLast);

        return Write(std::make_pair(std::string("last"), hashGenesis), hashLast);
    }


    /* Erase the last txid of sigchain to disk indexed by genesis. */
    bool LedgerDB::EraseLast(const uint256_t& hashGenesis)
    {
        /* Check for client mode. */
        if(config::fClient.load())
            return Client->Erase(std::make_pair(std::string("last"), hashGenesis));

        return Erase(std::make_pair(std::string("last"), hashGenesis));
    }


    /* Reads the last txid of sigchain to disk indexed by genesis. */
    bool LedgerDB::ReadLast(const uint256_t& hashGenesis, uint512_t &hashLast, const uint8_t nFlags)
    {
        /* If the caller has requested to include mempool transactions then check there first*/
        if(nFlags == TAO::Ledger::FLAGS::MEMPOOL)
        {
            TAO::Ledger::Transaction tx;
            if(TAO::Ledger::mempool.Get(hashGenesis, tx))
            {
                hashLast = tx.GetHash();

                return true;
            }
        }

        /* Check for client mode. */
        if(config::fClient.load())
            return Client->ReadLast(hashGenesis, hashLast);

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
    bool LedgerDB::ReadStake(const uint256_t& hashGenesis, uint512_t &hashLast, const uint8_t nFlags)
    {
        /* If we haven't checked the mempool or haven't found one in the mempool then read the last from the ledger DB */
        return Read(std::make_pair(std::string("stake"), hashGenesis), hashLast);
    }


    /* Writes a proof to disk. Proofs are used to keep track of spent temporal proofs. */
    bool LedgerDB::WriteProof(const uint256_t& hashProof, const uint512_t& hashTx,
                              const uint32_t nContract, const uint8_t nFlags)
    {
        /* Get the key typle. */
        const std::tuple<uint256_t, uint512_t, uint32_t> tIndex =
            std::make_tuple(hashProof, hashTx, nContract);

        /* Memory mode for pre-database commits. */
        if(nFlags == TAO::Ledger::FLAGS::MEMPOOL)
        {
            LOCK(MEMORY_MUTEX);

            /* Check for pending transactions. */
            if(pMemory)
            {
                /* Write proof to memory. */
                pMemory->setEraseProofs.erase(tIndex);
                pMemory->setProofs.insert(tIndex);

                return true;
            }

            /* Write proof to commited memory. */
            pCommit->setProofs.insert(tIndex);

            return true;
        }
        else if(nFlags == TAO::Ledger::FLAGS::MINER)
        {
            LOCK(MEMORY_MUTEX);

            /* Check for pending transactions. */
            if(pMiner)
                pMiner->setProofs.insert(tIndex);

            return true;
        }
        else if(nFlags == TAO::Ledger::FLAGS::BLOCK || nFlags == TAO::Ledger::FLAGS::ERASE)
        {
            LOCK(MEMORY_MUTEX);

            /* Erase memory proof if they exist. */
            if(pMemory)
            {
                pMemory->setEraseProofs.insert(tIndex);
                pMemory->setProofs.erase(tIndex);
            }
            else
               pCommit->setProofs.erase(tIndex);

            /* Check for erase to short circuit out. */
            if(nFlags == TAO::Ledger::FLAGS::ERASE)
                return true;
        }

        /* Check the client database. */
        if(config::fClient.load())
            return Client->WriteProof(hashProof, hashTx, nContract);

        return Write(tIndex);
    }


    /* Indexes a proof to disk tied to spending transactions. Proofs are used to keep track of spent temporal proofs. */
    bool LedgerDB::IndexProof(const uint256_t& hashProof, const uint512_t& hashTx,
                    const uint32_t nContract, const uint512_t& hashIndex, const uint8_t nFlags)
    {
        /* Check for block flag. */
        if(nFlags != TAO::Ledger::FLAGS::BLOCK)
            return WriteProof(hashProof, hashTx, nContract, nFlags);

        /* Get the key typle. */
        const std::tuple<uint256_t, uint512_t, uint32_t> tIndex =
            std::make_tuple(hashProof, hashTx, nContract);

        /* Cleanup our proof memory if writing for block. */
        if(nFlags == TAO::Ledger::FLAGS::BLOCK || nFlags == TAO::Ledger::FLAGS::ERASE)
        {
            LOCK(MEMORY_MUTEX);

            /* Erase memory proof if they exist. */
            if(pMemory)
            {
                pMemory->setEraseProofs.insert(tIndex);
                pMemory->setProofs.erase(tIndex);
            }
            else
               pCommit->setProofs.erase(tIndex);

            /* Check for erase to short circuit out. */
            if(nFlags == TAO::Ledger::FLAGS::ERASE)
                return true;
        }

        return Index(tIndex, hashIndex);
    }


    /* Checks if a proof exists. Proofs are used to keep track of spent temporal proofs. */
    bool LedgerDB::HasProof(const uint256_t& hashProof, const uint512_t& hashTx,
                            const uint32_t nContract, const uint8_t nFlags)
    {
        /* Get the key pair. */
        const std::tuple<uint256_t, uint512_t, uint32_t> tIndex =
            std::make_tuple(hashProof, hashTx, nContract);

        /* Memory mode for pre-database commits. */
        if(nFlags == TAO::Ledger::FLAGS::MEMPOOL || nFlags == TAO::Ledger::FLAGS::LOOKUP)
        {
            LOCK(MEMORY_MUTEX);

            /* Check pending transaction memory. */
            if(pMemory && pMemory->setProofs.count(tIndex))
                return true;

            /* Check commited memory. */
            if(pCommit->setProofs.count(tIndex))
                return true;
        }
        else if(nFlags == TAO::Ledger::FLAGS::MINER)
        {
            LOCK(MEMORY_MUTEX);

            /* Check pending transaction memory. */
            if(pMiner && pMiner->setProofs.count(tIndex))
                return true;
        }

        /* Check the client database. */
        if(config::fClient.load())
            return Client->HasProof(hashProof, hashTx, nContract, nFlags);

        return Exists(tIndex);
    }


    /* Remove a temporal proof from the database. */
    bool LedgerDB::EraseProof(const uint256_t& hashProof, const uint512_t& hashTx,
                              const uint32_t nContract, const uint8_t nFlags)
    {
        /* Get the key pair. */
        std::tuple<uint256_t, uint512_t, uint32_t> tuple = std::make_tuple(hashProof, hashTx, nContract);

        /* Check for memory transaction. */
        if(nFlags == TAO::Ledger::FLAGS::MEMPOOL || nFlags == TAO::Ledger::FLAGS::LOOKUP)
        {
            LOCK(MEMORY_MUTEX);

            /* Check for available states. */
            if(pMemory)
            {
                /* Erase state out of transaction. */
                pMemory->setProofs.erase(tuple);
                pMemory->setEraseProofs.insert(tuple);

                return true;
            }

            /* Erase proof from mempool. */
            pCommit->setProofs.erase(tuple);

            return true;
        }
        else if(nFlags == TAO::Ledger::FLAGS::BLOCK || nFlags == TAO::Ledger::FLAGS::ERASE)
        {
            LOCK(MEMORY_MUTEX);

            /* Erase memory proof if they exist. */
            if(pCommit->setProofs.count(tuple))
            {
                /* Erase the proof. */
                if(pMemory)
                {
                    pMemory->setEraseProofs.insert(tuple);
                    pMemory->setProofs.erase(tuple);
                }
                else
                    pCommit->setProofs.erase(tuple);
            }

            /* Break on erase.  */
            if(nFlags == TAO::Ledger::FLAGS::ERASE)
                return true;
        }

        /* Check the client database. */
        if(config::fClient.load())
            return Client->EraseProof(hashProof, hashTx, nContract);

        return Erase(tuple);
    }


    /* Indexes a contract to disk tied to validating transactions */
    bool LedgerDB::IndexContract(const uint512_t& hashTx, const uint32_t nContract, const uint512_t& hashIndex)
    {
        /* Get the key pair. */
        const std::tuple<std::string, uint512_t, uint32_t> tIndex =
            std::make_tuple("validated", hashTx, nContract);

        /* Index our record to the database. */
        return Index(tIndex, hashIndex);
    }


    /* Remove a contract index from the database. */
    bool LedgerDB::EraseContract(const uint512_t& hashTx, const uint32_t nContract)
    {
        /* Get the key pair. */
        const std::tuple<std::string, uint512_t, uint32_t> tIndex =
            std::make_tuple("validated", hashTx, nContract);

        /* Index our record to the database. */
        return Erase(tIndex);
    }


    /* Index our proofs as keychain entries to add support to read spending transaction from the proof itself. */
    void LedgerDB::IndexProofs()
    {
        /* Check for address indexing flag. */
        if(Exists(std::string("index.proofs.complete")))
        {
            /* Check there is no argument supplied. */
            if(!config::HasArg("-indexproofs"))
            {
                /* Warn that -indexheight is persistent. */
                debug::notice(FUNCTION, "-indexproofs enabled from valid indexes");

                /* Set indexing argument now. */
                RECURSIVE(config::ARGS_MUTEX);
                config::mapArgs["-indexproofs"] = "1";

                /* Cache our internal arguments. */
                config::fIndexProofs.store(true);
            }

            /* Quit if we have reindexed proofs. */
            if(!config::GetBoolArg("-forcereindexproofs"))
                return;
        }

        /* Check there is no argument supplied. */
        if(!config::fIndexProofs.load())
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
        uint32_t nContract     = 0;

        /* Track our txid and proof data. */
        uint512_t hashTx;
        uint256_t hashProof;

        /* Start our scan. */
        debug::notice(FUNCTION, "Indexing proofs from block ", state.GetHash().SubString());
        while(!config::fShutdown.load())
        {
            /* Loop through found transactions. */
            for(uint32_t nIndex = 0; nIndex < state.vtx.size(); ++nIndex)
            {
                /* Get a copy of our txid. */
                const uint512_t hash =
                    state.vtx[nIndex].second;

                /* Handle for Tritium Transactions. */
                if(state.vtx[nIndex].first == TAO::Ledger::TRANSACTION::TRITIUM)
                {
                    /* Read the transaction from disk. */
                    TAO::Ledger::Transaction tx;
                    if(!LLD::Ledger->ReadTx(hash, tx))
                        continue;

                    /* Iterate the transaction contracts. */
                    for(uint32_t nIndex = 0; nIndex < tx.Size(); ++nIndex)
                    {
                        /* Grab contract reference. */
                        const TAO::Operation::Contract& rContract = tx[nIndex];

                        /* Check for a validation index. */
                        if(TAO::Register::Unpack(rContract, hashTx, nContract))
                        {
                            /* Check that we have the contract validated. */
                            if(!LLD::Contract->HasContract(std::make_pair(hashTx, nContract)))
                                continue;

                            /* Index our record to the database. */
                            if(!IndexContract(hashTx, nContract, hash))
                                debug::warning(FUNCTION, "TRITIUM: failed to write contract index for ", hash.ToString());

                            continue;
                        }

                        /* Unpack the contract info we are working on. */
                        if(!TAO::Register::Unpack(rContract, hashProof, hashTx, nContract))
                            continue;

                        /* Check for a valid proof. */
                        if(!HasProof(hashProof, hashTx, nContract))
                            continue;

                        /* Index our record to the database. */
                        if(!IndexProof(hashProof, hashTx, nContract, hash))
                            debug::warning(FUNCTION, "TRITIUM: failed to write index proof for ", hash.ToString());
                    }
                }

                /* Handle for Legacy Transactions. */
                if(state.vtx[nIndex].first == TAO::Ledger::TRANSACTION::LEGACY)
                {
                    /* Read the transaction from disk. */
                    Legacy::Transaction tx;
                    if(!LLD::Legacy->ReadTx(hash, tx))
                        continue;

                    /* Iterate the transaction contracts. */
                    for(const auto& in : tx.vin)
                    {
                        const std::pair<uint512_t, uint32_t> tIndex =
                            std::make_pair(in.prevout.hash, in.prevout.n);

                        /* Check for double spends. */
                        if(!LLD::Legacy->IsSpent(in.prevout.hash, in.prevout.n))
                            continue;

                        /* Index our spend into legacy database. */
                        if(!LLD::Legacy->IndexSpend(in.prevout.hash, in.prevout.n, hash))
                            debug::warning(FUNCTION, "LEGACY: failed to write index proof for ", hash.ToString());
                    }
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
            {
                /* Write our last index now. */
                Write(std::string("index.proofs.complete"));

                debug::notice(FUNCTION, "Complated scanning ", nScannedCount, " tx in ", timer.Elapsed(), " seconds");

                break;
            }
        }
    }


    /* Writes a block state object to disk. */
    bool LedgerDB::WriteBlock(const uint1024_t& hashBlock, const TAO::Ledger::BlockState& state)
    {
        return Write(hashBlock, state, "block");
    }


    /* Reads a block state object from disk. */
    bool LedgerDB::ReadBlock(const uint1024_t& hashBlock, TAO::Ledger::BlockState &state)
    {
        /* Check for client mode. */
        if(config::fClient.load())
        {
            /* Get the merkle transaction from disk. */
            TAO::Ledger::ClientBlock block;
            if(!Client->ReadBlock(hashBlock, block))
                return false;

            /* Set the return value. */
            state = block;
            return true;
        }

        return Read(hashBlock, state);
    }


    /* Reads a block state object from disk for an atomic object. */
    bool LedgerDB::ReadBlock(const uint1024_t& hashBlock, memory::atomic<TAO::Ledger::BlockState> &atomicState)
    {
        /* Check for client mode. */
        if(config::fClient.load())
        {
            /* Get the merkle transaction from disk. */
            TAO::Ledger::ClientBlock block;
            if(!Client->ReadBlock(hashBlock, block))
                return false;

            /* Set the return value. */
            atomicState.store(block);
            return true;
        }

        TAO::Ledger::BlockState state;
        if(!Read(hashBlock, state))
            return false;

        atomicState.store(state);
        return true;
    }


    /* Checks if there is a block state object on disk. */
    bool LedgerDB::HasBlock(const uint1024_t& hashBlock)
    {
        /* Check for client mode. */
        if(config::fClient.load())
            return Client->HasBlock(hashBlock);

        return Exists(hashBlock);
    }


    /* Erase a block from disk. */
    bool LedgerDB::EraseBlock(const uint1024_t& hashBlock)
    {
        /* Check for client mode. */
        if(config::fClient.load())
            return Client->Erase(hashBlock);

        return Erase(hashBlock);
    }


    /* Checks if a genesis transaction exists. */
    bool LedgerDB::HasFirst(const uint256_t& hashGenesis, const uint8_t nFlags)
    {
        /* If the caller has requested to include mempool transactions then check there first*/
        if(nFlags == TAO::Ledger::FLAGS::MEMPOOL)
        {
            /* Check if mempool has given genesis. */
            if(TAO::Ledger::mempool.Has(hashGenesis))
                return true;
        }

        return Exists(std::make_pair(std::string("genesis"), hashGenesis));
    }


    /* Writes a genesis transaction-id to disk. */
    bool LedgerDB::WriteFirst(const uint256_t& hashGenesis, const uint512_t& hashTx)
    {
        return Write(std::make_pair(std::string("genesis"), hashGenesis), hashTx);
    }


    /* Reads a genesis transaction-id from disk. */
    bool LedgerDB::ReadFirst(const uint256_t& hashGenesis, uint512_t& hashTx)
    {
        return Read(std::make_pair(std::string("genesis"), hashGenesis), hashTx);
    }


    /* Erases a genesis-id from disk. */
    bool LedgerDB::EraseFirst(const uint256_t& hashGenesis)
    {
        return Erase(std::make_pair(std::string("genesis"), hashGenesis));
    }


    /* Begin a memory transaction following ACID properties. */
    void LedgerDB::MemoryBegin(const uint8_t nFlags)
    {
        LOCK(MEMORY_MUTEX);

        /* Check for miner. */
        if(nFlags == TAO::Ledger::FLAGS::MINER)
        {
            /* Set the pre-commit memory mode. */
            if(pMiner)
                delete pMiner;

            pMiner = new LedgerTransaction();

            return;
        }

        /* Set the pre-commit memory mode. */
        if(pMemory)
            delete pMemory;

        pMemory = new LedgerTransaction();
    }


    /* Abort a memory transaction following ACID properties. */
    void LedgerDB::MemoryRelease(const uint8_t nFlags)
    {
        LOCK(MEMORY_MUTEX);

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
    void LedgerDB::MemoryCommit()
    {
        LOCK(MEMORY_MUTEX);

        /* Abort the current memory mode. */
        if(pMemory)
        {
            /* Loop through all new states and apply to commit data. */
            for(const auto& claim : pMemory->mapClaims)
                pCommit->mapClaims[claim.first] = claim.second;

            /* Loop through values to erase. */
            for(const auto& erase : pMemory->setEraseClaims)
                pCommit->mapClaims.erase(erase);

            /* Loop through all new states and apply to commit data. */
            for(const auto& proof : pMemory->setProofs)
                pCommit->setProofs.insert(proof);

            /* Loop through values to erase. */
            for(const auto& erase : pMemory->setEraseProofs)
                pCommit->setProofs.erase(erase);

            /* Free the memory. */
            delete pMemory;
            pMemory = nullptr;
        }
    }

}
