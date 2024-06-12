/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2023

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLD_INCLUDE_LOCAL_H
#define NEXUS_LLD_INCLUDE_LOCAL_H

#include <LLC/types/uint1024.h>

#include <LLD/templates/static.h>
#include <LLD/cache/binary_lru.h>
#include <LLD/keychain/hashmap.h>

#include <TAO/Ledger/include/stake_change.h>


/** Forward declarations **/
namespace TAO
{
    namespace Ledger
    {
        class Transaction;
    }
}


namespace LLD
{

  /** LocalDB
   *
   *  Database class for storing local wallet transactions.
   *
   **/
    class LocalDB : public Templates::StaticDatabase<BinaryHashMap, BinaryLRU, Config::Hashmap>
    {

    public:

        /** The Database Constructor. To determine file location and the Bytes per Record. **/
        LocalDB(const Config::Static& sector, const Config::Hashmap& keychain);


        /** Default Destructor **/
        virtual ~LocalDB();


        /** WriteIndex
         *
         *  Writes the txid of the transaction that modified the given register.
         *
         *  @param[in] hashAddress The register address.
         *  @param[in] pairIndex The index containing expiration and txid.
         *
         *  @return True if the last was successfully written, false otherwise.
         *
         **/
        bool WriteIndex(const uint256_t& hashAddress, const std::pair<uint512_t, uint64_t>& pairIndex);


        /** ReadIndex
         *
         *  Reads the txid of the transaction that modified the given register.
         *
         *  @param[in] hashAddress The register address.
         *  @param[out] pairIndex The index containing expiration and txid.
         *
         *  @return True if the last was successfully read, false otherwise.
         *
         **/
        bool ReadIndex(const uint256_t& hashAddress, std::pair<uint512_t, uint64_t> &pairIndex);


        /** WriteExpiration
         *
         *  Writes the timestamp of a register's local disk cache expiration.
         *
         *  @param[in] hashAddress The register address.
         *  @param[in] nTimestamp The timestamp the register cache expires.
         *
         *  @return True if the last was successfully written, false otherwise.
         *
         **/
        bool WriteExpiration(const uint256_t& hashAddress, const uint64_t nTimestamp);


        /** ReadExpiration
         *
         *  Reads the timestamp of a register's local disk cache expiration.
         *
         *  @param[in] hashAddress The register address.
         *  @param[out] nTimestamp The timestamp register cache expires.
         *
         *  @return True if the last was successfully read, false otherwise.
         *
         **/
        bool ReadExpiration(const uint256_t& hashAddress, uint64_t &nTimestamp);


        /** WriteStakeChange
         *
         *  Writes a stake change request indexed by genesis.
         *
         *  @param[in] hashGenesis The user genesis requesting the stake change.
         *  @param[in] stakeChange Stake change request data to record.
         *
         *  @return True if the change was successfully written, false otherwise.
         *
         **/
        bool WriteStakeChange(const uint256_t& hashGenesis, const TAO::Ledger::StakeChange& stakeChange);


        /** ReadStakeChange
         *
         *  Reads a stake change request for a sig chain genesis.
         *
         *  @param[in] hashGenesis The user genesis to read.
         *
         *  @return True if the change was successfully read, false otherwise.
         *
         **/
        bool ReadStakeChange(const uint256_t& hashGenesis, TAO::Ledger::StakeChange& stakeChange);


        /** EraseStakeChange
         *
         *  Removes a recorded stake change request.
         *
         *  @param[in] hashGenesis The user genesis of the request to remove.
         *
         *  @return True if the change was successfully erased, false otherwise.
         *
         **/
        bool EraseStakeChange(const uint256_t& hashGenesis);


        /** WriteSuppressNotification
         *
         *  Writes a notification suppression record
         *
         *  @param[in] hashTx The transaction Id of the notification to suppress.
         *  @param[in] nContract The contract Id of the notification to suppress.
         *  @param[in] nTimestamp Timestamp of when the notification should be suppressed until
         *
         *  @return True if the record was successfully written, false otherwise.
         *
         **/
        bool WriteSuppressNotification(const uint512_t& hashTx, const uint32_t nContract, const uint64_t &nTimestamp);


