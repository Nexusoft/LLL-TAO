/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/trust.h>

#include <Legacy/types/trustkey.h>


namespace LLD
{

    /** The Database Constructor. To determine file location and the Bytes per Record. **/
    TrustDB::TrustDB(uint8_t nFlagsIn)
    : SectorDatabase(std::string("trust"), nFlagsIn)
    {
    }


    /** Default Destructor **/
    TrustDB::~TrustDB()
    {
    }


    /* Writes a trust key to the ledger DB. */
    bool TrustDB::WriteTrustKey(const uint576_t& hashKey, const Legacy::TrustKey& key)
    {
        return Write(hashKey, key);
    }


    /* Reads a trust key from the ledger DB. */
    bool TrustDB::ReadTrustKey(const uint576_t& hashKey, Legacy::TrustKey& key)
    {
        return Read(hashKey, key);
    }

}
