/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2021

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLD_INCLUDE_LEDGER_H
#define NEXUS_LLD_INCLUDE_LEDGER_H

#include <LLC/types/uint1024.h>

#include <LLD/templates/sector.h>
#include <LLD/cache/binary_lru.h>
#include <LLD/keychain/hashmap.h>

#include <TAO/Operation/types/contract.h>

#include <TAO/Register/types/state.h>

#include <TAO/Ledger/include/enum.h>

#include <Util/include/memory.h>

#include <tuple>


/** Forward declarations **/
namespace TAO
{
    namespace Ledger
    {
        class BlockState;
        class Transaction;
    }

    namespace API { class Transaction; }
}


namespace LLD
{

    /** LedgerTransaction
     *
     *  Helper class for managing memory states in register database.
     *
     **/
    class LedgerTransaction
    {
    public:

        /** Collection of proofs to be written to database. **/
        std::set<std::tuple<uint256_t, uint512_t, uint32_t>> setProofs;
        std::set<std::tuple<uint256_t, uint512_t, uint32_t>> setEraseProofs;


        /** Collection of claims to be written to database. **/
        std::map<std::pair<uint512_t, uint32_t>, uint64_t> mapClaims;
        std::set<std::pair<uint512_t, uint32_t>>           setEraseClaims;

    };


    /** LedgerDB
     *
     *  The database class for the Ledger Layer.
     *
     **/
    class LedgerDB : public SectorDatabase<BinaryHashMap, BinaryLRU>
    {

        /** Mutex to lock internally when accessing memory mode. **/
        std::mutex MEMORY_MUTEX;


        /** Ledger transaction to track current open transaction. **/
        LedgerTransaction* pMemory;


        /** Miner transaction to track current states for miner verification. **/
        LedgerTransaction* pMiner;


        /** Ledger transaction to keep open all commited data. **/
        LedgerTransaction* pCommit;


    public:


        /** The Database Constructor. To determine file location and the Bytes per Record. **/
        LedgerDB(const uint8_t nFlagsIn = FLAGS::CREATE | FLAGS::WRITE,
            const uint32_t nBucketsIn = 77773, const uint32_t nCacheIn = 1024 * 1024);


        /** Default Destructor **/
        virtual ~LedgerDB();


        /** WriteBestChain
         *
         *  Writes the best chain pointer to the ledger DB.
         *
         *  @param[in] hashBest The best chain hash to write.
         *
         *  @return True if the write was successful, false otherwise.
         *
         **/
        bool WriteBestChain(const uint1024_t& hashBest);


        /** ReadBestChain
         *
         *  Reads the best chain pointer from the ledger DB.
         *
         *  @param[out] hashBest The best chain hash to read.
         *
         *  @return True if the read was successful, false otherwise.
         *
         **/
        bool ReadBestChain(uint1024_t &hashBest);


        /** WriteHybridGenesis
         *
         *  Writes the hybrid chain pointer to the ledger DB.
         *
         *  @param[in] hashBest The best chain hash to write.
         *
         *  @return True if the write was successful, false otherwise.
         *
         **/
        bool WriteHybridGenesis(const uint1024_t& hashBest);


        /** ReadHybridGenesis
         *
         *  Reads the hybrid chain pointer from the ledger DB.
         *
         *  @param[out] hashBest The best chain hash to read.
         *
         *  @return True if the read was successful, false otherwise.
         *
         **/
        bool ReadHybridGenesis(uint1024_t &hashBest);


        /** ReadBestChain
         *
         *  Reads the best chain pointer from the ledger DB.
         *
         *  @param[out] atomicBest The best chain hash to read in atomic form.
         *
         *  @return True if the read was successful, false otherwise.
         *
         **/
        bool ReadBestChain(memory::atomic<uint1024_t> &atomicBest);


        /** ReadContract
         *
         *  Reads a contract from the ledger DB.
         *
         *  @param[in] hashTx The txid of transaction to read.
         *  @param[in] nContract The contract output to read.
         *  @param[in] nFlags The flags to determine memory pool or disk
         *
         *  @return The contract object that was read.
         *
         **/
        const TAO::Operation::Contract ReadContract(const uint512_t& hashTx,
                                              const uint32_t nContract, const uint8_t nFlags = TAO::Ledger::FLAGS::BLOCK);


