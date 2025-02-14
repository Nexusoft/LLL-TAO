/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/types/uint1024.h>

#include <LLD/include/global.h>

#include <TAO/API/types/transaction.h>

#include <TAO/Operation/types/contract.h>

#include <TAO/Register/include/unpack.h>

#include <TAO/Ledger/include/enum.h>
#include <TAO/Ledger/include/constants.h>
#include <TAO/Ledger/include/chainstate.h>

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


    /* Reads the surplus coins that are not on tritium */
    bool LogicalDB::ReadLegacySurplus(uint64_t &nSurplus)
    {
        return Read(std::string("legacy.surplus"), nSurplus);
    }


    /* Writes the surplus coins that are not on tritium */
    bool LogicalDB::WriteLegacySurplus(const uint64_t nSurplus)
    {
        return Write(std::string("legacy.surplus"), nSurplus);
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


    /* Push an register transaction to process for given register address. */
    bool LogicalDB::PushRegisterTx(const uint256_t& hashRegister, const uint512_t& hashTx)
    {
        /* Get our current sequence number. */
        uint32_t nOwnerSequence = 0;

        /* Read our sequences from disk. */
        Read(std::make_pair(std::string("register.tx.sequence"), hashRegister), nOwnerSequence);

        /* Add our indexing entry by owner sequence number. */
        if(!Write(std::make_tuple(std::string("register.tx.index"), (nOwnerSequence % 5), hashRegister), hashTx))
            return false;

        /* Write our new events sequence to disk. */
        if(!Write(std::make_pair(std::string("register.tx.sequence"), hashRegister), ++nOwnerSequence))
            return false;

        return true;
    }


    /* Erase an register transaction for given register address. */
    bool LogicalDB::EraseRegisterTx(const uint256_t& hashRegister)
    {
        /* Get our current sequence number. */
        uint32_t nOwnerSequence = 0;

        /* Read our sequences from disk. */
        if(!Read(std::make_pair(std::string("register.tx.sequence"), hashRegister), nOwnerSequence))
            return false;

        /* Add our indexing entry by owner sequence number. */
        if(!Erase(std::make_tuple(std::string("register.tx.index"), (--nOwnerSequence % 5), hashRegister)))
            return false;

        /* Write our new events sequence to disk. */
        if(!Write(std::make_pair(std::string("register.tx.sequence"), hashRegister), nOwnerSequence))
            return false;

        return true;
    }


    /* Get an register transaction for given register address. */
    bool LogicalDB::LastRegisterTx(const uint256_t& hashRegister, uint512_t &hashTx)
    {
        /* Get our current sequence number. */
        uint32_t nOwnerSequence = 0;

        /* Read our sequences from disk. */
        if(!Read(std::make_pair(std::string("register.tx.sequence"), hashRegister), nOwnerSequence))
            return false;

        /* Add our indexing entry by owner sequence number. */
        if(!Read(std::make_tuple(std::string("register.tx.index"), (--nOwnerSequence % 5), hashRegister), hashTx))
            return false;

        return true;
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
            return debug::error(FUNCTION, hashTx.SubString(), " already exists");

        /* Get our current sequence number. */
        uint32_t nMarketSequence = 0, nOwnerSequence = 0;

        /* Read our sequences from disk. */
        Read(std::make_pair(std::string("market.sequence"), pairMarket), nMarketSequence);
        Read(std::make_pair(std::string("owner.sequence"),  hashOwner),   nOwnerSequence);

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
        if(!Write(std::make_pair(std::string("owner.sequence"), hashOwner), ++nOwnerSequence))
            return false;

        /* Write our order proof. */
        if(!Write(std::make_pair(hashTx, nContract)))
            return false;

        return true;
    }


    /* Pushes an order to the orderbook stack for a given asset. */
    bool LogicalDB::PushOrder(const uint256_t& hashRegister, const TAO::Operation::Contract& rContract, const uint32_t nContract)
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
        Read(std::make_pair(std::string("market.sequence"), hashRegister), nMarketSequence);
        Read(std::make_pair(std::string("owner.sequence"),  hashOwner),   nOwnerSequence);

        /* Write our order by sequence number. */
        if(!Write(std::make_pair(nMarketSequence, hashRegister), std::make_pair(hashTx, nContract)))
            return false;

        /* Add an additional indexing entry for owner level orders. */
        if(!Index(std::make_pair(nOwnerSequence, hashOwner), std::make_pair(nMarketSequence, hashRegister)))
            return false;

        /* Write our new market sequence to disk. */
        if(!Write(std::make_pair(std::string("market.sequence"), hashRegister), ++nMarketSequence))
            return false;

        /* Write our new owner sequence to disk. */
        if(!Write(std::make_pair(std::string("owner.sequence"), hashOwner), ++nOwnerSequence))
            return false;

        /* Write our order proof. */
        if(!Write(std::make_pair(hashTx, nContract)))
            return false;

        return true;
    }


    /* Pulls a list of orders from the orderbook stack. */
    bool LogicalDB::ListOrders(const std::pair<uint256_t, uint256_t>& pairMarket, std::vector<std::pair<uint512_t, uint32_t>> &vOrders)
    {
        /* Track our return success. */
        bool fSuccess = false;

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
            {
                vOrders.push_back(pairOrder);
                fSuccess = true;
            }

            /* Set our new sequence. */
            ++nSequence;
        }

        return fSuccess;
    }


    /* List the current active orders for given user's sigchain. */
    bool LogicalDB::ListOrders(const uint256_t& hashGenesis, std::vector<std::pair<uint512_t, uint32_t>> &vOrders)
    {
        /* Track our return success. */
        bool fSuccess = false;

        /* Cache our txid and contract as a pair. */
        std::pair<uint512_t, uint32_t> pairOrder;

        /* Loop until we have failed. */
        uint32_t nSequence = 0;
        while(!config::fShutdown.load()) //we want to early terminate on shutdown
        {
            /* Read our current record. */
            if(!Read(std::make_pair(nSequence, hashGenesis), pairOrder))
                break;

            /* Check for already executed contracts to omit. */
            if(!LLD::Contract->HasContract(pairOrder, TAO::Ledger::FLAGS::MEMPOOL))
            {
                vOrders.push_back(pairOrder);
                fSuccess = true;
            }

            /* Increment our sequence number. */
            ++nSequence;
        }

        return fSuccess;
    }

    /* List the current active orders for given market pair. */
    bool LogicalDB::ListAllOrders(const std::pair<uint256_t, uint256_t>& pairMarket, std::vector<std::pair<uint512_t, uint32_t>> &vOrders)
    {
        /* Track our return success. */
        bool fSuccess = false;

        /* Cache our txid and contract as a pair. */
        std::pair<uint512_t, uint32_t> pairOrder;

        /* Loop until we have failed. */
        uint32_t nSequence = 0;
        while(!config::fShutdown.load()) //we want to early terminate on shutdown
        {
            /* Read our current record. */
            if(!Read(std::make_pair(nSequence, pairMarket), pairOrder))
                break;

            /* Push order ot our list. */
            vOrders.push_back(pairOrder);

            /* Set our new sequence. */
            ++nSequence;
            fSuccess = true;
        }

        return fSuccess;
    }


    /* List the current active orders for given user's sigchain. */
    bool LogicalDB::ListAllOrders(const uint256_t& hashGenesis, std::vector<std::pair<uint512_t, uint32_t>> &vOrders)
    {
        /* Track our return success. */
        bool fSuccess = false;

        /* Cache our txid and contract as a pair. */
        std::pair<uint512_t, uint32_t> pairOrder;

        /* Loop until we have failed. */
        uint32_t nSequence = 0;
        while(!config::fShutdown.load()) //we want to early terminate on shutdown
        {
            /* Read our current record. */
            if(!Read(std::make_pair(nSequence, hashGenesis), pairOrder))
                break;

            /* Push order ot our list. */
            vOrders.push_back(pairOrder);

            /* Set our new sequence. */
            ++nSequence;
            fSuccess = true;
        }

        return fSuccess;
    }


    /* List the current completed orders for given user's sigchain. */
    bool LogicalDB::ListExecuted(const uint256_t& hashGenesis, std::vector<std::pair<uint512_t, uint32_t>> &vExecuted)
    {
        /* Track our return success. */
        bool fSuccess = false;

        /* Cache our txid and contract as a pair. */
        std::pair<uint512_t, uint32_t> pairOrder;

        /* Loop until we have failed. */
        uint32_t nSequence = 0;
        while(!config::fShutdown.load()) //we want to early terminate on shutdown
        {
            /* Read our current record. */
            if(!Read(std::make_pair(nSequence, hashGenesis), pairOrder))
                break;

            /* Check for already executed contracts to omit. */
            if(LLD::Contract->HasContract(pairOrder, TAO::Ledger::FLAGS::MEMPOOL))
            {
                vExecuted.push_back(pairOrder);
                fSuccess = true;
            }

            /* Increment our sequence number. */
            ++nSequence;
        }

        return fSuccess;
    }


    /*  List the current completed orders for given market pair. */
    bool LogicalDB::ListExecuted(const std::pair<uint256_t, uint256_t>& pairMarket, std::vector<std::pair<uint512_t, uint32_t>> &vExecuted)
    {
        /* Track our return success. */
        bool fSuccess = false;

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
            {
                vExecuted.push_back(pairOrder);
                fSuccess = true;
            }

            /* Increment our sequence number. */
            ++nSequence;
        }

        return fSuccess;
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


    /* Build indexes for transactions over a rolling modulus. For -indexregister flag. */
    void LogicalDB::IndexRegisters()
    {
        /* Not allowed in -client mode. */
        if(config::fClient.load())
            return;

        /* Check for address indexing flag. */
        if(Exists(std::string("register.indexed")))
        {
            /* Check there is no argument supplied. */
            if(!config::HasArg("-indexregister"))
            {
                /* Warn that -indexheight is persistent. */
                debug::notice(FUNCTION, "-indexregister enabled from valid indexes");

                /* Set indexing argument now. */
                RECURSIVE(config::ARGS_MUTEX);
                config::mapArgs["-indexregister"] = "1";

                /* Set our internal configuration. */
                config::fIndexRegister.store(true);
            }

            return;
        }

        /* Check there is no argument supplied. */
        if(!config::fIndexRegister.load())
            return;

        /* Start a timer to track. */
        runtime::timer timer;
        timer.Start();

        /* Get our starting hash. */
        const uint1024_t hashBegin =
             TAO::Ledger::hashTritium;

        /* Read the first tritium block. */
        TAO::Ledger::BlockState state;
        if(!LLD::Ledger->ReadBlock(hashBegin, state))
        {
            debug::warning(FUNCTION, "No tritium blocks available ", hashBegin.SubString());
            return;
        }

        /* Keep track of our total count. */
        uint32_t nScannedCount = 0;

        /* Start our scan. */
        debug::notice(FUNCTION, "Building register indexes from block ", state.GetHash().SubString());
        while(!config::fShutdown.load())
        {
            /* Loop through found transactions. */
            for(uint32_t nIndex = 0; nIndex < state.vtx.size(); ++nIndex)
            {
                /* We only care about tritium transactions. */
                if(state.vtx[nIndex].first != TAO::Ledger::TRANSACTION::TRITIUM)
                    continue;

                /* Get a reference of our txid. */
                const uint512_t& hashTx =
                    state.vtx[nIndex].second;

                /* Read the transaction from disk. */
                TAO::Ledger::Transaction tx;
                if(!LLD::Ledger->ReadTx(hashTx, tx))
                    continue;

                /* Iterate the transaction contracts. */
                std::set<uint256_t> setAddresses;
                for(uint32_t nContract = 0; nContract < tx.Size(); ++nContract)
                {
                    /* Grab contract reference. */
                    const TAO::Operation::Contract& rContract = tx[nContract];

                    /* Unpack the address we will be working on. */
                    uint256_t hashAddress;
                    if(!TAO::Register::Unpack(rContract, hashAddress))
                        continue;

                    /* Check for duplicate entries. */
                    if(setAddresses.count(hashAddress))
                        continue;

                    /* Check fo register in database. */
                    PushRegisterTx(hashAddress, hashTx);

                    /* Push the address now. */
                    setAddresses.insert(hashAddress);
                }

                /* Update the scanned count for meters. */
                ++nScannedCount;

                /* Meter for output. */
                if(nScannedCount % 100000 == 0)
                {
                    /* Get the time it took to rescan. */
                    uint32_t nElapsedSeconds = timer.Elapsed();
                    debug::log(0, FUNCTION, "Built ", nScannedCount, " register indexes in ", nElapsedSeconds, " seconds (",
                        std::fixed, (double)(nScannedCount / (nElapsedSeconds > 0 ? nElapsedSeconds : 1 )), " tx/s)");
                }
            }

            /* Iterate to the next block in the queue. */
            state = state.Next();
            if(!state)
            {
                /* Write our last index now. */
                Write(std::string("register.indexed"));

                debug::notice(FUNCTION, "Complated scanning ", nScannedCount, " in ", timer.Elapsed(), " seconds");

                break;
            }
        }
    }
}
