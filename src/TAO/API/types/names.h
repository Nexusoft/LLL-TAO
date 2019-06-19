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

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {
        /** Names
         *
         *  Names API Class.
         *  Manages the function pointers for all Asset commands.
         *
         **/
        class Names : public Base
        {
        public:

            /** Default Constructor. **/
            Names() { Initialize(); }


            /** Initialize.
             *
             *  Sets the function pointers for this API.
             *
             **/
            void Initialize() final;


            /** GetName
             *
             *  Returns the name of this API.
             *
             **/
            std::string GetName() const final
            {
                return "Names";
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
            std::string RewriteURL(const std::string& strMethod, json::json& jsonParams) override;


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
            json::json Create(const json::json& params, bool fHelp);


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
            json::json Get(const json::json& params, bool fHelp);


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
            json::json UpdateName(const json::json& params, bool fHelp);


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
            json::json TransferName(const json::json& params, bool fHelp);


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
            json::json ClaimName(const json::json& params, bool fHelp);


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
            json::json NameHistory(const json::json& params, bool fHelp);


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
            json::json CreateNamespace(const json::json& params, bool fHelp);


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
            json::json GetNamespace(const json::json& params, bool fHelp);


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
            json::json TransferNamespace(const json::json& params, bool fHelp);


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
            json::json ClaimNamespace(const json::json& params, bool fHelp);


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
            json::json NamespaceHistory(const json::json& params, bool fHelp);



            /******************* STATIC HELPER METHODS BELOW THIS LINE **********************/

            /** CreateName
             *
             *  Creates a new Name Object register for the given name and register address adds the register 
             *  operation to the contract
             *
             *  @param[in] uint256_t hashGenesis The genesis hash of the signature chain to create the Name for
             *  @param[in] strFullName The Name of the object.  May include the namespace suffix
             *  @param[in] hashRegister The register address that the Name object should resolve to
             *  
             *  @return The contract to containing the Name object creation .
             *
             **/
            static TAO::Operation::Contract CreateName(const uint256_t& hashGenesis, 
                                                       const std::string& strFullName,
                                                       const uint256_t& hashRegister);


            /** CreateName
             *
             *  Creates a new Name Object register for an object being transferred and adds it to a contract
             *
             *  @param[in] uint256_t hashGenesis The genesis hash of the signature chain to create the Name for
             *  @param[in] params The json request params
             *  @param[in] hashTransfer The transaction ID of the transfer transaction being claimed
             *
             *  @return The contract to containing the Name object creation .
             *
             **/
            static TAO::Operation::Contract CreateName(const uint256_t& hashGenesis,
                                                       const json::json& params, 
                                                       const uint512_t& hashTransfer);


            /** GetName
             *
             *  Retrieves a Name object by name.
             *
             *  @param[in] params The json request params
             *  @param[in] strObjectName The name parameter to use in the register hash
             *  @param[out] hashRegister The register address of the Name object, if found
             *
             *  @return The Name object .
             **/
            static TAO::Register::Object GetName(const json::json& params, 
                                                 const std::string& strObjectName, 
                                                 uint256_t& hashRegister);


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
                                                 const uint256_t& hashObject, 
                                                 uint256_t& hashNameObject);
            
            

            /** ResolveAddress
             *
             *  Resolves a register address from a name by looking up a Name object and returning the address that it points to.
             *
             *  @param[in] params The json request params
             *  @param[in] strName The name parameter to use in the register hash
             *
             *  @return The 256 bit hash of the object name.
             **/
            static uint256_t ResolveAddress(const json::json& params, const std::string& strName);


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
            static std::string ResolveName(const uint256_t& hashGenesis, const uint256_t& hashRegister);


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
            static std::string ResolveAccountTokenName(const json::json& params, const TAO::Register::Object& account);
            

        };
    }
}
