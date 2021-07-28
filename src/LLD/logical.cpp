/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/types/uint1024.h>

#include <LLD/types/logical.h>


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
