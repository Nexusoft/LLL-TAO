/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2019

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once

#include <vector>
#include <LLC/types/base_uint.h>

namespace LLC
{

     /** Encrypt
     *
     *  Encrypts the data using the AES 256 function and the specified symmetric key. 
     *  .       
     *
     *  @param[in] vchKey The symmetric key to use for the encryption.
     *  @param[in] vchPlainText The plain text data bytes to be encrypted.
     *  @param[out] vchEncrypted  The encrypted data.
     *
     *  @return True if the data was encrypted successfully.
     *
     **/
    bool EncryptAES256(const std::vector<uint8_t>& vchKey, const std::vector<uint8_t>& vchPlainText, std::vector<uint8_t> &vchEncrypted);


    /** Decrypt
     *
     *  Decrypts the data using the AES 256 function and the specified symmetric key.       
     *
     *  @param[in] vchKey The symmetric key to use for the encryption.
     *  @param[in] vchEncrypted The encrypted data to be decrypted.
     *  @param[out] vchPlainText The decrypted plain text data.
     *
     *  @return True if the data was decrypted successfully.
     *
     **/
    bool DecryptAES256(const std::vector<uint8_t>& vchKey, const std::vector<uint8_t>& vchEncrypted, std::vector<uint8_t> &vchPlainText);
        


}