        /** ReadTx
         *
         *  Reads a transaction from the ledger DB.
         *
         *  @param[in] hashTx The txid of transaction to read.
         *  @param[in] nFlags The flags to determine memory pool or disk
         *
         *  @return The transaction read from disk.
         *
         **/
        const TAO::Ledger::Transaction ReadTx(const uint512_t& hashTx, const uint8_t nFlags = TAO::Ledger::FLAGS::BLOCK);


        /** WriteTx
         *
         *  Writes a transaction to the ledger DB.
         *
         *  @param[in] hashTx The txid of transaction to write.
         *  @param[in] tx The transaction object to write.
         *
         *  @return True if the transaction was successfully written, false otherwise.
         *
         **/
        bool WriteTx(const uint512_t& hashTx, const TAO::Ledger::Transaction& tx);


        /** ReadTx
         *
         *  Reads a transaction from the ledger DB.
         *
         *  @param[in] hashTx The txid of transaction to read.
         *  @param[out] tx The transaction object to read.
         *  @param[in] nFlags The flags to determine memory pool or disk
         *
         *  @return True if the transaction was successfully read, false otherwise.
         *
         **/
        bool ReadTx(const uint512_t& hashTx, TAO::Ledger::Transaction &tx, const uint8_t nFlags = TAO::Ledger::FLAGS::BLOCK);


        /** ReadTx
         *
         *  Reads a transaction from the ledger DB and casts it to an API::Transaction type.
         *
         *  @param[in] hashTx The txid of transaction to read.
         *  @param[out] tx The transaction object to read.
         *  @param[in] nFlags The flags to determine memory pool or disk
         *
         *  @return True if the transaction was successfully read, false otherwise.
         *
         **/
        bool ReadTx(const uint512_t& hashTx, TAO::API::Transaction &tx, const uint8_t nFlags = TAO::Ledger::FLAGS::BLOCK);


        /** ReadTx
         *
         *  Reads a transaction from the ledger DB including checking conflicted memory.
         *
         *  @param[in] hashTx The txid of transaction to read.
         *  @param[out] tx The transaction object to read.
         *  @param[out] fConflicted The flags to determine if transaction is conflicted.
         *  @param[in] nFlags The flags to determine memory pool or disk
         *
         *  @return True if the transaction was successfully read, false otherwise.
         *
         **/
        bool ReadTx(const uint512_t& hashTx, TAO::Ledger::Transaction &tx, bool &fConflicted, const uint8_t nFlags = TAO::Ledger::FLAGS::BLOCK);


        /** ReadTx
         *
         *  Reads a proof spending tx. Proofs are used to keep track of spent temporal proofs.
         *
         *  @param[in] hashProof The proof that is being spent.
         *  @param[in] hashTx The transaction hash that proof is being spent for.
         *  @param[in] nContract The contract that proof is for
         *  @param[out] tx The transaction object to read.
         *
         *  @return True if the last was successfully read, false otherwise.
         *
         **/
        bool ReadTx(const uint256_t& hashProof, const uint512_t& hashTx, const uint32_t nContract, TAO::Ledger::Transaction &tx);


        /** ReadTx
         *
         *  Reads a contract spending tx. Contracts are used to keep track of contract validators.
         *
         *  @param[in] hashTx The transaction hash that proof is being spent for.
         *  @param[in] nContract The contract that proof is for
         *  @param[out] tx The transaction object to read.
         *
         *  @return True if the last was successfully read, false otherwise.
         *
         **/
        bool ReadTx(const uint512_t& hashTx, const uint32_t nContract, TAO::Ledger::Transaction &tx);


        /** EraseTx
         *
         *  Erases a transaction from the ledger DB.
         *
         *  @param[in] hashTx The txid of transaction to erase.
         *
         *  @return True if the transaction was successfully erased, false otherwise.
         *
         **/
        bool EraseTx(const uint512_t& hashTx);


        /** WriteClaimed
         *
         *  Writes a partial to the ledger DB.
         *
         *  @param[in] hashTx The txid of transaction to write.
         *  @param[in] nContract The contract partial payment.
         *  @param[in] nClaimed The partial amount to update.
         *  @param[in] nFlags The flags to determine memory pool or disk
         *
         *  @return True if the partial was successfully written.
         *
         **/
        bool WriteClaimed(const uint512_t& hashTx, const uint32_t nContract, const uint64_t nClaimed,
                          const uint8_t nFlags = TAO::Ledger::FLAGS::BLOCK);


