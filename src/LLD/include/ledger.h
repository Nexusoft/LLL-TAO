/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_LLD_INCLUDE_LEDGER_H
#define NEXUS_LLD_INCLUDE_LEDGER_H

#include <LLC/types/uint1024.h>

#include <LLD/include/version.h>
#include <LLD/templates/sector.h>

#include <TAO/Register/include/state.h>
#include <TAO/Ledger/types/transaction.h>
#include <TAO/Ledger/types/trustkey.h>
#include <TAO/Ledger/types/state.h>
#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Register/include/enum.h>


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

        std::map< std::pair<uint256_t, uint512_t>, uint32_t > mapProofs;

    public:


        /** The Database Constructor. To determine file location and the Bytes per Record. **/
        LedgerDB(uint8_t nFlagsIn = FLAGS::CREATE | FLAGS::WRITE)
        : SectorDatabase(std::string("ledger"), nFlagsIn) { }


        /** Default Destructor **/
        virtual ~LedgerDB() {}


        /** WriteBestChain
         *
         *  Writes the best chain pointer to the ledger DB.
         *
         *  @param[in] hashBest The best chain hash to write.
         *
         *  @return True if the write was successful, false otherwise.
         *
         **/
        bool WriteBestChain(const uint1024_t& hashBest)
        {
            return Write(std::string("hashbestchain"), hashBest);
        }


        /** ReadBestChain
         *
         *  Reads the best chain pointer from the ledger DB.
         *
         *  @param[out] hashBest The best chain hash to read.
         *
         *  @return True if the read was successful, false otherwise.
         *
         **/
        bool ReadBestChain(uint1024_t& hashBest)
        {
            return Read(std::string("hashbestchain"), hashBest);
        }


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
        bool WriteTx(const uint512_t& hashTransaction, const TAO::Ledger::Transaction& tx)
        {
            return Write(hashTransaction, tx);
        }


        /** ReadTx
         *
         *  Reads a transaction from the ledger DB.
         *
         *  @param[in] hashTransaction The txid of transaction to read.
         *  @param[in] tx The transaction object to read.
         *
         *  @return True if the transaction was successfully read, false otherwise.
         *
         **/
        bool ReadTx(const uint512_t& hashTransaction, TAO::Ledger::Transaction& tx)
        {
            return Read(hashTransaction, tx);
        }


        /** EraseTx
         *
         *  Erases a transaction from the ledger DB.
         *
         *  @param[in] hashTransaction The txid of transaction to erase.
         *
         *  @return True if the transaction was successfully erased, false otherwise.
         *
         **/
        bool EraseTx(const uint512_t& hashTransaction)
        {
            return Erase(hashTransaction);
        }


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
        bool IndexBlock(const uint512_t& hashTransaction, const uint1024_t& hashBlock)
        {
            return Index(std::make_pair(std::string("index"), hashTransaction), hashBlock);
        }

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
        bool IndexBlock(const uint32_t& nBlockHeight, const uint1024_t& hashBlock)
        {
            return Index(std::make_pair(std::string("height"), nBlockHeight), hashBlock);
        }


        /** EraseIndex
         *
         *  Erase a foreign index form the keychain
         *
         *  @param[in] hashTransaction The txid of the index to erase.
         *
         *  @return True if the index was erased, false otherwise.
         *
         **/
        bool EraseIndex(const uint512_t& hashTransaction)
        {
            return Erase(std::make_pair(std::string("index"), hashTransaction));
        }

        /** EraseIndex
         *
         *  Erase a foreign index form the keychain
         *
         *  @param[in] nBlockHeight The block height of the index to erase.
         *
         *  @return True if the index was erased, false otherwise.
         *
         **/
        bool EraseIndex(const uint32_t& nBlockHeight)
        {
            return Erase(std::make_pair(std::string("height"), nBlockHeight));
        }


        /** RepairIndex
         *
         *  Recover if an index is not found.
         *  Fixes a corrupted database with a linear search for the hash tx up
         *  to the chain height.
         *
         *  @param[in] hashTransaction The txid of transaction to write.
         *
         *  @return True if the transaction was successfully written, false otherwise.
         *
         **/
        bool RepairIndex(const uint512_t& hashTransaction, TAO::Ledger::BlockState state)
        {
            debug::log(0, FUNCTION, "repairing index for ", hashTransaction.ToString().substr(0, 20));

            /* Loop until it is found. */
            while(!config::fShutdown.load() && !state.IsNull())
            {
                /* Give debug output of status. */
                if(state.nHeight % 100000 == 0)
                    debug::log(0, FUNCTION, "repairing index..... ", state.nHeight);

                /* Check the state vtx size. */
                if(state.vtx.size() == 0)
                    debug::error(FUNCTION, "block ", state.GetHash().ToString().substr(0, 20), " has no transactions");

                /* Check for the transaction. */
                for(const auto& tx : state.vtx)
                {
                    /* If the transaction is found, write the index. */
                    if(tx.second == hashTransaction)
                    {
                        /* Repair the index once it is found. */
                        if(!IndexBlock(hashTransaction, state.GetHash()))
                            return false;

                        return true;
                    }
                }

                state = state.Prev();
            }

            return false;
        }

        /** RepairIndexHeight
         *
         *  Recover the block height index.
         *  Adds or fixes th block height index by iterating forward from the genesis block
         *
         *  @return True if the index was successfully written, false otherwise.
         *
         **/
        bool RepairIndexHeight()
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
        bool ReadBlock(const uint512_t& hashTransaction, TAO::Ledger::BlockState& state)
        {
            return Read(std::make_pair(std::string("index"), hashTransaction), state);
        }

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
        bool ReadBlock(const uint32_t& nBlockHeight, TAO::Ledger::BlockState& state)
        {
            return Read(std::make_pair(std::string("height"), nBlockHeight), state);
        }


        /** HasTx
         *
         *  Checks LedgerDB if a transaction exists.
         *
         *  @param[in] hashTransaction The txid of transaction to check.
         *
         *  @return True if the transaction exists, false otherwise.
         *
         **/
        bool HasTx(const uint512_t& hashTransaction)
        {
            return Exists(hashTransaction);
        }


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
        bool WriteLast(const uint256_t& hashGenesis, const uint512_t& hashLast)
        {
            return Write(std::make_pair(std::string("last"), hashGenesis), hashLast);
        }


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
        bool ReadLast(const uint256_t& hashGenesis, uint512_t& hashLast)
        {
            return Read(std::make_pair(std::string("last"), hashGenesis), hashLast);
        }


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
        bool WriteProof(const uint256_t& hashProof, const uint512_t& hashTransaction, uint8_t nFlags = TAO::Register::FLAGS::WRITE)
        {
            /* Memory mode for pre-database commits. */
            if(nFlags & TAO::Register::FLAGS::MEMPOOL)
            {
                LOCK(MEMORY_MUTEX);

                /* Write the new proof state. */
                mapProofs[std::make_pair(hashProof, hashTransaction)] = 0;
                return true;
            }
            else
            {
                LOCK(MEMORY_MUTEX);

                /* Erase memory proof if they exist. */
                if(mapProofs.count(std::make_pair(hashProof, hashTransaction)))
                    mapProofs.erase(std::make_pair(hashProof, hashTransaction));
            }

            return Write(std::make_pair(hashProof, hashTransaction));
        }


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
        bool HasProof(const uint256_t& hashProof, const uint512_t& hashTransaction, uint8_t nFlags = TAO::Register::FLAGS::WRITE)
        {
            /* Memory mode for pre-database commits. */
            if(nFlags & TAO::Register::FLAGS::MEMPOOL)
            {
                LOCK(MEMORY_MUTEX);

                /* If exists in memory, return true. */
                if(mapProofs.count(std::make_pair(hashProof, hashTransaction)))
                    return true;
            }

            return Exists(std::make_pair(hashProof, hashTransaction));
        }


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
        bool WriteBlock(const uint1024_t& hashBlock, const TAO::Ledger::BlockState& state)
        {
            return Write(hashBlock, state);
        }


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
        bool ReadBlock(const uint1024_t& hashBlock, TAO::Ledger::BlockState& state)
        {
            return Read(hashBlock, state);
        }


        /** HasBlock
         *
         *  Checks if there is a block state object on disk.
         *
         *  @param[in] hashBlock The block hash to check.
         *
         *  @return True if it exists, false otherwise.
         *
         **/
        bool HasBlock(const uint1024_t& hashBlock)
        {
            return Exists(hashBlock);
        }


        /** EraseBlock
         *
         *  Erase a block from disk.
         *
         *  @param[in] hashBlock The block hash to erase.
         *
         *  @return True if it exists, false otherwise.
         *
         **/
        bool EraseBlock(const uint1024_t& hashBlock)
        {
            return Erase(hashBlock);
        }


        /** HasGenesis
         *
         *  Checks if a genesis transaction exists.
         *
         *  @param[in] hashGenesis The genesis ID to check for.
         *
         *  @return True if the genesis exists, false otherwise.
         *
         **/
        bool HasGenesis(const uint256_t& hashGenesis)
        {
            return Exists(std::make_pair(std::string("genesis"), hashGenesis));
        }


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
        bool WriteGenesis(const uint256_t& hashGenesis, const uint512_t& hashTransaction)
        {
            return Write(std::make_pair(std::string("genesis"), hashGenesis), hashTransaction);
        }


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
        bool ReadGenesis(const uint256_t& hashGenesis, uint512_t& hashTransaction)
        {
            return Read(std::make_pair(std::string("genesis"), hashGenesis), hashTransaction);
        }
    };
}

#endif
