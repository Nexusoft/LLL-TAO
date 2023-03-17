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


        /** Lookup
         *
         *  Lookup any record using our internal names indexing system.
         *
         *  @param[in] jParams The parameters from the API call.
         *  @param[in] fHelp Trigger for help data.
         *
         *  @return The return object in JSON.
         *
         **/
        encoding::json Lookup(const encoding::json& jParams, const bool fHelp);


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

        /** GetName
         *
         *  Retrieves a Name object by name.
         *
         *  @param[in] jParams The json request parameters
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



        /** ResolveAddress
         *
         *  Resolves a register address from a name by looking up a Name object and returning the address that it points to.
         *
         *  @param[in] jParams The json request parameters
         *  @param[in] strName The name parameter to use in the register hash
         *  @param[in] fThrow Flag indicating it should throw an exception if not found
         *
         *  @return The 256 bit hash of the object name.
         **/
        static TAO::Register::Address ResolveAddress(const encoding::json& jParams, const std::string& strName,
                                                     const bool fThrow = true);


        /** ResolveNamespace
         *
         *  Resolves a register address from a namespace by looking up the namespace using incoming parameters
         *
         *  @param[in] jParams The json request parameters
         *
         *  @return The 256 bit hash of the object name.
         **/
        static TAO::Register::Address ResolveNamespace(const encoding::json& jParams);


        /** ResolveNamespace
         *
         *  Resolves a register address from a namespace by looking up the namespace by hashing namespace name
         *
         *  @param[in] strNamespace The namespace parameter to use in the register hash
         *
         *  @return The 256 bit hash of the object name.
         **/
        static TAO::Register::Address ResolveNamespace(const std::string& strNamespace);


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


        /** ReverseLookup
         *
         *  Does a reverse name look-up by PTR records from names API logical indexes.
         *
         *  @param[in] hashAddress The address we are performing reverse lookup on.
         *  @param[out] jRet The returned JSON to add our name keys to.
         *
         *  @return true if the lookup succeeded with valid ptr records.
         *
         **/
        static bool ReverseLookup(const uint256_t& hashAddress, encoding::json &jRet);



        /** NameToJSON
         *
         *  Output all the name related json data to a json object.
         *
         *  @param[in] rName The name object to build JSON data about.
         *  @param[out] jRet The returned JSON to add our name keys to.
         *
         *  @return true if the lookup succeeded with valid ptr records.
         *
         **/
        static void NameToJSON(const TAO::Register::Object& rName, encoding::json &jRet);

    };
}