        /** ReadClaimed
         *
         *  Read a partial to the ledger DB.
         *
         *  @param[in] hashTx The txid of transaction to read.
         *  @param[in] nContract The contract partial payment.
         *  @param[in] nClaimed The partial amount to read.
         *  @param[in] nFlags The flags to determine memory pool or disk
         *
         *  @return True if the partial was successfully read.
         *
         **/
        bool ReadClaimed(const uint512_t& hashTx, const uint32_t nContract, uint64_t& nClaimed,
                         const uint8_t nFlags = TAO::Ledger::FLAGS::BLOCK);


        /** ReadMature
         *
         *  Read if a transaction is mature.
         *
         *  @param[in] hashTx The hash of the transaction to determine if it is mature.
         *
         *  @return True if txid was successfully read.
         *
         **/
        bool ReadMature(const uint512_t &hashTx, const TAO::Ledger::BlockState* pblock = nullptr);


        /** ReadMature
         *
         *  Read if a transaction is mature.
         *
         *  @param[in] tx The transaction to determine if it is mature.
         *
         *  @return True if txid was successfully read.
         *
         **/
        bool ReadMature(const TAO::Ledger::Transaction& tx, const TAO::Ledger::BlockState* pblock = nullptr);


        /** ReadConfirmations
         *
         *  Read the number of confirmations a transaction has.
         *
         *  @param[in] hashTx The transaction to determine if it is mature.
         *  @param[out] nConfirms The number of confirmations.
         *
         *  @return True if txid was successfully read.
         *
         **/
        bool ReadConfirmations(const uint512_t &hashTx, uint32_t &nConfirms, const TAO::Ledger::BlockState* pblock = nullptr);


        /** HasIndex
         *
         *  Determine if a transaction has already been indexed.
         *
         *  @param[in] hashTx The txid of transaction to write.
         *
         *  @return True if the transaction was successfully written, false otherwise.
         *
         **/
        bool HasIndex(const uint512_t& hashTx);


        /** IndexBlock
         *
         *  Index a transaction hash to a block in keychain.
         *
         *  @param[in] hashTx The txid of transaction to write.
         *  @param[in] hashBlock The block hash to index to.
         *
         *  @return True if the transaction was successfully written, false otherwise.
         *
         **/
        bool IndexBlock(const uint512_t& hashTx, const uint1024_t& hashBlock);


        /** IndexBlock
         *
         *  Index a block height to a block in keychain.
         *
         *  @param[in] hashTx The txid of transaction to write.
         *  @param[in] nBlockHeight The block height to index to.
         *
         *  @return True if the transaction was successfully written, false otherwise.
         *
         **/
        bool IndexBlock(const uint32_t& nBlockHeight, const uint1024_t& hashBlock);


        /** EraseIndex
         *
         *  Erase a foreign index form the keychain
         *
         *  @param[in] hashTx The txid of the index to erase.
         *
         *  @return True if the index was erased, false otherwise.
         *
         **/
        bool EraseIndex(const uint512_t& hashTx);

        /** EraseIndex
         *
         *  Erase a foreign index form the keychain
         *
         *  @param[in] nBlockHeight The block height of the index to erase.
         *
         *  @return True if the index was erased, false otherwise.
         *
         **/
        bool EraseIndex(const uint32_t& nBlockHeight);


        /** ReadBlock
         *
         *  Reads a block state from disk from a tx index.
         *
         *  @param[in] hashBlock The block hash to read.
         *  @param[out] state The block state object to read.
         *
         *  @return True if the read was successful, false otherwise.
         *
         **/
        bool ReadBlock(const uint512_t& hashTx, TAO::Ledger::BlockState &state);


        /** ReadBlock
         *
         *  Reads a block state from disk from a tx index.
         *
         *  @param[in] nBlockHeight The block height to read.
         *  @param[out] state The block state object to read.
         *
         *  @return True if the read was successful, false otherwise.
         *
         **/
        bool ReadBlock(const uint32_t& nBlockHeight, TAO::Ledger::BlockState &state);


