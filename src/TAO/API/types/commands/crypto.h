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
namespace TAO::API
{
    /** Crypto
     *
     *  Crypto API Class.
     *  Manages the function pointers for all Crypto API commands.
     *
     **/
    class Crypto : public Derived<Crypto>
    {
    public:

        /** Default Constructor. **/
        Crypto()
        : Derived<Crypto>()
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
            return "crypto";
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
        encoding::json List(const encoding::json& params, const bool fHelp);


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
        encoding::json Get(const encoding::json& params, const bool fHelp);


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
        encoding::json GetPublic(const encoding::json& params, const bool fHelp);


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
        encoding::json GetPrivate(const encoding::json& params, const bool fHelp);


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
        encoding::json Create(const encoding::json& params, const bool fHelp);


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
        encoding::json Encrypt(const encoding::json& params, const bool fHelp);


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
        encoding::json Decrypt(const encoding::json& params, const bool fHelp);


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
        encoding::json Sign(const encoding::json& params, const bool fHelp);


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
        encoding::json Verify(const encoding::json& params, const bool fHelp);


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
        encoding::json GetHash(const encoding::json& params, const bool fHelp);


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
        encoding::json ChangeScheme(const encoding::json& params, const bool fHelp);


        /** CreateCertificate
         *
         *  Generates an x509 certificate and stores a hash of the certificate data in the "cert" slot in the crypto register
         *
         *  @param[in] params The parameters from the API call
         *  @param[in] fHelp Trigger for help data.
         *
         *  @return The return object in JSON.
         *
         **/
        encoding::json CreateCertificate(const encoding::json& params, const bool fHelp);


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
        encoding::json GetCertificate(const encoding::json& params, const bool fHelp);


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
        encoding::json VerifyCertificate(const encoding::json& params, const bool fHelp);


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
        uint8_t get_scheme(const encoding::json& params );


    };
}
