/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/types/local.h>

#include <TAO/Ledger/types/transaction.h>

namespace LLD
{

    /** The Database Constructor. To determine file location and the Bytes per Record. **/
    LocalDB::LocalDB(const uint8_t nFlagsIn, const uint32_t nBucketsIn, const uint32_t nCacheIn)
    : SectorDatabase(std::string("_LOCAL")
    , nFlagsIn
    , nBucketsIn
    , nCacheIn)
    {
    }


    /** Default Destructor **/
    LocalDB::~LocalDB()
    {
    }


    /* Writes the txid of the transaction that modified the given register. */
    bool LocalDB::WriteIndex(const uint256_t& hashAddress, const std::pair<uint512_t, uint64_t>& pairIndex)
    {
        return Write(std::make_pair(std::string("index"), hashAddress), pairIndex);
    }


    /* Reads the txid of the transaction that modified the given register. */
    bool LocalDB::ReadIndex(const uint256_t& hashAddress, std::pair<uint512_t, uint64_t> &pairIndex)
    {
        return Read(std::make_pair(std::string("index"), hashAddress), pairIndex);
    }

    /* Writes the timestamp of a register's local disk cache expiration. */
    bool LocalDB::WriteExpiration(const uint256_t& hashAddress, const uint64_t nTimestamp)
    {
        return Write(std::make_pair(std::string("expires"), hashAddress), nTimestamp);
    }


    /* Reads the timestamp of a register's local disk cache expiration. */
    bool LocalDB::ReadExpiration(const uint256_t& hashAddress, uint64_t &nTimestamp)
    {
        return Read(std::make_pair(std::string("expires"), hashAddress), nTimestamp);
    }

    /* Writes a stake change request indexed by genesis. */
    bool LocalDB::WriteStakeChange(const uint256_t& hashGenesis, const TAO::Ledger::StakeChange& stakeChange)
    {
        return Write(std::make_pair(std::string("stakechange"), hashGenesis), stakeChange);
    }


    /* Reads a stake change request for a sig chain genesis. */
    bool LocalDB::ReadStakeChange(const uint256_t& hashGenesis, TAO::Ledger::StakeChange &stakeChange)
    {
        return Read(std::make_pair(std::string("stakechange"), hashGenesis), stakeChange);
    }


    /* Removes a recorded stake change request. */
    bool LocalDB::EraseStakeChange(const uint256_t& hashGenesis)
    {
        return Erase(std::make_pair(std::string("stakechange"), hashGenesis));
    }


    /* Writes a notification suppression record */
    bool LocalDB::WriteSuppressNotification(const uint512_t& hashTx, const uint32_t nContract, const uint64_t &nTimestamp)
    {
        return Write(std::make_tuple(std::string("suppress"), hashTx, nContract), nTimestamp);
    }


    /* Reads a notification suppression record */
    bool LocalDB::ReadSuppressNotification(const uint512_t& hashTx, const uint32_t nContract, uint64_t &nTimestamp)
    {
        return Read(std::make_tuple(std::string("suppress"), hashTx, nContract), nTimestamp);
    }


    /* Removes a suppressed notification record */
    bool LocalDB::EraseSuppressNotification(const uint512_t& hashTx, const uint32_t nContract)
    {
        return Erase(std::make_tuple(std::string("suppress"), hashTx, nContract));
    }


    /* Writes a username - genesis hash pair to the local database. */
    bool LocalDB::WriteGenesis(const SecureString& strUsername, const uint256_t& hashGenesis)
    {
        std::vector<uint8_t> vKey(strUsername.begin(), strUsername.end());
        return Write(std::make_pair(std::string("genesis"), vKey), hashGenesis);
    }


    /* Reads a genesis hash from the local database for a given username */
    bool LocalDB::ReadGenesis(const SecureString& strUsername, uint256_t &hashGenesis)
    {
        std::vector<uint8_t> vKey(strUsername.begin(), strUsername.end());
        return Read(std::make_pair(std::string("genesis"), vKey), hashGenesis);
    }


    /* Writes session data to the local database. */
    bool LocalDB::WriteSession(const uint256_t& hashGenesis, const std::vector<uint8_t>& vchData)
    {
        return Write(std::make_pair(std::string("session"), hashGenesis), vchData);
    }


    /* Reads session data from the local database */
    bool LocalDB::ReadSession(const uint256_t& hashGenesis, std::vector<uint8_t>& vchData)
    {
        return Read(std::make_pair(std::string("session"), hashGenesis), vchData);
    }


    /* Deletes session data from the local database fort he given session ID. */
    bool LocalDB::EraseSession(const uint256_t& hashGenesis)
    {
        return Erase(std::make_pair(std::string("session"), hashGenesis));
    }


    /* Determines whether the local DB contains session data for the given session ID */
    bool LocalDB::HasSession(const uint256_t& hashGenesis)
    {
        return Exists(std::make_pair(std::string("session"), hashGenesis));
    }


    /* Writes the timestamp of a cached name expiration. */
    bool LocalDB::WriteNameExpiration(const uint256_t& hashGenesis, const uint256_t& hashAddress, const uint64_t nTimestamp)
    {
        return Write(std::make_tuple(std::string("name_expires"), hashGenesis, hashAddress), nTimestamp);
    }

    /* Reads the timestamp of a cached name expiration. */
    bool LocalDB::ReadNameExpiration(const uint256_t& hashGenesis, const uint256_t& hashAddress, uint64_t &nTimestamp)
    {
        return Read(std::make_tuple(std::string("name_expires"), hashGenesis, hashAddress), nTimestamp);
    }


    /* Writes a cached name to the local database. */
    bool LocalDB::WriteName(const uint256_t& hashGenesis, const uint256_t& hashAddress, const std::string& strName)
    {
        /* Write expiration of this name. */
        WriteNameExpiration(hashGenesis, hashAddress, runtime::unifiedtimestamp() + 600); //10 minute expiration

        /* Write the name record */
        return Write(std::make_tuple(std::string("name"), hashGenesis, hashAddress), strName);
    }


    /* Reads a cached name from the local database. */
    bool LocalDB::ReadName(const uint256_t& hashGenesis, const uint256_t& hashAddress, std::string &strName)
    {
        /* Generate the key for the name record so we can check whether it exists and whether it has expired */
        std::tuple<std::string, uint256_t, uint256_t> key = std::make_tuple(std::string("name"), hashGenesis, hashAddress);

        /* If the name exists, check for expiry */
        if(Exists(key))
        {
            /* Expiry timestamp for this name */
            uint64_t nExpires = 0;

            /* Flag indicating the cached name has expired */
            bool fExpired = false;
            
            /* Read the expiration time for this name record */
            if(ReadNameExpiration(hashGenesis, hashAddress, nExpires))
            {
                /* Check expiration timestamp. */
                if(runtime::unifiedtimestamp() > nExpires)
                    fExpired = true;
            }
            else
                fExpired = true;

            /* Only return the name if it hasn't expired */
            if(!fExpired)
                return Read(std::make_tuple(std::string("name"), hashGenesis, hashAddress), strName);           
        }

        /* If it doesn't exist or has expired then return false */
        return false;
        
    }

}
