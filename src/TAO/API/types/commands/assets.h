/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2023

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once

#include <TAO/API/types/base.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /** Assets
     *
     *  Assets API Class.
     *  Manages the function pointers for all Asset commands.
     *
     **/
    class Assets : public Derived<Assets>
    {
    public:

        /** Default Constructor. **/
        Assets()
        : Derived<Assets>()
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
            return "assets";
        }


        /** Create
         *
         *  Create an asset or digital item.
         *
         *  @param[in] jParams The parameters from the API call.
         *  @param[in] fHelp Trigger for help data.
         *
         *  @return The return object in JSON.
         *
         **/
        encoding::json Create(const encoding::json& jParams, const bool fHelp);


        /** Partial
         *
         *  List assets with fungible tokens that represent ownership.
         *
         *  @param[in] jParams The parameters from the API call.
         *  @param[in] fHelp Trigger for help data.
         *
         *  @return The return object in JSON.
         *
         **/
        encoding::json Partial(const encoding::json& jParams, const bool fHelp);


        /** Tokenize
         *
         *  Tokenize an asset into fungible tokens that represent ownership.
         *
         *  @param[in] jParams The parameters from the API call.
         *  @param[in] fHelp Trigger for help data.
         *
         *  @return The return object in JSON.
         *
         **/
        encoding::json Tokenize(const encoding::json& jParams, const bool fHelp);


        /** Update
         *
         *  Update the data in an asset
         *
         *  @param[in] jParams The parameters from the API call.
         *  @param[in] fHelp Trigger for help data.
         *
         *  @return The return object in JSON.
         *
         **/
        encoding::json Update(const encoding::json& jParams, const bool fHelp);


        /** Verify
         *
         *  Verify an asset is currently tokenized for given asset.
         *
         *  @param[in] jParams The parameters from the API call.
         *  @param[in] fHelp Trigger for help data.
         *
         *  @return The return object in JSON.
         *
         **/
        encoding::json Verify(const encoding::json& jParams, const bool fHelp);


        /** SchemaToJSON
         *
         *  Returns the JSON representation of a given schema
         *
         *  @param[in] rObject The state register containing the schema data
         *  @param[in] hashRegister The register address of the schema register
         *
         *  @return the invoice JSON
         *
         **/
        static encoding::json SchemaToJSON(const TAO::Register::Object& rObject, const uint256_t& hashRegister);

    };
}
