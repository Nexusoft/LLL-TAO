/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_LLD_INCLUDE_LEDGER_H
#define NEXUS_LLD_INCLUDE_LEDGER_H

#include <LLC/types/uint1024.h>

#include <LLD/include/version.h>
#include <LLD/templates/sector.h>

#include <LLD/cache/binary_lru.h>
#include <LLD/keychain/hashmap.h>

#include <TAO/Register/include/state.h>
#include <TAO/Ledger/types/transaction.h>
#include <TAO/Ledger/types/state.h>
#include <TAO/Register/include/enum.h>

namespace LLD
{

    class LedgerDB : public SectorDatabase<BinaryHashMap, BinaryLRU>
    {
        std::recursive_mutex MEMORY_MUTEX;

        std::map< std::pair<uint256_t, uint512_t>, uint32_t > mapProofs;

    public:
        /** The Database Constructor. To determine file location and the Bytes per Record. **/
        LedgerDB(uint8_t nFlags = FLAGS::CREATE | FLAGS::WRITE)
        : SectorDatabase("ledger", nFlags) { }


        bool WriteBestChain(uint1024_t hashBest)
        {
            return Write(std::string("best"), hashBest);
        }


        bool ReadBestChain(uint1024_t& hashBest)
        {
            return Read(std::string("best"), hashBest);
        }

        bool WriteTx(uint512_t hashTransaction, TAO::Ledger::Transaction tx)
        {
            return Write(std::make_pair(std::string("tx"), hashTransaction), tx);
        }


        bool ReadTx(uint512_t hashTransaction, TAO::Ledger::Transaction& tx)
        {
            return Read(std::make_pair(std::string("tx"), hashTransaction), tx);
        }


        bool HasTx(uint512_t hashTransaction)
        {
            return Exists(std::make_pair(std::string("tx"), hashTransaction));
        }


        bool WriteProof(uint256_t hashProof, uint512_t hashTransaction, uint8_t nFlags = TAO::Register::FLAGS::WRITE)
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

            uint64_t nDummy = 0; //this is being used as a boolean express. TODO: make LLD handle bool key writes
            return Write(std::make_pair(hashProof, hashTransaction), nDummy);
        }


        bool WriteBlock(uint1024_t hashBlock, TAO::Ledger::BlockState state)
        {
            return Write(std::make_pair(std::string("block"), hashBlock), state);
        }


        bool ReadBlock(uint1024_t hashBlock, TAO::Ledger::BlockState& state)
        {
            return Read(std::make_pair(std::string("block"), hashBlock), state);
        }


        bool HasProof(uint256_t hashProof, uint512_t hashTransaction, uint8_t nFlags = TAO::Register::FLAGS::WRITE)
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


        bool HasGenesis(uint256_t hashGenesis)
        {
            return Exists(std::make_pair(std::string("genesis"), hashGenesis));
        }


        bool WriteGenesis(uint256_t hashGenesis, TAO::Ledger::Transaction tx)
        {
            return Write(std::make_pair(std::string("genesis"), hashGenesis), tx);
        }


        bool ReadGenesis(uint256_t hashGenesis, TAO::Ledger::Transaction& tx)
        {
            return Read(std::make_pair(std::string("genesis"), hashGenesis), tx);
        }
    };
}

#endif
