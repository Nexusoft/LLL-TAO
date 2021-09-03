/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/types/uint1024.h>

#include <LLD/include/global.h>

#include <TAO/Operation/types/contract.h>

#include <TAO/Ledger/include/enum.h>

namespace LLD
{
    /** The Database Constructor. To determine file location and the Bytes per Record. **/
    LogicalDB::LogicalDB(const uint8_t nFlagsIn, const uint32_t nBucketsIn, const uint32_t nCacheIn)
    : SectorDatabase(std::string("_API")
    , nFlagsIn
    , nBucketsIn
    , nCacheIn)
    {
    }


    /** Default Destructor **/
    LogicalDB::~LogicalDB()
    {
    }


    /* Writes a session's access time to the database. */
    bool LogicalDB::WriteSession(const uint256_t& hashGenesis, const uint64_t nActive)
    {
        return Write(std::make_pair(std::string("access"), hashGenesis), nActive);
    }


    /* Reads a session's access time to the database. */
    bool LogicalDB::ReadSession(const uint256_t& hashGenesis, uint64_t &nActive)
    {
        return Read(std::make_pair(std::string("access"), hashGenesis), nActive);
    }


    /* Reads the last txid that was indexed. */
    bool LogicalDB::ReadLastIndex(uint512_t &hashTx)
    {
        return Read(std::string("indexing"), hashTx);
    }


    /* Writes the last txid that was indexed. */
    bool LogicalDB::WriteLastIndex(const uint512_t& hashTx)
    {
        return Write(std::string("indexing"), hashTx);
    }


    /* Pushes an order to the orderbook stack. */
    bool LogicalDB::PushOrder(const std::pair<uint256_t, uint256_t>& pairMarket,
                              const TAO::Operation::Contract& rContract, const uint32_t nContract)
    {
        /* Grab a refernece of our txid. */
        const uint512_t& hashTx =
            rContract.Hash();

        /* Grab a reference of our caller. */
        const uint256_t& hashOwner =
            rContract.Caller();

        /* Check for already existing order. */
        if(HasOrder(hashTx, nContract))
            return false;

        /* Get our current sequence number. */
        uint32_t nMarketSequence = 0, nOwnerSequence = 0;

        /* Read our sequences from disk. */
        Read(std::make_pair(std::string("market.sequence"), pairMarket), nMarketSequence);
        Read(std::make_pair(std::string("market.sequence"), hashOwner),   nOwnerSequence);

        /* Start an ACID transaction for this set of records. */
        TxnBegin();

        /* Write our order by sequence number. */
        if(!Write(std::make_pair(nMarketSequence, pairMarket), std::make_pair(hashTx, nContract)))
            return false;

        /* Add an additional indexing entry for owner level orders. */
        if(!Index(std::make_pair(nOwnerSequence, hashOwner), std::make_pair(nMarketSequence, pairMarket)))
            return false;

        /* Write our new market sequence to disk. */
        if(!Write(std::make_pair(std::string("market.sequence"), pairMarket), ++nMarketSequence))
            return false;

        /* Write our new owner sequence to disk. */
        if(!Write(std::make_pair(std::string("market.sequence"), hashOwner), ++nOwnerSequence))
            return false;

        /* Write our order proof. */
        if(!Write(std::make_pair(hashTx, nContract)))
            return false;

        return TxnCommit();
    }


    /* Pulls a list of orders from the orderbook stack. */
    bool LogicalDB::ListOrders(const std::pair<uint256_t, uint256_t>& pairMarket, std::vector<std::pair<uint512_t, uint32_t>> &vOrders)
    {
        /* Cache our txid and contract as a pair. */
        std::pair<uint512_t, uint32_t> pairOrder;

        /* Loop until we have failed. */
        uint32_t nSequence = 0;
        while(!config::fShutdown.load()) //we want to early terminate on shutdown
        {
            /* Read our current record. */
            if(!Read(std::make_pair(nSequence, pairMarket), pairOrder))
                break;

            /* Check for already executed contracts to omit. */
            if(!LLD::Contract->HasContract(pairOrder, TAO::Ledger::FLAGS::MEMPOOL))
                vOrders.push_back(pairOrder);

            /* Increment our sequence number. */
            ++nSequence;
        }

        return !vOrders.empty();
    }


    /*  List the current completed orders for given market pair. */
    bool LogicalDB::ListExecuted(const std::pair<uint256_t, uint256_t>& pairMarket, std::vector<std::pair<uint512_t, uint32_t>> &vExecuted)
    {
        /* Cache our txid and contract as a pair. */
        std::pair<uint512_t, uint32_t> pairOrder;

        /* Loop until we have failed. */
        uint32_t nSequence = 0;
        while(!config::fShutdown.load()) //we want to early terminate on shutdown
        {
            /* Read our current record. */
            if(!Read(std::make_pair(nSequence, pairMarket), pairOrder))
                break;

            /* Check for already executed contracts to omit. */
            if(LLD::Contract->HasContract(pairOrder, TAO::Ledger::FLAGS::MEMPOOL))
                vExecuted.push_back(pairOrder);

            /* Increment our sequence number. */
            ++nSequence;
        }

        return !vExecuted.empty();
    }


    /* Checks if an order has been indexed in the database already. */
    bool LogicalDB::HasOrder(const uint512_t& hashTx, const uint32_t nContract)
    {
        return Exists(std::make_pair(hashTx, nContract));
    }


    /* Writes a register address PTR mapping from address to name address */
    bool LogicalDB::WritePTR(const uint256_t& hashAddress, const uint256_t& hashName)
    {
        return Write(std::make_pair(std::string("ptr"), hashAddress), hashName);
    }


    /* Reads a register address PTR mapping from address to name address */
    bool LogicalDB::ReadPTR(const uint256_t& hashAddress, uint256_t &hashName)
    {
        return Read(std::make_pair(std::string("ptr"), hashAddress), hashName);
    }


    /* Erases a register address PTR mapping */
    bool LogicalDB::ErasePTR(const uint256_t& hashAddress)
    {
        return Erase(std::make_pair(std::string("ptr"), hashAddress));
    }

    /* Checks if a register address has a PTR mapping */
    bool LogicalDB::HasPTR(const uint256_t& hashAddress)
    {
        return Exists(std::make_pair(std::string("ptr"), hashAddress));
    }
}
