/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/
#pragma once

#include <LLC/types/uint1024.h>

/* Forward Declarations. */
namespace TAO::Register { class Object; class State; }


/* Global TAO namespace. */
namespace TAO::API
{

    /** GetFigures
     *
     *  Converts the decimals from an object into raw figures using power function
     *
     *  @param[in] object The Object Register to derive the figures from
     *
     *  @return the whole 64-bit value with figures expanded
     *
     **/
    uint64_t GetFigures(const TAO::Register::Object& object);


    /** GetDecimals
     *
     *  Retrieves the number of decimals that applies to amounts for this token or account object.
     *  If the object register passed in is a token account then we need to look at the token definition
     *
     *  @param[in] object The Object Register to determine the decimals for
     *
     *  @return the number of decimals that apply to amounts for this token or account
     *
     **/
    uint8_t GetDecimals(const TAO::Register::Object& object);


    /** GetStandardType
     *
     *  Get the type standardized into object if applicable.
     *
     *  @param[in] rObject The object we are extracting for.
     *
     *  @return the given standard type.
     *
     **/
    uint16_t GetStandardType(const TAO::Register::Object& rObject);


    /** GetPending
     *
     *  Get the sum of all debit notifications for the the specified token
     *
     *  @param[in] hashGenesis The genesis hash for the sig chain owner.
     *  @param[in] hashToken The token to find the pending for .
     *  @param[in] hashAccount Optional account/token register to filter on
     *
     *  @return The sum of all pending debits
     *
     **/
    uint64_t GetPending(const uint256_t& hashGenesis, const uint256_t& hashToken, const uint256_t& hashAccount = 0);


    /** GetUnconfirmed
     *
     *  Get the sum of all incoming/outgoing debit transactions and outgoing credits in the mempool for the the specified token
     *
     *  @param[in] hashGenesis The genesis hash for the sig chain owner.
     *  @param[in] hashToken The token to find the pending for.
     *  @param[in] fOutgoing Flag indicating to include outgoing debits, i.e. debits in the mempool that we have made.
     *  @param[in] hashAccount Optional account/token register to filter on
     *
     *  @return The sum of all pending debits
     *
     **/
    uint64_t GetUnconfirmed(const uint256_t& hashGenesis, const uint256_t& hashToken,
                            const bool fOutgoing, const uint256_t& hashAccount = 0);


    /** GetImmature
     *
     *  Get the sum of all immature coinbase transactions
     *
     *  @param[in] hashGenesis The genesis hash for the sig chain owner.
     *
     *  @return The sum of all immature coinbase transactions
     *
     **/
    uint64_t GetImmature(const uint256_t& hashGenesis);


    /** GetTokenOwnership
    *
    *  Calculates the percentage of tokens owned from the total supply.
    *
    *  @param[in] hashToken The address of the token to check
    *  @param[in] hashGenesis The genesis hash for the sig chain to check
    *
    *  @return The percentage of tokens owned from the total supply
    *
    **/
    double GetTokenOwnership(const TAO::Register::Address& hashToken, const uint256_t& hashGenesis);


    /** GetRegisters
     *
     *  Reads a batch of states registers from the Register DB
     *
     *  @param[in] vAddresses The list of register addresses to read
     *  @param[out] vStates The list of states paired to the register address.
     *              The list will be sorted by the create timestamp of the register, oldest first
     *
     *  @return True if successful
     *
     **/
    bool GetRegisters(const std::vector<TAO::Register::Address>& vAddresses,
                      std::vector<std::pair<TAO::Register::Address, TAO::Register::State>>& vStates);


    /** GetAccountByToken
     *
     *  Searches the sig chain for the first account for the given token type
     *
     *  @param[in] hashGenesis The genesis hash of the sig chain to search
     *  @param[in] hashToken The token type to search for
     *  @param[out] hashAccount Populated with the address of the account, if found
     *
     *  @return Boolean indicating whether an account was found or not
     *
     **/
    bool GetAccountByToken(const uint256_t& hashGenesis, const uint256_t& hashToken, TAO::Register::Address& hashAccount);


    /** GetRegisterName
     *
     *  Returns a type string for the register type
     *
     *  @param[in] nType The register type enum
     *
     *  @return A string representation of the register type
     *
     **/
    std::string GetRegisterName(const uint8_t nType);


    /** GetStandardName
     *
     *  Returns a type string for the register object type
     *
     *  @param[in] nType The object type enum
     *
     *  @return A string representation of the object register type
     *
     **/
    std::string GetStandardName(const uint8_t nType);

}
