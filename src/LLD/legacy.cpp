/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/legacy.h>


namespace LLD
{

    /** The Database Constructor. To determine file location and the Bytes per Record. **/
    LegacyDB::LegacyDB(uint8_t nFlags, uint64_t nBuckets, uint32_t nCacheIn)
    : SectorDatabase(std::string("legacy"), nFlags, nBuckets, nCacheIn)
    {
    }


    /** Default Destructor **/
    LegacyDB::~LegacyDB()
    {
    }


    /* Writes a transaction to the legacy DB. */
    bool LegacyDB::WriteTx(const uint512_t& hashTx, const Legacy::Transaction& tx)
    {
        return Write(std::make_pair(std::string("tx"), hashTx), tx);
    }


    /* Reads a transaction from the legacy DB. */
    bool LegacyDB::ReadTx(const uint512_t& hashTx, Legacy::Transaction& tx)
    {
        return Read(std::make_pair(std::string("tx"), hashTx), tx);
    }


    /* Erases a transaction from the ledger DB. */
    bool LegacyDB::EraseTx(const uint512_t& hashTx)
    {
        return Erase(std::make_pair(std::string("tx"), hashTx));
    }


    /* Checks if a transaction exists. */
    bool LegacyDB::HasTx(const uint512_t& hashTx)
    {
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
}