        /** ReadSuppressNotification
         *
         *  Reads a notification suppression record
         *
         *  @param[in] hashTx The transaction Id of the notification .
         *  @param[in] nContract The contract Id of the notification.
         *  @param[out] nTimestamp Timestamp of when the notification should be suppressed until
         *
         *  @return True if the record was successfully read, false otherwise.
         *
         **/
        bool ReadSuppressNotification(const uint512_t& hashTx, const uint32_t nContract, uint64_t &nTimestamp);


        /** EraseSuppressNotification
         *
         *  Removes a suppressed notification record.
         *
         *  @param[in] hashTx The transaction Id of the notification .
         *  @param[in] nContract The contract Id of the notification.
         *
         *  @return True if the record successfully erased, false otherwise.
         *
         **/
        bool EraseSuppressNotification(const uint512_t& hashTx, const uint32_t nContract);


        /** WriteFirst
         *
         *  Writes a username - genesis hash pair to the local database.
         *
         *  @param[in] strUsername The username.
         *  @param[in] hashGenesis The genesis hash corresponding to this username.
         *
         *  @return True if the last was successfully written, false otherwise.
         *
         **/
        bool WriteFirst(const SecureString& strUsername, const uint256_t& hashGenesis);


        /** ReadFirst
         *
         *  Reads a genesis hash from the local database for a given username.
         *
         *  @param[in] strUsername The username to read the genesis hash for.
         *  @param[out] hashGenesis The genesis hash corresponding to this username.
         *
         *  @return True if the last was successfully read, false otherwise.
         *
         **/
        bool ReadFirst(const SecureString& strUsername, uint256_t &hashGenesis);


        /** WriteSession
         *
         *  Writes session data to the local database.
         *
         *  @param[in] hashGenesis The genesis hash of the user to save the session for.
         *  @param[in] vchData The session data to be saved.
         *
         *  @return True if the session was successfully written, false otherwise.
         *
         **/
        bool WriteSession(const uint256_t& hashGenesis, const std::vector<uint8_t>& vchData);


        /** ReadSession
         *
         *  Reads session data from the local database .
         *
         *  @param[in] nSession The genesis hash of the user to load the session for
         *  @param[out] vchData The session data to be loaded.
         *
         *  @return True if the session was successfully read, false otherwise.
         *
         **/
        bool ReadSession(const uint256_t& hashGenesis, std::vector<uint8_t>& vchData);


        /** EraseSession
         *
         *  Deletes session data from the local database fort he given session ID.
         *
         *  @param[in] hashGenesis The genesis hash of the user to erase the session for
         *
         *  @return True if the data was successfully deleted, false otherwise.
         *
         **/
        bool EraseSession(const uint256_t& hashGenesis);


        /** HasSession
         *
         *  Determines whether the local DB contains session data for the given session ID
         *
         *  @param[in] hashGenesis The genesis hash of the user to check
         *
         *  @return True if the session data exists, false otherwise.
         *
         **/
        bool HasSession(const uint256_t& hashGenesis);


        /** HasRecord
         *
         *  Check if a record exists for a table.
         *
         *  @param[in] strTable The table that we are checking for key in.
         *  @param[in] strKey The key we are checking for in the table
         *
         *  @return True if record exists in table, false otherwise.
         *
         **/
        bool HasRecord(const std::string& strTable, const std::string& strKey);


        /** EraseRecord
         *
         *  Erase a record from a table.
         *
         *  @param[in] strTable The table that we are erasing in.
         *  @param[in] strKey The key we are erasing in the table
         *
         *  @return True if record was erased, false otherwise.
         *
         **/
        bool EraseRecord(const std::string& strTable, const std::string& strKey);


        /** PushRecord
         *
         *  Push a new record to a given table.
         *
         *  @param[in] strTable The table that we are erasing in.
         *  @param[in] strKey The key we are erasing in the table
         *  @param[in] strValue The value that we are pushing as a record.
         *
         *  @return True if the session data exists, false otherwise.
         *
         **/
        bool PushRecord(const std::string& strTable, const std::string& strKey, const std::string& strValue);


        /** ListRecords
         *
         *  List the current records for a given table.
         *
         *  @param[in] strTable The table that we are erasing in.
         *  @param[out] vRecords The return vector that holds records retrieved.
         *
         *  @return True if we found data in the given records.
         *
         **/
        bool ListRecords(const std::string& strTable, std::vector<std::pair<std::string, std::string>> &vRecords);

    };
}

#endif
