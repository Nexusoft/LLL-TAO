/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/types/legacy.h>

#include <TAO/Ledger/types/mempool.h>

namespace LLD
{

    /** The Database Constructor. To determine file location and the Bytes per Record. **/
    LegacyDB::LegacyDB(const uint8_t nFlagsIn, const uint32_t nBucketsIn, const uint32_t nCacheIn)
    : SectorDatabase(std::string("_LEGACY")
    , nFlagsIn
    , nBucketsIn
    , nCacheIn)
    {
    }


    /* Default Destructor */
    LegacyDB::~LegacyDB()
    {
    }


    /* Writes a transaction to the legacy DB. */
    bool LegacyDB::WriteTx(const uint512_t& hashTx, const Legacy::Transaction& tx)
    {
        return Write(std::make_pair(std::string("tx"), hashTx), tx, "tx");
    }


    /* Reads a transaction from the legacy DB. */
    bool LegacyDB::ReadTx(const uint512_t& hashTx, Legacy::Transaction& tx, bool &fConflicted, const uint8_t nFlags)
    {
        /* Special check for memory pool. */
        if(nFlags == TAO::Ledger::FLAGS::MEMPOOL || nFlags == TAO::Ledger::FLAGS::MINER)
        {
            /* Get the transaction. */
            if(TAO::Ledger::mempool.Get(hashTx, tx, fConflicted))
                return true;
        }

        return Read(std::make_pair(std::string("tx"), hashTx), tx);
    }


    /* Reads a transaction from the legacy DB. */
    bool LegacyDB::ReadTx(const uint512_t& hashTx, Legacy::Transaction& tx, const uint8_t nFlags)
    {
        /* Special check for memory pool. */
        if(nFlags == TAO::Ledger::FLAGS::MEMPOOL || nFlags == TAO::Ledger::FLAGS::MINER)
        {
            /* Get the transaction. */
            if(TAO::Ledger::mempool.Get(hashTx, tx))
                return true;
        }

        return Read(std::make_pair(std::string("tx"), hashTx), tx);
    }


    /* Erases a transaction from the ledger DB. */
    bool LegacyDB::EraseTx(const uint512_t& hashTx)
    {
        //TODO: this is never used. Might consdier removing since transactions are never erased
        return Erase(std::make_pair(std::string("tx"), hashTx));
    }


    /* Checks if a transaction exists. */
    bool LegacyDB::HasTx(const uint512_t& hashTx, const uint8_t nFlags)
    {
        /* Special check for memory pool. */
        if(nFlags == TAO::Ledger::FLAGS::MEMPOOL)
        {
            /* Get the transaction. */
            if(TAO::Ledger::mempool.Has(hashTx))
                return true;
        }

        return Exists(std::make_pair(std::string("tx"), hashTx));
    }


    /* Writes an output as spent. */
    bool LegacyDB::WriteSpend(const uint512_t& hashTx, uint32_t nOutput)
    {
        return Write(std::make_pair(hashTx, nOutput));
    }


    /* Removes a spend flag on an output. */
    bool LegacyDB::EraseSpend(const uint512_t& hashTx, uint32_t nOutput)
    {
        return Erase(std::make_pair(hashTx, nOutput));
    }


    /* Checks if an output was spent. */
    bool LegacyDB::IsSpent(const uint512_t& hashTx, uint32_t nOutput)
    {
        return Exists(std::make_pair(hashTx, nOutput));
    }

    /* Writes a new sequence event to the ledger database. */
    bool LegacyDB::WriteSequence(const uint256_t& hashAddress, const uint32_t nSequence)
    {
        return Write(std::make_pair(std::string("sequence"), hashAddress), nSequence);
    }


    /* Reads a new sequence from the ledger database */
    bool LegacyDB::ReadSequence(const uint256_t& hashAddress, uint32_t& nSequence)
    {
        return Read(std::make_pair(std::string("sequence"), hashAddress), nSequence);
    }


    /* Write a new event to the ledger database of current txid. */
    bool LegacyDB::WriteEvent(const uint256_t& hashAddress, const uint512_t& hashTx)
    {
        /* Get the current sequence number. */
        uint32_t nSequence = 0;
        if(!ReadSequence(hashAddress, nSequence))
            nSequence = 0; //reset value just in case

        /* Write the new sequence number iterated by one. */
        if(!WriteSequence(hashAddress, nSequence + 1))
            return false;

        return Index(std::make_pair(hashAddress, nSequence), std::make_pair(std::string("tx"), hashTx));
    }


    /* Erase an event from the ledger database. */
    bool LegacyDB::EraseEvent(const uint256_t& hashAddress)
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
    bool LegacyDB::ReadEvent(const uint256_t& hashAddress, const uint32_t nSequence, Legacy::Transaction& tx)
    {
        return Read(std::make_pair(hashAddress, nSequence), tx);
    }


    /* Writes the key of a trust key to record that it has been converted from Legacy to Tritium. */
    bool LegacyDB::WriteTrustConversion(const uint576_t& hashTrust)
    {
        return Write(hashTrust);
    }


    /* Checks if a Legacy trust key has already been converted to Tritium. */
    bool LegacyDB::HasTrustConversion(const uint576_t& hashTrust)
    {
        return Exists(hashTrust);
    }


    /* Erase a Legacy trust key conversion. */
    bool LegacyDB::EraseTrustConversion(const uint576_t& hashTrust)
    {
        return Erase(hashTrust); //Needed if rollback migrate operation
    }

}
