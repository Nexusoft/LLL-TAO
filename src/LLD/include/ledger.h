/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLD_INCLUDE_LEDGER_H
#define NEXUS_LLD_INCLUDE_LEDGER_H

#include <LLC/types/uint1024.h>

#include <LLD/templates/sector.h>

#include <TAO/Operation/types/contract.h>

#include <TAO/Register/types/state.h>
#include <TAO/Register/include/enum.h>

#include <Util/include/memory.h>


/** Forward declarations **/
namespace TAO
{
    namespace Ledger
    {
        class BlockState;
        class Transaction;
    }
}


namespace LLD
{

    /** LedgerDB
     *
     *  The database class for the Ledger Layer.
     *
     **/
    class LedgerDB : public SectorDatabase<BinaryHashMap, BinaryLRU>
    {
        std::mutex MEMORY_MUTEX;

        std::map<std::pair<uint256_t, uint512_t>, uint32_t> mapProofs;


    public:


        /** The Database Constructor. To determine file location and the Bytes per Record. **/
        LedgerDB(uint8_t nFlagsIn = FLAGS::CREATE | FLAGS::WRITE);


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
        bool ReadBestChain(uint1024_t& hashBest);


        /** ReadBestChain
         *
         *  Reads the best chain pointer from the ledger DB.
         *
         *  @param[out] atomicBest The best chain hash to read in atomic form.
         *
         *  @return True if the read was successful, false otherwise.
         *
         **/
        bool ReadBestChain(memory::atomic<uint1024_t>& atomicBest);


        /** ReadContract
         *
         *  Reads a contract from the ledger DB.
         *
         *  @param[in] hashTransaction The txid of transaction to read.
         *  @param[in] nContract The contract output to read.
         *  @param[in] nFlags The flags to determine memory pool or disk
         *
         *  @return The contract object that was read.
         *
         **/
        TAO::Operation::Contract ReadContract(const uint512_t& hashTransaction, const uint32_t nContract, const uint8_t nFlags = TAO::Register::FLAGS::WRITE);


        /** WriteTx
         *
         *  Writes a transaction to the ledger DB.
         *
         *  @param[in] hashTransaction The txid of transaction to write.
         *  @param[in] tx The transaction object to write.
         *
         *  @return True if the transaction was successfully written, false otherwise.
         *
         **/
        bool WriteTx(const uint512_t& hashTransaction, const TAO::Ledger::Transaction& tx);


        /** ReadTx
         *
         *  Reads a transaction from the ledger DB.
         *
         *  @param[in] hashTransaction The txid of transaction to read.
         *  @param[in] tx The transaction object to read.
         *  @param[in] nFlags The flags to determine memory pool or disk
         *
         *  @return True if the transaction was successfully read, false otherwise.
         *
         **/
        bool ReadTx(const uint512_t& hashTransaction, TAO::Ledger::Transaction& tx, const uint8_t nFlags = TAO::Register::FLAGS::WRITE);


        /** EraseTx
         *
         *  Erases a transaction from the ledger DB.
         *
         *  @param[in] hashTransaction The txid of transaction to erase.
         *
         *  @return True if the transaction was successfully erased, false otherwise.
         *
         **/
        bool EraseTx(const uint512_t& hashTransaction);


        /** WriteClaimed
         *
         *  Writes a partial to the ledger DB.
         *
         *  @param[in] hashTransaction The txid of transaction to write.
         *  @param[in] nContract The contract partial payment.
         *  @param[in] nClaimed The partial amount to update.
         *
         *  @return True if the partial was successfully written.
         *
         **/
        bool WriteClaimed(const uint512_t& hashTransaction, const uint32_t nContract, const uint64_t nClaimed);


        /** ReadClaimed
         *
         *  Read a partial to the ledger DB.
         *
         *  @param[in] hashTransaction The txid of transaction to read.
         *  @param[in] nContract The contract partial payment.
         *  @param[in] nClaimed The partial amount to read.
         *
         *  @return True if the partial was successfully read.
         *
         **/
        bool ReadClaimed(const uint512_t& hashTransaction, const uint32_t nContract, uint64_t& nClaimed);


        /** HasIndex
         *
         *  Determine if a transaction has already been indexed.
         *
         *  @param[in] hashTransaction The txid of transaction to write.
         *
         *  @return True if the transaction was successfully written, false otherwise.
         *
         **/
        bool HasIndex(const uint512_t& hashTransaction);


        /** IndexBlock
         *
         *  Index a transaction hash to a block in keychain.
         *
         *  @param[in] hashTransaction The txid of transaction to write.
         *  @param[in] hashBlock The block hash to index to.
         *
         *  @return True if the transaction was successfully written, false otherwise.
         *
         **/
        bool IndexBlock(const uint512_t& hashTransaction, const uint1024_t& hashBlock);


        /** IndexBlock
         *
         *  Index a block height to a block in keychain.
         *
         *  @param[in] hashTransaction The txid of transaction to write.
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
         *  @param[in] hashTransaction The txid of the index to erase.
         *
         *  @return True if the index was erased, false otherwise.
         *
         **/
        bool EraseIndex(const uint512_t& hashTransaction);

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


