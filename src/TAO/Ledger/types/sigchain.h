/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_TAO_LEDGER_TYPES_SIGNATURE_CHAIN_H
#define NEXUS_TAO_LEDGER_TYPES_SIGNATURE_CHAIN_H

#include <string>

#include <LLC/hash/SK.h>
#include <LLC/hash/macro.h>

#include <Util/include/allocators.h>

/* Global TAO namespace. */
namespace TAO
{

    /* Ledger Layer namespace. */
    namespace Ledger
    {

        /** SignatureChain
         *
         *  Base Class for a Signature SignatureChain
         *
         *  Similar to HD wallet systems, but integrated into the Layer 2 of the nexus software stack.
         *  Seed phrase includes at least 128 bits of entropy (8 char username, 8 char password) and pin
         *  to attack a signature chain by dictionary attack.
         *
         */
        class SignatureChain
        {

            /** Secure allocator to represent the username of this signature chain. **/
            SecureString strUsername;


            /** Secure allocater to represent the password of this signature chain. **/
            SecureString strPassword;

        public:


            /** Constructor to generate Keychain
             *
             * @param[in] strUsernameIn The username to seed the signature chain
             * @param[in] strPasswordIn The password to seed the signature chain
             **/
            SignatureChain(SecureString strUsernameIn, SecureString strPasswordIn)
            : strUsername(strUsernameIn.c_str())
            , strPassword(strPasswordIn.c_str())
            {

            }


            /** Genesis
             *
             *  This function is responsible for generating the genesis ID.
             *
             *  @return The 512 bit hash of this key in the series.
             **/
            uint256_t Genesis()
            {
                /* Generate the Secret Phrase */
                std::vector<uint8_t> vSecret(strUsername.begin(), strUsername.end());
                vSecret.insert(vSecret.end(), strPassword.begin(), strPassword.end());

                /* Generate the Hashes */
                uint1024_t hashSecret = LLC::SK1024(vSecret);

                /* Generate the Final Root Hash. */
                return LLC::SK256(hashSecret.GetBytes());
            }


            /** Generate
             *
             *  This function is responsible for genearting the private key in the keychain of a specific account.
             *  The keychain is a series of keys seeded from a secret phrase and a PIN number.
             *
             *  @param[in] nKeyID The key number in the keychian
             *  @param[in] strSecret The secret phrase to use (Never Cached)
             *
             *  @return The 512 bit hash of this key in the series.
             **/
            uint512_t Generate(uint32_t nKeyID, SecureString strSecret)
            {
                /* Serialize the Key ID (Big Endian). */
                std::vector<uint8_t> vKeyID((uint8_t*)&nKeyID, (uint8_t*)&nKeyID + sizeof(nKeyID));

                /* Generate the Secret Phrase */
                std::vector<uint8_t> vSecret(strUsername.begin(), strUsername.end());
                vSecret.insert(vSecret.end(), vKeyID.begin(), vKeyID.end());
                vSecret.insert(vSecret.end(), strPassword.begin(), strPassword.end());

                /* Generate the secret data. */
                std::vector<uint8_t> vPin(strSecret.begin(), strSecret.end());
                vPin.insert(vPin.end(), vKeyID.begin(), vKeyID.end());

                /* Generate the Hashes */
                uint1024_t hashSecret = LLC::SK1024(vSecret);
                uint1024_t hashPIN    = LLC::SK1024(vPin);

                std::vector<uint8_t> vFinal;
                vFinal.insert(vFinal.end(), (uint8_t*)&hashSecret, (uint8_t*)&hashSecret + 128);
                vFinal.insert(vFinal.end(), (uint8_t*)&hashPIN, (uint8_t*)&hashPIN + 128);

                /* Generate the Final Root Hash. */
                return LLC::SK512(vFinal);
            }
        };
    }
}

#endif
