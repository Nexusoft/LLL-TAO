/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2023

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLD_INCLUDE_REGISTER_H
#define NEXUS_LLD_INCLUDE_REGISTER_H

#include <LLC/types/uint1024.h>

#include <LLD/templates/sector.h>
#include <LLD/cache/binary_lru.h>
#include <LLD/keychain/hashmap.h>

#include <TAO/Register/types/crypto.h>

#include <TAO/Ledger/include/enum.h>

namespace LLD
{
    /** Type to handle internal FLAGS::LOOKUP cache entries. **/
    typedef std::map<uint256_t, std::pair<TAO::Register::State, uint64_t>> RegisterCache;


    /** RegisterTransaction
     *
     *  Helper class for managing memory states in register database.
     *
     **/
    class RegisterTransaction
    {
    public:

        /** Map of states that are stored in memory mode until commited. **/
        std::map<uint256_t, TAO::Register::State> mapStates;


        /** Set of indexes to remove during commit. **/
        std::set<uint256_t> setErase;

    };


    /** RegisterDB
     *
     *  The database class for the Register Layer.
     *
     **/
    class RegisterDB : public SectorDatabase<BinaryHashMap, BinaryLRU>
    {

        /** Memory mutex to lock when accessing internal memory states. **/
        std::mutex MEMORY;


        /** Register transaction to track current open transaction. **/
        RegisterTransaction* pMemory;


        /** Miner transaction to track current states for miner verification. **/
        RegisterTransaction* pMiner;


        /** Register transaction to keep open all commited data. **/
        RegisterTransaction* pCommit;


        /** Register cache to keep lookup data in memory. **/
        RegisterCache* pLookup;


    public:


        /** The Database Constructor. To determine file location and the Bytes per Record. **/
        RegisterDB(const uint8_t nFlagsIn = FLAGS::CREATE | FLAGS::WRITE,
            const uint32_t nBucketsIn = 77773, const uint32_t nCacheIn = 1024 * 1024);


        /** Default Destructor **/
        virtual ~RegisterDB();


        /** WriteState
         *
         *  Writes a state register to the register database.
         *  If MEMPOOL flag is set, this will write state into a temporary
         *  memory to handle register state sequencing before blocks commit
         *
         *  If WRITE flag is set, this will erase the temporary memory state
         *  and commit the state to disk
         *
         *  @param[in] hashRegister The register address.
         *  @param[in] state The state register to write.
         *  @param[in] nFlags The flags from ledger
         *
         *  @return True if write was successful, false otherwise.
         *
         **/
        bool WriteState(const uint256_t& hashRegister, const TAO::Register::State& state, const uint8_t nFlags = TAO::Ledger::FLAGS::BLOCK);


        /** ReadState
         *
         *  Read a state register from the register database.
         *
         *  @param[in] hashRegister The register address.
         *  @param[out] state The state register to read.
         *
         *  @return True if read was successful, false otherwise.
         *
         **/
        bool ReadState(const uint256_t& hashRegister, TAO::Register::State& state, const uint8_t nFlags = TAO::Ledger::FLAGS::BLOCK);


        /** EraseState
         *
         *  Erase a state register from the register database.
         *
         *  @param[in] hashRegister The register address.
         *  @param[out] state The state register to read.
         *
         *  @return True if erase was successful, false otherwise.
         *
         **/
        bool EraseState(const uint256_t& hashRegister, const uint8_t nFlags = TAO::Ledger::FLAGS::BLOCK);


        /** ReadObject
         *
         *  Read an object register from the register database.
         *
         *  @param[in] hashRegister The register address.
         *  @param[out] object The object register to read.
         *
         *  @return True if read was successful, false otherwise.
         *
         **/
        bool ReadObject(const uint256_t& hashRegister, TAO::Register::Object& object, const uint8_t nFlags = TAO::Ledger::FLAGS::BLOCK);