        /** HasTx
         *
         *  Checks LedgerDB if a transaction exists.
         *
         *  @param[in] hashTx The txid of transaction to check.
         *  @param[in] nFlags The flags to determine memory pool or disk
         *
         *  @return True if the transaction exists, false otherwise.
         *
         **/
        bool HasTx(const uint512_t& hashTx, const uint8_t nFlags = TAO::Ledger::FLAGS::BLOCK);


        /** WriteSequence
         *
         *  Writes a new sequence event to the ledger database.
         *
         *  @param[in] hashAddress The event address to write.
         *  @param[in] nSequence The sequence to write.
         *
         *  @return True if the write was successful.
         *
         **/
        bool WriteSequence(const uint256_t& hashAddress, const uint32_t nSequence);


        /** ReadSequence
         *
         *  Reads a new sequence from the ledger database
         *
         *  @param[in] hashAddress The event address to read.
         *  @param[out] nSequence The sequence to read.
         *
         *  @return True if the write was successful.
         *
         **/
        bool ReadSequence(const uint256_t& hashAddress, uint32_t &nSequence);


        /** WriteEvent
         *
         *  Write a new event to the ledger database of current txid.
         *
         *  @param[in] hashAddress The event address to write.
         *  @param[in] hashTx The transaction event is triggering.
         *
         *  @return True if the write was successful.
         *
         **/
        bool WriteEvent(const uint256_t& hashAddress, const uint512_t& hashTx);


        /** EraseEvent
         *
         *  Erase an event from the ledger database
         *
         *  @param[in] hashAddress The event address to write.
         *
         *  @return True if the write was successful.
         *
         **/
        bool EraseEvent(const uint256_t& hashAddress);


        /** HasEvent
         *
         *  Checks an event in the ledger database of foreign index.
         *
         *  @param[in] hashAddress The event address to write.
         *  @param[in] nSequence The sequence number to read from event.
         *
         *  @return True if the event exists.
         *
         **/
        bool HasEvent(const uint256_t& hashAddress, const uint32_t nSequence);


        /** ReadEvent
         *
         *  Reads a new event to the ledger database of foreign index.
         *  This is responsible for knowing foreign sigchain events that correlate to your own.
         *
         *  @param[in] hashAddress The event address to write.
         *  @param[in] nSequence The sequence number to read from event.
         *  @param[out] tx The transaction to read event from.
         *
         *  @return True if the write was successful.
         *
         **/
        bool ReadEvent(const uint256_t& hashAddress, const uint32_t nSequence, TAO::Ledger::Transaction &tx);


        /** ReadLastEvent
         *
         *  Reads the last event (highest sequence number) for the sig chain / register
         *
         *  @param[in] hashAddress The event address to read.
         *  @param[out] hashLast The last hash (txid) to read.
         *
         *  @return True if the write was successful.
         *
         **/
        bool ReadLastEvent(const uint256_t& hashAddress, uint512_t& hashLast);


        /** WriteLast
         *
         *  Writes the last txid of sigchain to disk indexed by genesis.
         *
         *  @param[in] hashGenesis The genesis hash to write.
         *  @param[in] hashLast The last hash (txid) to write.
         *
         *  @return True if the last was successfully written, false otherwise.
         *
         **/
        bool WriteLast(const uint256_t& hashGenesis, const uint512_t& hashLast);


        /** EraseLast
         *
         *  Erase the last txid of sigchain to disk indexed by genesis.
         *
         *  @param[in] hashGenesis The genesis hash to erase.
         *
         *  @return True if the last was successfully written, false otherwise.
         *
         **/
        bool EraseLast(const uint256_t& hashGenesis);


        /** ReadLast
         *
         *  Reads the last txid of sigchain to disk indexed by genesis.
         *
         *  @param[in] hashGenesis The genesis hash to read.
         *  @param[out] hashLast The last hash (txid) to read.
         *  @param[in] nFlags Determines if mempool transactions should be included (MEMPOOL) or only those in a block (BLOCK)
         *
         *  @return True if the last was successfully read, false otherwise.
         *
         **/
        bool ReadLast(const uint256_t& hashGenesis, uint512_t& hashLast, const uint8_t nFlags = TAO::Ledger::FLAGS::BLOCK);


        /** WriteStake
         *
         *  Writes the last stake transaction of sigchain to disk indexed by genesis.
         *
         *  @param[in] hashGenesis The genesis hash to write.
         *  @param[in] hashLast The last stake hash (txid) to write.
         *
         *  @return True if successfully written, false otherwise.
         *
         **/
        bool WriteStake(const uint256_t& hashGenesis, const uint512_t& hashLast);


