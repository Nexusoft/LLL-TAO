/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2021

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "act the way you'd like to be and soon you'll be the way you act" - Leonard Cohen

____________________________________________________________________________________________*/

#pragma once

#include <TAO/Ledger/types/credentials.h>

#include <TAO/Register/types/object.h>

#include <Util/include/memory.h>

/* Global TAO namespace. */
namespace TAO::Register
{

    /** Crypto Register
     *
     *  Manages key verification and message encryption through the crypto object register
     *
     **/
    class Crypto : public Object
    {
    public:

        /** Default constructor. **/
        Crypto();


        /** Copy Constructor. **/
        Crypto(const Crypto& object);


        /** Move Constructor. **/
        Crypto(Crypto&& object) noexcept;


        /** Copy Assignment operator overload **/
        Crypto& operator=(const Crypto& object);


        /** Move Assignment operator overload **/
        Crypto& operator=(Crypto&& object) noexcept;


        /** Default Destructor **/
        ~Crypto();


        IMPLEMENT_SERIALIZE
        (
            READWRITE(nVersion);
            READWRITE(nType);
            READWRITE(hashOwner);
            READWRITE(nCreated);
            READWRITE(nModified);
            READWRITE(vchState);

            //checksum hash not serialized on gethash
            if(!(nSerType & SER_GETHASH))
                READWRITE(hashChecksum);
        )


        /** CheckCredentials
         *
         *  Checks our credentials against a given generate key to make sure they match.
         *
         *  @param[in] pCredentials The credential object to verify credentials.
         *  @param[in] strPIN The pin number to be used to verify credentials.
         *
         *  @return true if the signature is a valid signature for this crypto object register.
         *
         **/
        bool CheckCredentials(const memory::encrypted_ptr<TAO::Ledger::Credentials>& pCredentials, const SecureString& strPIN);


        /** VerifySignature
         *
         *  Verify signature data to a pubkey and valid key hash inside crypto object register.
         *
         *  @param[in] strKey The key name that we are checking for.
         *  @param[in] vData The data we need to check signature against.
         *  @param[in] vPubKey The byte vector of the public key.
         *  @param[in] vSig The byte vector of the signature to check.
         *
         *  @return true if the signature is a valid signature for this crypto object register.
         *
         **/
        bool VerifySignature(const std::string& strKey, const bytes_t& vData, const bytes_t& vPubKey, const bytes_t& vSig);


        /** GenerateSignature
         *
         *  Generate a signature from signature chain credentials with valid key hash.
         *
         *  @param[in] strKey The key name that we are checking for.
         *  @param[in] pCredentials The credential object to generate signature.
         *  @param[in] strPIN The pin number to be used to generate signature.
         *  @param[in] vData The data we need to generate signature for.
         *  @param[out] vPubKey The byte vector of the public key to generate.
         *  @param[out] vSig The byte vector of the signature to generate.
         *
         *  @return true if the signature was generated correctly from crypto object register.
         *
         **/
        bool GenerateSignature(const std::string& strKey, const memory::encrypted_ptr<TAO::Ledger::Credentials>& pCredentials,
                               const SecureString& strPIN, const bytes_t& vData, bytes_t &vPubKey, bytes_t &vSig);
    };
}