        /** ReadCrypto
         *
         *  Read a crypto object register from the register database.
         *
         *  @param[in] hashGenesis The genesis-id of crypto object register owner..
         *  @param[out] object The object register to read.
         *
         *  @return True if read was successful, false otherwise.
         *
         **/
        bool ReadCrypto(const uint256_t& hashGenesis, TAO::Register::Crypto& object, const uint8_t nFlags = TAO::Ledger::FLAGS::BLOCK);


        /** IndexTrust
         *
         *  Index a genesis to a register address.
         *
         *  @param[in] hashGenesis The genesis-id address.
         *  @param[in] hashRegister The state register to index to.
         *
         *  @return True if write was successful, false otherwise.
         *
         **/
        bool IndexTrust(const uint256_t& hashGenesis, const uint256_t& hashRegister);


        /** HasTrust
         *
         *  Check that a genesis doesn't already exist
         *
         *  @param[in] hashGenesis The genesis-id address.
         *
         *  @return True if write was successful, false otherwise.
         *
         **/
        bool HasTrust(const uint256_t& hashGenesis);


        /** WriteTrust
         *
         *  Write a genesis to a register address.
         *
         *  @param[in] hashGenesis The genesis-id address.
         *  @param[in] state The state register to write.
         *
         *  @return True if write was successful, false otherwise.
         *
         **/
        bool WriteTrust(const uint256_t& hashGenesis, const TAO::Register::State& state);


        /** ReadTrust
         *
         *  Index a genesis to a register address.
         *
         *  @param[in] hashGenesis The genesis-id address.
         *  @param[in] state The state register to read.
         *
         *  @return True if read was successful, false otherwise.
         *
         **/
        bool ReadTrust(const uint256_t& hashGenesis, TAO::Register::State& state);


        /** EraseTrust
         *
         *  Erase a genesis from a register address.
         *
         *  @param[in] hashGenesis The genesis-id address.
         *  @param[in] state The state register to read.
         *
         *  @return True if read was successful, false otherwise.
         *
         **/
        bool EraseTrust(const uint256_t& hashGenesis);


        /** HasState
         *
         *  Determines if a state exists in the register database.
         *
         *  @param[in] hashRegister The register address.
         *  @param[in] nFlags The flags for the register database.
         *
         *  @return True if it exists, false otherwise.
         *
         **/
        bool HasState(const uint256_t& hashRegister, const uint8_t nFlags = TAO::Ledger::FLAGS::BLOCK);


        /** IndexAddress
         *
         *  Handle a reindexing to add address to sequential reads. For -indexaddress flag.
         *
         *  @return true if indexing has completed.
         *
         **/
        void IndexAddress();


        /** MemoryBegin
         *
         *  Begin a memory transaction following ACID properties.
         *
         **/
        void MemoryBegin(const uint8_t nFlags = TAO::Ledger::FLAGS::MEMPOOL);


        /** MemoryRelease
         *
         *  Release a memory transaction following ACID properties.
         *
         **/
        void MemoryRelease(const uint8_t nFlags = TAO::Ledger::FLAGS::MEMPOOL);


        /** MemoryCommit
         *
         *  Commit a memory transaction following ACID properties.
         *
         **/
        void MemoryCommit();

    private:


        /** client_lookup
         *
         *  Does a -client mode lookup using lookup service.
         *
         *  @param[in] hashRegister The register address.
         *  @param[out] state The state register to read.
         *
         *  @return True if read was successful, false otherwise.
         *
         **/
        bool client_lookup(const uint256_t& hashRegister, TAO::Register::State& state, const uint8_t nFlags);


        /** get_address_type
         *
         *  Get a type string of the given address for sequential write keys.
         *
         *  @param [in] hashAddress The address to get type string for.
         *
         *  @return the type string of address.
         *
         **/
        std::string get_address_type(const uint256_t& hashAddress);

    };

}

#endif
