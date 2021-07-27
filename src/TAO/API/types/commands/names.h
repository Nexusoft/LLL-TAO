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


        /** RewriteURL
         *
         *  Allows derived API's to handle custom/dynamic URL's where the strMethod does not
         *  map directly to a function in the target API.  Insted this method can be overriden to
         *  parse the incoming URL and route to a different/generic method handler, adding parameter
         *  values if necessary.  E.g. get/myasset could be rerouted to get/asset with name=myasset
         *  added to the jsonParams
         *  The return json contains the modifed method URL to be called.
         *
         *  @param[in] strMethod The name of the method being invoked.
         *  @param[in] jsonParams The json array of parameters being passed to this method.
         *
         *  @return the API method URL
         *
         **/
        std::string RewriteURL(const std::string& strMethod, encoding::json& jsonParams) override;


        /** Create
         *
         *  Create a name
         *
         *  @param[in] params The parameters from the API call.
         *  @param[in] fHelp Trigger for help data.
         *
         *  @return The return object in JSON.
         *
         **/
        encoding::json Create(const encoding::json& params, const bool fHelp);


        /** Get
         *
         *  Get the data from a name.  NOTE the intentional naming of this method that does not fit with the convention of
         *  the rest of the class - this is intentional as GetName is already a base class method that has been marked as final.
         *
         *  @param[in] params The parameters from the API call.
         *  @param[in] fHelp Trigger for help data.
         *
         *  @return The return object in JSON.
         *
         **/
        encoding::json Get(const encoding::json& params, const bool fHelp);


        /** UpdateName
         *
         *  Update the register address in a name
         *
         *  @param[in] params The parameters from the API call.
         *  @param[in] fHelp Trigger for help data.
         *
         *  @return The return object in JSON.
         *
         **/
        encoding::json UpdateName(const encoding::json& params, const bool fHelp);


        /** TransferName
         *
         *  Transfer a name
         *
         *  @param[in] params The parameters from the API call.
         *  @param[in] fHelp Trigger for help data.
         *
         *  @return The return object in JSON.
         *
         **/
        encoding::json TransferName(const encoding::json& params, const bool fHelp);


        /** ClaimName
         *
         *  Claim a transferred name .
         *
         *  @param[in] params The parameters from the API call.
         *  @param[in] fHelp Trigger for help data.
         *
         *  @return The return object in JSON.
         *
         **/
        encoding::json ClaimName(const encoding::json& params, const bool fHelp);


        /** NameHistory
         *
         *  History of an name and its ownership
         *
         *  @param[in] params The parameters from the API call.
         *  @param[in] fHelp Trigger for help data.
         *
         *  @return The return object in JSON.
         *
         **/
        encoding::json NameHistory(const encoding::json& params, const bool fHelp);


        /** CreateNamespace
         *
         *  Create a namespace
         *
         *  @param[in] params The parameters from the API call.
         *  @param[in] fHelp Trigger for help data.
         *
         *  @return The return object in JSON.
         *
         **/
        encoding::json CreateNamespace(const encoding::json& params, const bool fHelp);


        /** GetNamespace
         *
         *  Get the data from a namespace.
         *
         *  @param[in] params The parameters from the API call.
         *  @param[in] fHelp Trigger for help data.
         *
         *  @return The return object in JSON.
         *
         **/
        encoding::json GetNamespace(const encoding::json& params, const bool fHelp);


        /** TransferNamespace
         *
         *  Transfer a name space
         *
         *  @param[in] params The parameters from the API call.
         *  @param[in] fHelp Trigger for help data.
         *
         *  @return The return object in JSON.
         *
         **/
        encoding::json TransferNamespace(const encoding::json& params, const bool fHelp);


        /** ClaimNamespace
         *
         *  Claim a transferred namespace .
         *
         *  @param[in] params The parameters from the API call.
         *  @param[in] fHelp Trigger for help data.
         *
         *  @return The return object in JSON.
         *
         **/
        encoding::json ClaimNamespace(const encoding::json& params, const bool fHelp);


        /** NamespaceHistory
         *
         *  History of an namespace and its ownership
         *
         *  @param[in] params The parameters from the API call.
         *  @param[in] fHelp Trigger for help data.
         *
         *  @return The return object in JSON.
         *
         **/
        encoding::json NamespaceHistory(const encoding::json& params, const bool fHelp);



        /******************* STATIC HELPER METHODS BELOW THIS LINE **********************/

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
        static TAO::Register::Object GetName(const encoding::json& params,
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
        static TAO::Register::Address ResolveAddress(const encoding::json& params,
                                                     const std::string& strName,
                                                     const bool fThrow = true);


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


        /** ResolveAccountTokenName
         *
         *  Retrieves the token name for the token that this account object is used for.
         *  The token is obtained by looking at the token_address field,
         *  which contains the register address of the issuing token
         *
         *  @param[in] params The json request params
         *  @param[in] account The Object Register of the token account
         *
         *  @return the token name for the token that this account object is used for
         *
         **/
        static std::string ResolveAccountTokenName(const encoding::json& params, const TAO::Register::Object& account);


    };
}