        /** EraseStake
         *
         *  Erase the last stake transaction of sigchain.
         *
         *  @param[in] hashGenesis The genesis hash to erase.
         *
         *  @return True if successfully erased, false otherwise.
         *
         **/
        bool EraseStake(const uint256_t& hashGenesis);


        /** ReadStake
         *
         *  Reads the last stake transaction of sigchain.
         *
         *  @param[in] hashGenesis The genesis hash to read.
         *  @param[out] hashLast The last stake hash (txid)
         *  @param[in] nFlags Determines if mempool transactions should be included (MEMPOOL) or only those in a block (BLOCK)
         *
         *  @return True if successfully read, false otherwise.
         *
         **/
        bool ReadStake(const uint256_t& hashGenesis, uint512_t& hashLast, const uint8_t nFlags = TAO::Ledger::FLAGS::BLOCK);


        /** WriteProof
         *
         *  Writes a proof to disk. Proofs are used to keep track of spent temporal proofs.
         *
         *  @param[in] hashProof The proof that is being spent.
         *  @param[in] hashTx The transaction hash that proof is being spent for.
         *  @param[in] nContract The contract that proof is for
         *  @param[in] nFlags Flags to detect if in memory mode (MEMPOOL) or disk mode (WRITE).
         *
         *  @return True if the last was successfully written, false otherwise.
         *
         **/
        bool WriteProof(const uint256_t& hashProof, const uint512_t& hashTx,
                        const uint32_t nContract, const uint8_t nFlags = TAO::Ledger::FLAGS::BLOCK);


        /** IndexProof
         *
         *  Indexes a proof to disk tied to spending transactions. Proofs are used to keep track of spent temporal proofs.
         *
         *  @param[in] hashProof The proof that is being spent.
         *  @param[in] hashTx The transaction hash that proof is spending.
         *  @param[in] nContract The contract that proof is for
         *  @param[in] hashIndex The transaction hash that spent this proof.
         *  @param[in] nFlags Flags to detect if in memory mode (MEMPOOL) or disk mode (WRITE).
         *
         *  @return True if the last was successfully written, false otherwise.
         *
         **/
        bool IndexProof(const uint256_t& hashProof, const uint512_t& hashTx,
                        const uint32_t nContract, const uint512_t& hashIndex, const uint8_t nFlags = TAO::Ledger::FLAGS::BLOCK);


        /** HasProof
         *
         *  Checks if a proof exists. Proofs are used to keep track of spent temporal proofs.
         *
         *  @param[in] hashProof The proof that is being spent.
         *  @param[in] hashTx The transaction hash that proof is being spent for.
         *  @param[in] nContract The contract that proof is for
         *  @param[in] nFlags Flags to detect if in memory mode (MEMPOOL) or disk mode (WRITE)
         *
         *  @return True if the last was successfully read, false otherwise.
         *
         **/
        bool HasProof(const uint256_t& hashProof, const uint512_t& hashTx,
                      const uint32_t nContract, const uint8_t nFlags = TAO::Ledger::FLAGS::BLOCK);


        /** EraseProof
         *
         *  Remove a temporal proof from the database.
         *
         *  @param[in] hashProof The proof that is being spent.
         *  @param[in] hashTx The transaction hash that proof is being spent for.
         *  @param[in] nContract The contract that proof is for
         *  @param[in] nFlags Flags to detect if in memory mode (MEMPOOL) or disk mode (WRITE)
         *
         *  @return True if the last was successfully read, false otherwise.
         *
         **/
        bool EraseProof(const uint256_t& hashProof, const uint512_t& hashTx,
                        const uint32_t nContract, const uint8_t nFlags = TAO::Ledger::FLAGS::BLOCK);


        /** IndexContract
         *
         *  Indexes a contract to disk tied to validating transactions
         *
         *  @param[in] hashTx The transaction hash that proof is spending.
         *  @param[in] nContract The contract that proof is for
         *  @param[in] hashIndex The transaction hash that spent this proof.
         *
         *  @return True if the last was successfully written, false otherwise.
         *
         **/
        bool IndexContract(const uint512_t& hashTx, const uint32_t nContract, const uint512_t& hashIndex);


