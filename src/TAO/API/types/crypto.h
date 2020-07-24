/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once


#include <TAO/API/types/base.h>

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {
        /** Crypto
         *
         *  Crypto API Class.
         *  Manages the function pointers for all Crypto API commands.
         *
         **/
        class Crypto : public Base
        {
        public:

            /** Default Constructor. **/
            Crypto()
            : Base()
            {
                Initialize();
            }


            /** Initialize.
             *
             *  Sets the function pointers for this API.
             *
             **/
            void Initialize() final;


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


            /** GetName
             *
             *  Returns the name of this API.
             *
             **/
            std::string GetName() const final
            {
                return "Crypto";
            }


            /** List
             *
             *  List the public keys stored in the crypto object register
             *
             *  @param[in] params The parameters from the API call
             *  @param[in] fHelp Trigger for help data.
             *
             *  @return The return object in JSON.
             *
             **/
            json::json List(const json::json& params, bool fHelp);


            /** Get
             *
             *  Returns the public key from the crypto object register for the specified key name, from the specified signature chain
             *
             *  @param[in] params The parameters from the API call
             *  @param[in] fHelp Trigger for help data.
             *
             *  @return The return object in JSON.
             *
             **/
            json::json Get(const json::json& params, bool fHelp);


            /** GetPublic
             *
             *  Generates and returns the public key for a key stored in the crypto object register
             *
             *  @param[in] params The parameters from the API call
             *  @param[in] fHelp Trigger for help data.
             *
             *  @return The return object in JSON.
             *
             **/
            json::json GetPublic(const json::json& params, bool fHelp);
            

            /** GetPrivate
             *
             *  Generates and returns the private key for a key stored in the crypto object register
             *
             *  @param[in] params The parameters from the API call
             *  @param[in] fHelp Trigger for help data.
             *
             *  @return The return object in JSON.
             *
             **/
            json::json GetPrivate(const json::json& params, bool fHelp);


            /** Create
             *
             *  Generates private key based on keyname/user/pass/pin and stores it in the keyname slot in the crypto register
             *
             *  @param[in] params The parameters from the API call
             *  @param[in] fHelp Trigger for help data.
             *
             *  @return The return object in JSON.
             *
             **/
            json::json Create(const json::json& params, bool fHelp);


            /** Encrypt
             *
             *  Encrypts data using the specified public key 
             *
             *  @param[in] params The parameters from the API call
             *  @param[in] fHelp Trigger for help data.
             *
             *  @return The return object in JSON.
             *
             **/
            json::json Encrypt(const json::json& params, bool fHelp);


            /** Decrypt
             *
             *  Decrypts data using the local private key 
             *
             *  @param[in] params The parameters from the API call
             *  @param[in] fHelp Trigger for help data.
             *
             *  @return The return object in JSON.
             *
             **/
            json::json Decrypt(const json::json& params, bool fHelp);


            /** Sign
             *
             *  Generates a signature for the data based on the private key for the keyname/user/pass/pin combination 
             *
             *  @param[in] params The parameters from the API call
             *  @param[in] fHelp Trigger for help data.
             *
             *  @return The return object in JSON.
             *
             **/
            json::json Sign(const json::json& params, bool fHelp);


            /** Verify
             *
             *  Verifies the signature is correct for the specified public key and data
             *
             *  @param[in] params The parameters from the API call
             *  @param[in] fHelp Trigger for help data.
             *
             *  @return The return object in JSON.
             *
             **/
            json::json Verify(const json::json& params, bool fHelp);


            /** GetHash
             *
             *  Generates a hash of the data using the requested hashing function
             *
             *  @param[in] params The parameters from the API call
             *  @param[in] fHelp Trigger for help data.
             *
             *  @return The return object in JSON.
             *
             **/
            json::json GetHash(const json::json& params, bool fHelp);


            /** ChangeScheme
             *
             *  Change the signature scheme used to generate the public-private keys for the users signature chain 
             *  as well as keys stored in the crypto object register
             *
             *  @param[in] params The parameters from the API call
             *  @param[in] fHelp Trigger for help data.
             *
             *  @return The return object in JSON.
             *
             **/
            json::json ChangeScheme(const json::json& params, bool fHelp);



            /** GetCertificate
             *
             *  Returns the last generated x509 certificate for this sig chain
             * 
             *  @param[in] params The parameters from the API call
             *  @param[in] fHelp Trigger for help data.
             *
             *  @return The return object in JSON.
             *
             **/
            json::json GetCertificate(const json::json& params, bool fHelp);


            /** VerifyCertificate
             *
             *  Verifies the x509 certificate
             * 
             *  @param[in] params The parameters from the API call
             *  @param[in] fHelp Trigger for help data.
             *
             *  @return The return object in JSON.
             *
             **/
            json::json VerifyCertificate(const json::json& params, bool fHelp);


        /* private helper methods */
        private:

            /** get_scheme
             *
             *  Determines the scheme (key type) to use for constructing keys.
             *
             *  @param[in] params The parameters from the API call
             *  @param[in] fHelp Trigger for help data.
             *
             *  @return The return object in JSON.
             *
             **/
            uint8_t get_scheme(const json::json& params );


        };
    }
}
