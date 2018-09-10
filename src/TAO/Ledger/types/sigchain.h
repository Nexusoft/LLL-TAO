/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <string>

#include "../../../LLC/hash/SK.h"
#include "../../../LLC/hash/macro.h"

namespace TAO
{

    namespace Ledger
    {
        /** Base Class for a Signature SignatureChain
        *
        *  Similar to HD wallet systems, but integrated into the Layer 2 of the nexus software stack.
        *  Seed phrase includes at least 128 bits of entropy (8 char username, 8 char password) and pin
        *  to attack a signature chain by dictionary attack.
        *
        */
        class SignatureChain
        {

            /** String to represent the username of this signature chain. **/
            std::string strUsername;


            /** String to represent the password of this signature chain. **/
            std::string strPassword;

        public:
            /** Constructor to generate Keychain
            *
            * @param[in] strUsernameIn The username to seed the signature chain
            * @param[in] strPasswordIn The password to seed the signature chain
            */
            SignatureChain(std::string strUsernameIn, std::string strPasswordIn) : strUsername(strUsernameIn), strPassword(strPasswordIn) {}


            /** Generate Function
            *
            * This function is responsible for genearting the privat ekey in the keychain of a specific account.
            * The keychain is a series of keys seeded from a secret phrase and a PIN number.
            *
            * @param[in] nKeyID The key number in the keychian
            * @param[in] strSecret The secret phrase to use (Never Cached)
            *
            * @return The 512 bit hash of this key in the series.
            */
            uint512_t Generate(uint32_t nKeyID, std::string strSecret)
            {
                /* Serialize the Key ID (Big Endian). */
                std::vector<uint8_t> vKeyID;
                vKeyID.push_back(nKeyID >> 24 & 0xff);
                vKeyID.push_back(nKeyID >> 16 & 0xff);
                vKeyID.push_back(nKeyID >> 8  & 0xff);
                vKeyID.push_back(nKeyID       & 0xff);

                /* Generate the Secret Phrase */
                std::vector<uint8_t> vSecret(strUsername.begin(), strUsername.end());
                vSecret.insert(vSecret.end(), vKeyID.begin(), vKeyID.end());
                vSecret.insert(vSecret.end(), strPassword.begin(), strPassword.end());

                /* Generate the Pin Data.
                TODO: Look to see if this can be improved. */
                std::vector<uint8_t> vPin(strSecret.begin(), strSecret.end());
                vPin.insert(vPin.end(), vKeyID.begin(), vKeyID.end());

                /* Generate the Hashes */
                uint1024_t hashSecret = LLC::SK1024(vSecret);
                uint1024_t hashPIN    = LLC::SK1024(vPin);

                /* Generate the Final Root Hash. */
                return LLC::SK512(BEGIN(hashSecret), END(hashPIN));
            }
        };
    }
}
