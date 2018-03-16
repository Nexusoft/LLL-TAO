/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2017] ++
            
            (c) Copyright The Nexus Developers 2014 - 2017
            
            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.
            
            "fides in stellis, virtus in numeris" - Faith in the Stars, Power in Numbers

____________________________________________________________________________________________*/

#include <string>

#include "../../../LLC/hash/SK.h"
#include "../../../LLC/hash/macro.h"

namespace Core
{

    /** Base Class for a Signature SignatureChain
    * 
    *  Similar to HD wallet systems, but integrated into the Layer 1 of the network.
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
        uint512 Generate(unsigned int nKeyID, std::string strSecret)
        {
            /* Serialize the Key ID (Big Endian). */
            std::vector<unsigned char> vKeyID;
            vKeyID.push_back(nKeyID >> 24);
            vKeyID.push_back(nKeyID >> 16);
            vKeyID.push_back(nKeyID >> 8 );
            vKeyID.push_back(nKeyID      );
    
            /* Generate the Secret Phrase */
            std::vector<unsigned char> vSecret(strUsername.begin(), strUsername.end());
            vSecret.insert(vSecret.end(), vKeyID.begin(), vKeyID.end());
            vSecret.insert(vSecret.end(), strPassword.begin(), strPassword.end());
            
            /* Generate the Pin Data. 
            TODO: Look to see if this can be improved. */
            std::vector<unsigned char> vPin(strSecret.begin(), strSecret.end());
            vPin.insert(vPin.end(), vKeyID.begin(), vKeyID.end());
            
            /* Generate the Hashes */
            uint1024 hashSecret = LLC::HASH::SK1024(vSecret);
            uint1024 hashPIN    = LLC::HASH::SK1024(vPin);
            
            /* Generate the Final Root Hash. */
            return LLC::HASH::SK512(BEGIN(hashSecret), END(hashPIN));
        }
    };

}