        /** RepairIndex
         *
         *  Recover if an index is not found.
         *  Fixes a corrupted database with a linear search for the hash tx up
         *  to the chain height.
         *
         *  @param[in] hashTransaction The txid of transaction to write.
         *  @param[in] state The block state of the block the transaction belongs to.
         *
         *  @return True if the transaction was successfully written, false otherwise.
         *
         **/
        bool RepairIndex(const uint512_t& hashTransaction, const TAO::Ledger::BlockState &state);


        /** RepairIndexHeight
         *
         *  Recover the block height index.
         *  Adds or fixes th block height index by iterating forward from the genesis block
         *
         *  @return True if the index was successfully written, false otherwise.
         *
         **/
        bool RepairIndexHeight();


        /** ReadBlock
         *
         *  Reads a block state from disk from a tx index.
         *
         *  @param[in] hashBlock The block hash to read.
         *  @param[in] state The block state object to read.
         *
         *  @return True if the read was successful, false otherwise.
         *
         **/
        bool ReadBlock(const uint512_t& hashTransaction, TAO::Ledger::BlockState& state);


        /** ReadBlock
         *
         *  Reads a block state from disk from a tx index.
         *
         *  @param[in] nBlockHeight The block height to read.
         *  @param[in] state The block state object to read.
         *
         *  @return True if the read was successful, false otherwise.
         *
         **/
        bool ReadBlock(const uint32_t& nBlockHeight, TAO::Ledger::BlockState& state);


        /** HasTx
         *
         *  Checks LedgerDB if a transaction exists.
         *
         *  @param[in] hashTransaction The txid of transaction to check.
         *
         *  @return True if the transaction exists, false otherwise.
         *
         **/
        bool HasTx(const uint512_t& hashTransaction);


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
         *  @param[in] hashLast The last hash (txid) to read.
         *
         *  @return True if the last was successfully read, false otherwise.
         *
         **/
        bool ReadLast(const uint256_t& hashGenesis, uint512_t& hashLast);


        /** WriteProof
         *
         *  Writes a proof to disk. Proofs are used to keep track of spent temporal proofs.
         *
         *  @param[in] hashProof The proof that is being spent.
         *  @param[in] hashTransaction The transaction hash that proof is being spent for.
         *  @param[in] nFlags Flags to detect if in memory mode (MEMPOOL) or disk mode (WRITE).
         *
         *  @return True if the last was successfully written, false otherwise.
         *
         **/
        bool WriteProof(const uint256_t& hashProof, const uint512_t& hashTransaction, const uint64_t nContract, uint8_t nFlags = TAO::Register::FLAGS::WRITE);


        /** HasProof
         *
         *  Checks if a proof exists. Proofs are used to keep track of spent temporal proofs.
         *
         *  @param[in] hashProof The proof that is being spent.
         *  @param[in] hashTransaction The transaction hash that proof is being spent for.
         *  @param[in] nFlags Flags to detect if in memory mode (MEMPOOL) or disk mode (WRITE)
         *
         *  @return True if the last was successfully read, false otherwise.
         *
         **/
        bool HasProof(const uint256_t& hashProof, const uint512_t& hashTransaction, const uint64_t nContract, uint8_t nFlags = TAO::Register::FLAGS::WRITE);


        /** EraseProof
         *
         *  Remove a temporal proof from the database.
         *
         *  @param[in] hashProof The proof that is being spent.
         *  @param[in] hashTransaction The transaction hash that proof is being spent for.
         *  @param[in] nFlags Flags to detect if in memory mode (MEMPOOL) or disk mode (WRITE)
         *
         *  @return True if the last was successfully read, false otherwise.
         *
         **/
        bool EraseProof(const uint256_t& hashProof, const uint512_t& hashTransaction, const uint64_t nContract, uint8_t nFlags = TAO::Register::FLAGS::WRITE);


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
         *  @param[in] state The block state object to read.
         *
         *  @return True if the read was successful, false otherwise.
         *
         **/
        bool ReadBlock(const uint1024_t& hashBlock, TAO::Ledger::BlockState& state);


        /** ReadBlock
         *
         *  Reads a block state object from disk for an atomic object.
         *
         *  @param[in] hashBlock The block hash to read.
         *  @param[in] atomicState The block state object to read in atomic form.
         *
         *  @return True if the read was successful, false otherwise.
         *
         **/
        bool ReadBlock(const uint1024_t& hashBlock, memory::atomic<TAO::Ledger::BlockState>& atomicState);


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


        /** HasGenesis
         *
         *  Checks if a genesis transaction exists.
         *
         *  @param[in] hashGenesis The genesis ID to check for.
         *
         *  @return True if the genesis exists, false otherwise.
         *
         **/
        bool HasGenesis(const uint256_t& hashGenesis);


        /** WriteGenesis
         *
         *  Writes a genesis transaction-id to disk.
         *
         *  @param[in] hashGenesis The genesis ID to write for.
         *  @param[in] hashTransaction The transaction-id to write for.
         *
         *  @return True if the genesis is written, false otherwise.
         *
         **/
        bool WriteGenesis(const uint256_t& hashGenesis, const uint512_t& hashTransaction);


        /** ReadGenesis
         *
         *  Reads a genesis transaction-id from disk.
         *
         *  @param[in] hashGenesis The genesis ID to read for.
         *  @param[out] hashTransaction The transaction-id to read for.
         *
         *  @return True if the genesis was read, false otherwise.
         *
         **/
        bool ReadGenesis(const uint256_t& hashGenesis, uint512_t& hashTransaction);

    };
}

#endif
