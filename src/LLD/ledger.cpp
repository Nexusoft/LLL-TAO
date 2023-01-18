/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2021

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
            {
                /* Check for -client mode or active server object. */
                if(!LLP::TRITIUM_SERVER || !config::fClient.load())
                    throw debug::exception(FUNCTION, "failed to read contract");

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
                                throw debug::exception(FUNCTION, "no connections available...");

                        }
                    }
                }

                /* Debug output to console. */
                debug::log(1, FUNCTION, "CLIENT MODE: Requesting ACTION::GET::DEPENDANT for ", hashTx.SubString());
                pConnection->BlockingLookup
                (
                    5000,
                    LLP::LookupNode::REQUEST::DEPENDANT,
                    uint8_t(LLP::LookupNode::SPECIFIER::TRITIUM), hashTx
                );
                debug::log(1, FUNCTION, "CLIENT MODE: TYPES::DEPENDANT received for ", hashTx.SubString());

                /* Recursively process once we have done the lookup. */
                return ReadContract(hashTx, nContract, nFlags);
            }

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
            {
                /* Check for -client mode or active server object. */
                if(!LLP::TRITIUM_SERVER || !config::fClient.load())
                    throw debug::exception(FUNCTION, "failed to read contract");

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
                                throw debug::exception(FUNCTION, "no connections available...");

                        }
                    }
                }

                /* Debug output to console. */
                debug::log(1, FUNCTION, "CLIENT MODE: Requesting GET::DEPENDANT::LEGACY for ", hashTx.SubString());
                pConnection->BlockingLookup
                (
                    5000,
                    LLP::LookupNode::REQUEST::DEPENDANT,
                    uint8_t(LLP::LookupNode::SPECIFIER::LEGACY), hashTx
                );
                debug::log(1, FUNCTION, "CLIENT MODE: TYPES::DEPENDANT::LEGACY received for ", hashTx.SubString());

                /* Recursively process once we have done the lookup. */
                return ReadContract(hashTx, nContract, nFlags);
            }

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
            return nConfirms >= TAO::Ledger::MaturityCoinBase((pblock ? *pblock : TAO::Ledger::ChainState::stateBest.load()));

        /* Switch for coinstake. */
        else if(tx.IsCoinStake())
            return nConfirms >= TAO::Ledger::MaturityCoinStake((pblock ? *pblock : TAO::Ledger::ChainState::stateBest.load()));

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
            return Client->HasIndex(hashTx);

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
        const std::tuple<uint256_t, uint512_t, uint32_t> tuple = std::make_tuple(hashProof, hashTx, nContract);

        /* Memory mode for pre-database commits. */
        if(nFlags == TAO::Ledger::FLAGS::MEMPOOL)
        {
            LOCK(MEMORY_MUTEX);

            /* Check for pending transactions. */
            if(pMemory)
            {
                /* Write proof to memory. */
                pMemory->setEraseProofs.erase(tuple);
                pMemory->setProofs.insert(tuple);

                return true;
            }

            /* Write proof to commited memory. */
            pCommit->setProofs.insert(tuple);

            return true;
        }
        else if(nFlags == TAO::Ledger::FLAGS::MINER)
        {
            LOCK(MEMORY_MUTEX);

            /* Check for pending transactions. */
            if(pMiner)
                pMiner->setProofs.insert(tuple);

            return true;
        }
        else if(nFlags == TAO::Ledger::FLAGS::BLOCK || nFlags == TAO::Ledger::FLAGS::ERASE)
        {
            LOCK(MEMORY_MUTEX);

            /* Erase memory proof if they exist. */
            if(pMemory)
            {
                pMemory->setEraseProofs.insert(tuple);
                pMemory->setProofs.erase(tuple);
            }
            else
               pCommit->setProofs.erase(tuple);

            /* Check for erase to short circuit out. */
            if(nFlags == TAO::Ledger::FLAGS::ERASE)
                return true;
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

            /* Check pending transaction memory. */
            if(pMemory && pMemory->setProofs.count(tuple))
                return true;

            /* Check commited memory. */
            if(pCommit->setProofs.count(tuple))
                return true;
        }
        else if(nFlags == TAO::Ledger::FLAGS::MINER)
        {
            LOCK(MEMORY_MUTEX);

            /* Check pending transaction memory. */
            if(pMiner && pMiner->setProofs.count(tuple))
                return true;
        }

        //debug::log(0, FUNCTION, "Checking for Proof ", hashProof.SubString(), " txid ", hashTx.SubString(), " contract ", nContract);

        return Exists(tuple);
    }


    /* Remove a temporal proof from the database. */
    bool LedgerDB::EraseProof(const uint256_t& hashProof, const uint512_t& hashTx,
                              const uint32_t nContract, const uint8_t nFlags)
    {
        /* Get the key pair. */
        std::tuple<uint256_t, uint512_t, uint32_t> tuple = std::make_tuple(hashProof, hashTx, nContract);

        /* Check for memory transaction. */
        if(nFlags == TAO::Ledger::FLAGS::MEMPOOL)
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

        return Erase(tuple);
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
