/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/types/uint1024.h>

#include <LLD/types/api.h>


namespace LLD
{
    /** The Database Constructor. To determine file location and the Bytes per Record. **/
    APIDB::APIDB(const uint8_t nFlagsIn, const uint32_t nBucketsIn, const uint32_t nCacheIn)
    : SectorDatabase(std::string("_API")
    , nFlagsIn
    , nBucketsIn
    , nCacheIn)
    {
    }


    /** Default Destructor **/
    APIDB::~APIDB()
    {
    }


    /** WriteSession
     *
     *  Writes a session's access time to the database.
     *
     *  @param[in] hashAddress The register address.
     *  @param[in] nActive The timestamp this session was last active
     *
     *  @return True if a session exists in the localdb
     *
     **/
    bool APIDB::WriteSession(const uint256_t& hashGenesis, const uint64_t nActive)
    {
        return Write(std::make_pair(std::string("access"), hashGenesis), nActive);
    }


    /** ReadSession
     *
     *  Reads a session's access time to the database.
     *
     *  @param[in] hashAddress The register address.
     *  @param[out] nActive The timestamp this session was last active
     *
     *  @return True if a session exists in the localdb
     *
     **/
    bool APIDB::ReadSession(const uint256_t& hashGenesis, uint64_t &nActive)
    {
        return Read(std::make_pair(std::string("access"), hashGenesis), nActive);
    }
}
