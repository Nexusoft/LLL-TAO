/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/
#pragma once

#include <Util/include/json.h>

#include <TAO/Register/types/object.h>
#include <TAO/Register/types/address.h>
#include <TAO/Ledger/types/transaction.h>

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        /** IsRegisterAddress
         *
         *  Determines whether a string value is a valid base58 encoded register address.
        *  This only checks to see if the value an be decoded into a valid Register::Address with a valid Type.
        *  It does not check to see whether the register address exists in the database
         *
         *  @param[in] strValueToCheck The value to check
         *
         *  @return True if the value can be decoded into a valid Register::Address with a valid Type.
         **/
        bool IsRegisterAddress(const std::string& strValueToCheck);


        /** GetDecimals
         *
         *  Retrieves the number of decimals that applies to amounts for this token or account object.
         *  If the object register passed in is a token account then we need to look at the token definition
         *  in order to get the decimals.  The token is obtained by looking at the token_address field,
         *  which contains the register address of the issuing token
         *
         *  @param[in] object The Object Register to determine the decimals for
         *
         *  @return the number of decimals that apply to amounts for this token or account
         *
         **/
        uint8_t GetDecimals(const TAO::Register::Object& object);


        /** ListRegisters
         *
         *  Scans a signature chain to work out all registers that it owns
         *
         *  @param[in] hashGenesis The genesis hash of the signature chain to scan
         *  @param[out] vRegisters The list of register addresses from sigchain.
         *
         *  @return A vector of register addresses owned by the sig chain
         *
         **/
        bool ListRegisters(const uint256_t& hashGenesis, std::vector<TAO::Register::Address>& vRegisters);


        /** ListObjects
         *
         *  Scans a signature chain to work out all non-standard object that it owns
         *
         *  @param[in] hashGenesis The genesis hash of the signature chain to scan
         *  @param[out] vObjects The list of object register addresses from sigchain.
         *
         *  @return A vector of register addresses owned by the sig chain
         *
         **/
        bool ListObjects(const uint256_t& hashOwner, std::vector<TAO::Register::Address>& vObjects);


        /** ListAccounts
         *
         *  Scans a signature chain to work out all assets that it owns
         *
         *  @param[in] hashGenesis The genesis hash of the signature chain to scan
         *  @param[in] fTokens If set then this will include tokens in the list
         *  @param[in] fTrust If set then this will include trust accounts in the list
         *  @param[out] vAccounts The list of account register addresses from sigchain.
         *
         *  @return A vector of register addresses owned by the sig chain
         *
         **/
        bool ListAccounts(const uint256_t& hashGenesis, std::vector<TAO::Register::Address>& vAccounts, 
                          bool fTokens, bool fTrust);


        /** AddFee
         *
         *  Calculates the required fee for the transaction and adds the OP::FEE contract to the transaction if necessary.
         *  The method will lookup the "default" NXS account and use this account to pay the fees.  An exception will be thrown
         *  If there are insufficient funds to pay the fee.
         *
         *  @param[in] tx The transaction to add the fee to
         *
         *  @return true if the fee was successfully added, otherwise false
         *
         **/
        bool AddFee(TAO::Ledger::Transaction& tx);


        /** RegisterType
         *
         *  Returns a type string for the register type
         *
         *  @param[in] nType The register type enum
         *
         *  @return A string representation of the register type
         *
         **/  
        std::string RegisterType(uint8_t nType);


        /** ObjectType
         *
         *  Returns a type string for the register object type
         *
         *  @param[in] nType The object type enum
         *
         *  @return A string representation of the object register type
         *
         **/
        std::string ObjectType(uint8_t nType);


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
        uint64_t GetUnconfirmed(const uint256_t& hashGenesis, 
                                const uint256_t& hashToken, 
                                bool fOutgoing, 
                                const uint256_t& hashAccount = 0);


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


        /** ListPartial
         *
         *  Lists all object registers partially owned by way of tokens that the sig chain owns 
         *
         *  @param[in] hashGenesis The genesis hash of the signature chain to scan
         *  @param[out] vObjects The list of object register addresses from sigchain.
         *
         *  @return A vector of register addresses partially owned by the sig chain
         *
         **/
        bool ListPartial(const uint256_t& hashGenesis, std::vector<TAO::Register::Address>& vRegisters);


        /** ListTokenizedObjects
         *
         *  Finds all objects that have been tokenized and therefore owned by hashToken
         *
         *  @param[in] hashToken The token to find objects for
         *  @param[out] vObjects The list of object register addresses owned by the token.
         *
         *  @return A vector of register addresses owned by the token
         *
         **/
        bool ListTokenizedObjects(const TAO::Register::Address& hashToken, 
                                  std::vector<TAO::Register::Address>& vObjects);


        /** CheckMature
        *
        *  Utilty method that checks that the signature chain is mature and can therefore create new transactions.
        *  Throws an appropriate APIException if it is not mature
        *
        *  @param[in] hashGenesis The genesis hash of the signature chain to check
        *
        **/
        void CheckMature(const uint256_t& hashGenesis);


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


        /** VoidContract
        *
        *  Creates a void contract for the specified transaction
        *
        *  @param[in] contract The contract to void
        *  @param[in] the ID of the contract in the transaction
        *  @param[out] voidContract The void contract to be created
        * 
        *  @return True if a void contract was created.
        *
        **/
        bool VoidContract(const TAO::Operation::Contract& contract, const uint32_t nContract, TAO::Operation::Contract &voidContract);


    }
}
