/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/types/client.h>

#include <TAO/Ledger/types/merkle.h>
#include <TAO/Ledger/types/client.h>

#include <Legacy/types/merkle.h>

namespace LLD
{

    /** The Database Constructor. To determine file location and the Bytes per Record. **/
    ClientDB::ClientDB(const uint8_t nFlagsIn, const uint32_t nBucketsIn, const uint32_t nCacheIn)
    : SectorDatabase(std::string("_CLIENT")
    , nFlagsIn
    , nBucketsIn
    , nCacheIn)
    {
    }


    /** Default Destructor **/
    ClientDB::~ClientDB()
    {
    }


    /* Writes a transaction to the client DB. */
    bool ClientDB::WriteTx(const uint512_t& hashTx, const TAO::Ledger::MerkleTx& tx)
    {
        return Write(hashTx, tx, "tritium");
    }


    /* Reads a transaction from the client DB. */
    bool ClientDB::ReadTx(const uint512_t& hashTx, TAO::Ledger::MerkleTx &tx, const uint8_t nFlags)
    {
        /* Special check for memory pool. */
        if(nFlags == TAO::Ledger::FLAGS::MEMPOOL)
        {
            //get from client mempool
        }

        return Read(hashTx, tx);
    }


    /* Writes a transaction to the client DB. */
    bool ClientDB::WriteTx(const uint512_t& hashTx, const Legacy::MerkleTx& tx)
    {
        return Write(hashTx, tx, "legacy");
    }


    /* Reads a transaction from the client DB. */
    bool ClientDB::ReadTx(const uint512_t& hashTx, Legacy::MerkleTx &tx, const uint8_t nFlags)
    {
        /* Special check for memory pool. */
        if(nFlags == TAO::Ledger::FLAGS::MEMPOOL)
        {
            //get from client mempool
        }

        return Read(hashTx, tx);
    }


    /* Checks client DB if a transaction exists. */
    bool ClientDB::HasTx(const uint512_t& hashTx, const uint8_t nFlags)
    {
        /* Special check for memory pool. */
        if(nFlags == TAO::Ledger::FLAGS::MEMPOOL)
        {
            //get from client mempool
        }

        return Exists(hashTx);
    }


    /* Writes a block block object to disk. */
    bool ClientDB::WriteBlock(const uint1024_t& hashBlock, const TAO::Ledger::ClientBlock& block)
    {
        return Write(hashBlock, block, "block");
    }


    /* Reads a block block object from disk. */
    bool ClientDB::ReadBlock(const uint1024_t& hashBlock, TAO::Ledger::ClientBlock &block)
    {
        return Read(hashBlock, block);
    }
    

    /* Checks if a client block exisets on disk. */
    bool ClientDB::HasBlock(const uint1024_t& hashBlock)
    {
        return Exists(hashBlock);
    }


    /* Erase a block from disk. */
    bool ClientDB::EraseBlock(const uint1024_t& hashBlock)
    {
        return Erase(hashBlock);
    }


    /* Checks if a genesis transaction exists. */
    bool ClientDB::HasFirst(const uint256_t& hashGenesis)
    {
        return Exists(std::make_pair(std::string("first"), hashGenesis));
    }


    /* Writes a genesis transaction-id to disk. */
    bool ClientDB::WriteFirst(const uint256_t& hashGenesis, const uint512_t& hashTx)
    {
        return Write(std::make_pair(std::string("first"), hashGenesis), hashTx, "first");
    }


    /* Reads a genesis transaction-id from disk. */
    bool ClientDB::ReadFirst(const uint256_t& hashGenesis, uint512_t& hashTx)
    {
        return Read(std::make_pair(std::string("first"), hashGenesis), hashTx);
    }


    /* Writes the last txid of sigchain to disk indexed by genesis. */
    bool ClientDB::WriteLast(const uint256_t& hashGenesis, const uint512_t& hashLast)
    {
        return Write(std::make_pair(std::string("last"), hashGenesis), hashLast, "last");
    }


    /* Erase the last txid of sigchain to disk indexed by genesis. */
    bool ClientDB::EraseLast(const uint256_t& hashGenesis)
    {
        return Erase(std::make_pair(std::string("last"), hashGenesis));
    }


    /* Reads the last txid of sigchain to disk indexed by genesis. */
    bool ClientDB::ReadLast(const uint256_t& hashGenesis, uint512_t& hashLast, const uint8_t nFlags)
    {
        /* Special check for memory pool. */
        if(nFlags == TAO::Ledger::FLAGS::MEMPOOL)
        {
            //get from client mempool
        }

        return Read(std::make_pair(std::string("last"), hashGenesis), hashLast);
    }

}
