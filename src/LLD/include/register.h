/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLD_INCLUDE_REGISTER_H
#define NEXUS_LLD_INCLUDE_REGISTER_H

#include <LLC/types/uint1024.h>

#include <LLD/templates/sector.h>

#include <TAO/Register/include/enum.h>
#include <TAO/Register/types/state.h>


namespace LLD
{

    /** RegisterDB
     *
     *  The database class for the Register Layer.
     *
     **/
    class RegisterDB : public SectorDatabase<BinaryHashMap, BinaryLRU>
    {
        std::mutex MEMORY_MUTEX;

        std::map<uint256_t, TAO::Register::State> mapStates;
        std::map<uint256_t, uint256_t> mapIdentifiers;

    public:


        /** The Database Constructor. To determine file location and the Bytes per Record. **/
        RegisterDB(uint8_t nFlags = FLAGS::CREATE);


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


        /** WriteIdentifier
         *
         *  Writes a token identifier to the register database.
         *
         *  @param[in] nIdentifier The token identifier.
         *  @param[in] hashRegister The register address of the token.
         *
         *  @return True if write was successful, false otherwise.
         *
         **/
        bool WriteIdentifier(const uint256_t nIdentifier, const uint256_t& hashRegister, const uint8_t nFlags = TAO::Ledger::FLAGS::BLOCK);


        /** EraseIdentifier
         *
         *  Erase a token identifier to the register database.
         *
         *  @param[in] nIdentifier The token identifier.
         *
         *  @return True if write was successful, false otherwise.
         *
         **/
        bool EraseIdentifier(const uint256_t nIdentifier);


        /** ReadIdentifier
         *
         *  Read a token identifier from the register database.
         *
         *  @param[in] nIdentifier The token identifier.
         *  @param[out] hashRegister The register address of token.
         *  @param[in] nFlags The flags for the register database.
         *
         *  @return True if read was successful, false otherwise.
         *
         **/
        bool ReadIdentifier(const uint256_t nIdentifier, uint256_t& hashRegister, const uint8_t nFlags = TAO::Ledger::FLAGS::BLOCK);


        /** HasIdentifier
         *
         *  Determines if an identifier exists in the database.
         *
         *  @param[in] nIdentifier The token identifier.
         *  @param[in] nFlags The flags for the register database.
         *
         *  @return True if it exists, false otherwise.
         *
         **/
        bool HasIdentifier(const uint256_t nIdentifier, const uint8_t nFlags = TAO::Ledger::FLAGS::BLOCK);


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


        /** GetStates
         *
         *  Get the previous states of a register.
         *
         *  @param[in] hashRegister The register address.
         *  @param[out] states The states vector to return.
         *
         *  @return True if any states were found, false otherwise.
         *
         **/
        bool GetStates(const uint256_t& hashRegister, std::vector<TAO::Register::State>& states, const uint8_t nFlags = TAO::Ledger::FLAGS::BLOCK);

    };

}

#endif
