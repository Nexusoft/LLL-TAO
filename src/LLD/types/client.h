/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLD_INCLUDE_CLIENT_H
#define NEXUS_LLD_INCLUDE_CLIENT_H

#include <LLC/types/uint1024.h>

#include <LLD/templates/sector.h>
#include <LLD/cache/binary_lru.h>
#include <LLD/keychain/hashmap.h>

#include <TAO/Ledger/include/enum.h>

#include <Util/include/memory.h>

/** Forward declarations **/
namespace Legacy { class MerkleTx; }
namespace TAO
{
    namespace Ledger
    {
        class MerkleTx;
        class ClientBlock;
    }
}


namespace LLD
{

  /** LocalDB
   *
   *  Database class for storing local wallet transactions.
   *
   **/
    class ClientDB : public SectorDatabase<BinaryHashMap, BinaryLRU>
    {
    public:

        /** The Database Constructor. To determine file location and the Bytes per Record. **/
        ClientDB(const uint8_t nFlagsIn = FLAGS::CREATE | FLAGS::WRITE,
            const uint32_t nBucketsIn = 77773, const uint32_t nCacheIn = 1024 * 1024);


        /** Default Destructor **/
        virtual ~ClientDB();


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


        /** WriteTx
         *
         *  Writes a transaction to the client DB.
         *
         *  @param[in] hashTx The txid of transaction to write.
         *  @param[in] tx The transaction object to write.
         *
         *  @return True if the transaction was successfully written, false otherwise.
         *
         **/
        bool WriteTx(const uint512_t& hashTx, const TAO::Ledger::MerkleTx& tx);


        /** ReadTx
         *
         *  Reads a transaction from the client DB.
         *
         *  @param[in] hashTx The txid of transaction to read.
         *  @param[out] tx The transaction object to read.
         *  @param[in] nFlags The flags to determine memory pool or disk
         *
         *  @return True if the transaction was successfully read, false otherwise.
         *
         **/
        bool ReadTx(const uint512_t& hashTx, TAO::Ledger::MerkleTx &tx, const uint8_t nFlags = TAO::Ledger::FLAGS::BLOCK);


        /** WriteTx
         *
         *  Writes a transaction to the client DB.
         *
         *  @param[in] hashTx The txid of transaction to write.
         *  @param[in] tx The transaction object to write.
         *
         *  @return True if the transaction was successfully written, false otherwise.
         *
         **/
        bool WriteTx(const uint512_t& hashTx, const Legacy::MerkleTx& tx);


        /** ReadTx
         *
         *  Reads a transaction from the client DB.
         *
         *  @param[in] hashTx The txid of transaction to read.
         *  @param[out] tx The transaction object to read.
         *  @param[in] nFlags The flags to determine memory pool or disk
         *
         *  @return True if the transaction was successfully read, false otherwise.
         *
         **/
        bool ReadTx(const uint512_t& hashTx, Legacy::MerkleTx &tx, const uint8_t nFlags = TAO::Ledger::FLAGS::BLOCK);


        /** HasTx
         *
         *  Checks client DB if a transaction exists.
         *
         *  @param[in] hashTx The txid of transaction to check.
         *  @param[in] nFlags The flags to determine memory pool or disk
         *
         *  @return True if the transaction exists, false otherwise.
         *
         **/
        bool HasTx(const uint512_t& hashTx, const uint8_t nFlags = TAO::Ledger::FLAGS::BLOCK);



        /** WriteBlock
         *
         *  Writes a client block object to disk.
         *
         *  @param[in] hashBlock The block hash to write as.
         *  @param[in] block The client block object to write.
         *
         *  @return True if the write was successful, false otherwise.
         *
         **/
        bool WriteBlock(const uint1024_t& hashBlock, const TAO::Ledger::ClientBlock& block);


        /** ReadBlock
         *
         *  Reads a client block object from disk.
         *
         *  @param[in] hashBlock The block hash to read.
         *  @param[out] block The client block object to read.
         *
         *  @return True if the read was successful, false otherwise.
         *
         **/
        bool ReadBlock(const uint1024_t& hashBlock, TAO::Ledger::ClientBlock &block);


        /** HasBlock
         *
         *  Checks if a client block exisets on disk.
         *
         *  @param[in] hashBlock The block hash to read.
         *
         *  @return True if the block exists.
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
         *
         *  @return True if the genesis exists, false otherwise.
         *
         **/
        bool HasFirst(const uint256_t& hashGenesis);


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
         *  @param[in] nFlags Determines if mempool transactions should be included (MEMPOOL) or only those in a block (BLOCK)
         *
         *  @return True if the last was successfully read, false otherwise.
         *
         **/
        bool ReadLast(const uint256_t& hashGenesis, uint512_t& hashLast, const uint8_t nFlags = TAO::Ledger::FLAGS::BLOCK);

    };
}

#endif