        /** EraseContract
         *
         *  Remove a contract index from the database.
         *
         *  @param[in] hashProof The proof that is being spent.
         *  @param[in] hashTx The transaction hash that proof is being spent for.
         *  @param[in] nContract The contract that proof is for
         *
         *  @return True if the last was successfully read, false otherwise.
         *
         **/
        bool EraseContract(const uint512_t& hashTx, const uint32_t nContract);


        /** IndexProofs
         *
         *  Index our proofs as keychain entries to add support to read spending transaction from the proof itself
         *
         **/
        void IndexProofs();


        /** WriteBlock
         *
         *  Writes a block state object to disk.
         *
         *  @param[in] hashBlock The block hash to write as.
         *  @param[in] state The block state object to write.
         *
         *  @return True if the write was successful, false otherwise.
         *
         **/
        bool WriteBlock(const uint1024_t& hashBlock, const TAO::Ledger::BlockState& state);


        /** ReadBlock
         *
         *  Reads a block state object from disk.
         *
         *  @param[in] hashBlock The block hash to read.
         *  @param[out] state The block state object to read.
         *
         *  @return True if the read was successful, false otherwise.
         *
         **/
        bool ReadBlock(const uint1024_t& hashBlock, TAO::Ledger::BlockState &state);


        /** ReadBlock
         *
         *  Reads a block state object from disk for an atomic object.
         *
         *  @param[in] hashBlock The block hash to read.
         *  @param[out] atomicState The block state object to read in atomic form.
         *
         *  @return True if the read was successful, false otherwise.
         *
         **/
        bool ReadBlock(const uint1024_t& hashBlock, memory::atomic<TAO::Ledger::BlockState> &atomicState);


        /** HasBlock
         *
         *  Checks if there is a block state object on disk.
         *
         *  @param[in] hashBlock The block hash to check.
         *
         *  @return True if it exists, false otherwise.
         *
         **/
        bool HasBlock(const uint1024_t& hashBlock);


        /** EraseBlock
         *
         *  Erase a block from disk.
         *
         *  @param[in] hashBlock The block hash to erase.
         *
         *  @return True if it exists, false otherwise.
         *
         **/
        bool EraseBlock(const uint1024_t& hashBlock);


        /** HasFirst
         *
         *  Checks if a genesis transaction exists.
         *
         *  @param[in] hashGenesis The genesis ID to check for.
         *  @param[in] nFlags The flags to determine what state to check for.
         *
         *  @return True if the genesis exists, false otherwise.
         *
         **/
        bool HasFirst(const uint256_t& hashGenesis, const uint8_t nFlags = TAO::Ledger::FLAGS::BLOCK);


        /** WriteFirst
         *
         *  Writes a genesis transaction-id to disk.
         *
         *  @param[in] hashGenesis The genesis ID to write for.
         *  @param[in] hashTx The transaction-id to write for.
         *
         *  @return True if the genesis is written, false otherwise.
         *
         **/
        bool WriteFirst(const uint256_t& hashGenesis, const uint512_t& hashTx);


        /** ReadFirst
         *
         *  Reads a genesis transaction-id from disk.
         *
         *  @param[in] hashGenesis The genesis ID to read for.
         *  @param[out] hashTx The transaction-id to read for.
         *
         *  @return True if the genesis was read, false otherwise.
         *
         **/
        bool ReadFirst(const uint256_t& hashGenesis, uint512_t& hashTx);


        /** EraseFirst
         *
         *  Erases a genesis-id from disk.
         *
         *  @param[in] hashGenesis The genesis ID to check for.
         *
         *  @return True if the genesis exists, false otherwise.
         *
         **/
        bool EraseFirst(const uint256_t& hashGenesis);


        /** MemoryBegin
         *
         *  Begin a memory transaction following ACID properties.
         *
         **/
        void MemoryBegin(const uint8_t nFlags = TAO::Ledger::FLAGS::MEMPOOL);


        /** MemoryRelease
         *
         *  Release a memory transaction following ACID properties.
         *
         **/
        void MemoryRelease(const uint8_t nFlags = TAO::Ledger::FLAGS::MEMPOOL);


        /** MemoryCommit
         *
         *  Commit a memory transaction following ACID properties.
         *
         **/
        void MemoryCommit();


   };
}

#endif
