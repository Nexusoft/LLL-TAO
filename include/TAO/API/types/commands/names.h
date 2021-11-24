/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once

#include <TAO/API/types/base.h>

#include <TAO/Operation/types/contract.h>
#include <TAO/Register/types/object.h>
#include <TAO/Register/types/address.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /** Names
     *
     *  Names API Class.
     *  Manages the function pointers for all Asset commands.
     *
     **/
    class Names : public Derived<Names>
    {
    public:

        /** Default Constructor. **/
        Names()
        : Derived<Names>()
        {
        }


        /** Initialize.
         *
         *  Sets the function pointers for this API.
         *
         **/
        void Initialize() final;


        /** Name
         *
         *  Returns the name of this API.
         *
         **/
        static std::string Name()
        {
            return "names";
        }


        /* Generic handler for creating new indexes for this specific command-set. */
        void Index(const TAO::Operation::Contract& rContract, const uint32_t nContract) override;


        /** CreateName
         *
         *  Creates a new Name Object register for the given name and register address adds the register
         *  operation to the contract
         *
         *  @param[in] uint256_t hashGenesis The genesis hash of the signature chain to create the Name for
         *  @param[in] strName The Name of the object
         *  @param[in] strNamespace The Namespace to create the name in
         *  @param[in] hashRegister The register address that the Name object should resolve to
         *
         *  @return The contract to containing the Name object creation .
         *
         **/
        static TAO::Operation::Contract CreateName(const uint256_t& hashGenesis,
                                                   const std::string& strName,
                                                   const std::string& strNamespace,
                                                   const TAO::Register::Address& hashRegister);


        /** CreateName
         *
         *  Creates a new Name Object register for an object being transferred and adds it to a contract
         *
         *  @param[in] uint256_t hashGenesis The genesis hash of the signature chain to create the Name for
         *  @param[in] hashTransfer The transaction ID of the transfer transaction being claimed
         *
         *  @return The contract to containing the Name object creation .
         *
         **/
        static TAO::Operation::Contract CreateName(const uint256_t& hashGenesis,
                                                   const uint512_t& hashTransfer);


        /** GetName
         *
         *  Retrieves a Name object by name.
         *
         *  @param[in] params The json request params
         *  @param[in] strObjectName The name parameter to use in the register hash
         *  @param[out] hashRegister The register address of the Name object, if found
         *  @param[in] fThrow Flag indicating it should throw an exception if not found
         *
         *  @return The Name object .
         **/
        static TAO::Register::Object GetName(const encoding::json& jParams,
                                             const std::string& strObjectName,
                                             TAO::Register::Address& hashRegister,
                                             const bool fThrow = true);


        /** GetName
         *
         *  Retrieves a Name object by the register address, for a particular sig chain.
         *
         *  @param[in] hashGenesis The sig chain genesis hash
         *  @param[in] hashObject register address of the object to look up
         *  @param[out] hashNameObject The register address of the Name object, if found
         *
         *  @return The Name object .
         **/
        static TAO::Register::Object GetName(const uint256_t& hashGenesis,
                                             const TAO::Register::Address& hashObject,
                                             TAO::Register::Address& hashNameObject);



        /** ResolveAddress
         *
         *  Resolves a register address from a name by looking up a Name object and returning the address that it points to.
         *
         *  @param[in] params The json request params
         *  @param[in] strName The name parameter to use in the register hash
         *  @param[in] fThrow Flag indicating it should throw an exception if not found
         *
         *  @return The 256 bit hash of the object name.
         **/
        static TAO::Register::Address ResolveAddress(const encoding::json& jParams,
                                                     const std::string& strName,
                                                     const bool fThrow = true);


        /** ReverseLookup
         *
         *  Does a reverse name look-up by PTR records from names API logical indexes.
         *
         *  @param[in] hashAddress The address we are performing reverse lookup on.
         *  @param[out] strName The name returned with the lookup
         *
         *  @return true if the lookup succeeded with valid ptr records.
         *
         **/
        static bool ReverseLookup(const uint256_t& hashAddress, std::string &strName);



        /** ResolveName
         *
         *  Scans the Name records associated with the hashGenesis sig chain to find an entry with a matching hashObject address
         *
         *  @param[in] hashGenesis The sig chain genesis hash
         *  @param[in] hashRegister register address of the object to look up
         *
         *  @return the name of the object, if one is found
         *
         **/
        static std::string ResolveName(const uint256_t& hashGenesis, const TAO::Register::Address& hashRegister);


    };
}
