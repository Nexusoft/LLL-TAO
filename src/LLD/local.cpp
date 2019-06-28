/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/local.h>

#include <TAO/Ledger/types/transaction.h>

namespace LLD
{

    /** The Database Constructor. To determine file location and the Bytes per Record. **/
    LocalDB::LocalDB(uint8_t nFlags)
    : SectorDatabase(std::string("local"), nFlags, 256 * 256 * 8)
    {
    }


    /** Default Destructor **/
    LocalDB::~LocalDB()
    {
    }


    /* Writes a genesis transaction-id to disk. */
    bool LocalDB::WriteGenesis(const uint256_t& hashGenesis, const TAO::Ledger::Transaction& tx)
    {
        return Write(std::make_pair(std::string("genesis"), hashGenesis), tx);
    }


    /* Reads a genesis transaction-id from disk. */
    bool LocalDB::ReadGenesis(const uint256_t& hashGenesis, TAO::Ledger::Transaction &tx)
    {
        return Read(std::make_pair(std::string("genesis"), hashGenesis), tx);
    }


    /* Checks if a genesis transaction exists. */
    bool LocalDB::HasGenesis(const uint256_t& hashGenesis)
    {
        return Exists(std::make_pair(std::string("genesis"), hashGenesis));
    }


    /* Writes the last txid of sigchain to disk indexed by genesis. */
    bool LocalDB::WriteLast(const uint256_t& hashGenesis, const uint512_t& hashLast)
    {
        return Write(std::make_pair(std::string("last"), hashGenesis), hashLast);
    }


    /* Reads the last txid of sigchain to disk indexed by genesis. */
    bool LocalDB::ReadLast(const uint256_t& hashGenesis, uint512_t &hashLast)
    {
        return Read(std::make_pair(std::string("last"), hashGenesis), hashLast);
    }

}
