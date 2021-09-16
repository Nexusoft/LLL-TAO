/**
 * @file ssl_misc.c
 * @brief SSL 3.0 helper functions
 *
 * @section License
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Copyright (C) 2010-2019 Oryx Embedded SARL. All rights reserved.
 *
 * This file is part of CycloneSSL Open.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * @author Oryx Embedded SARL (www.oryx-embedded.com)
 * @version 1.9.6
 **/

//Switch to the appropriate trace level
#define TRACE_LEVEL TLS_TRACE_LEVEL

//Dependencies
#include <string.h>
#include "core/crypto.h"
#include "tls.h"
#include "ssl_misc.h"
#include "debug.h"

//Check TLS library configuration
#if (TLS_SUPPORT == ENABLED && TLS_MIN_VERSION <= SSL_VERSION_3_0)

//pad1 pattern
const uint8_t sslPad1[48] =
{
   0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36,
   0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36,
   0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36
};

//pad2 pattern
const uint8_t sslPad2[48] =
{
   0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C,
   0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C,
   0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C
};


/**
 * @brief Key expansion function (SSL 3.0)
 * @param[in] secret Pointer to the secret
 * @param[in] secretLen Length of the secret
 * @param[in] random Pointer to the random bytes
 * @param[in] randomLen Length of the random bytes
 * @param[out] output Pointer to the output
 * @param[in] outputLen Desired output length
 * @return Error code
 **/

error_t sslExpandKey(const uint8_t *secret, size_t secretLen,
   const uint8_t *random, size_t randomLen, uint8_t *output, size_t outputLen)
{
   uint_t i;
   size_t n;
   char_t pad[16];
   Md5Context *md5Context;
   Sha1Context *sha1Context;

   //Output length cannot exceed 256 bytes
   if(outputLen > (sizeof(pad) * MD5_DIGEST_SIZE))
      return ERROR_INVALID_LENGTH;

   //Allocate a memory buffer to hold the MD5 context
   md5Context = tlsAllocMem(sizeof(Md5Context));
   //Failed to allocate memory?
   if(md5Context == NULL)
   {
      //Report an error
      return ERROR_OUT_OF_MEMORY;
   }

   //Allocate a memory buffer to hold the SHA-1 context
   sha1Context = tlsAllocMem(sizeof(Sha1Context));
   //Failed to allocate memory?
   if(sha1Context == NULL)
   {
      //Clean up side effects
      tlsFreeMem(md5Context);
      //Report an error
      return ERROR_OUT_OF_MEMORY;
   }

   //Loop until enough output has been generated
   for(i = 0; outputLen > 0; i++)
   {
      //Generate pad
      memset(pad, 'A' + i, i + 1);

      //Compute SHA(pad + secret + random)
      sha1Init(sha1Context);
      sha1Update(sha1Context, pad, i + 1);
      sha1Update(sha1Context, secret, secretLen);
      sha1Update(sha1Context, random, randomLen);
      sha1Final(sha1Context, NULL);

      //Then compute MD5(secret + SHA(pad + secret + random))
      md5Init(md5Context);
      md5Update(md5Context, secret, secretLen);
      md5Update(md5Context, sha1Context->digest, SHA1_DIGEST_SIZE);
      md5Final(md5Context, NULL);

      //Calculate the number of bytes to copy
      n = MIN(outputLen, MD5_DIGEST_SIZE);
      //Copy the resulting hash value
      memcpy(output, md5Context->digest, n);

      //Advance data pointer
      output += n;
      //Decrement byte counter
      outputLen -= n;
   }

   //Release previously allocated resources
   tlsFreeMem(md5Context);
   tlsFreeMem(sha1Context);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Compute message authentication code (SSL 3.0)
 * @param[in] encryptionEngine Pointer to the encryption/decryption engine
 * @param[in] record Pointer to the TLS record
 * @param[in] data Pointer to the record data
 * @param[in] dataLen Length of the data
 * @param[out] mac The computed MAC value
 * @return Error code
 **/

error_t sslComputeMac(TlsEncryptionEngine *encryptionEngine,
   const TlsRecord *record, const uint8_t *data, size_t dataLen, uint8_t *mac)
{
   size_t padLen;
   const HashAlgo *hashAlgo;
   HashContext *hashContext;

   //Point to the hash algorithm to be used
   hashAlgo = encryptionEngine->hashAlgo;
   //Point to the hash context
   hashContext = (HashContext *) encryptionEngine->hmacContext->hashContext;

   //The length of pad1 and pad2 depends on the hash algorithm
   if(hashAlgo == MD5_HASH_ALGO)
   {
      //48-byte long patterns are used with MD5
      padLen = 48;
   }
   else if(hashAlgo == SHA1_HASH_ALGO)
   {
      //40-byte long patterns are used with SHA-1
      padLen = 40;
   }
   else
   {
      //SSL 3.0 supports only MD5 and SHA-1 hash functions
      return ERROR_INVALID_PARAMETER;
   }

   //Compute hash(secret + pad1 + seqNum + type + length + data)
   hashAlgo->init(hashContext);
   hashAlgo->update(hashContext, encryptionEngine->macKey, encryptionEngine->macKeyLen);
   hashAlgo->update(hashContext, sslPad1, padLen);
   hashAlgo->update(hashContext, &encryptionEngine->seqNum, sizeof(TlsSequenceNumber));
   hashAlgo->update(hashContext, &record->type, sizeof(record->type));
   hashAlgo->update(hashContext, (void *) &record->length, sizeof(record->length));
   hashAlgo->update(hashContext, data, dataLen);
   hashAlgo->final(hashContext, mac);

   //Then compute hash(secret + pad2 + hash(secret + pad1 + seqNum + type + length + data))
   hashAlgo->init(hashContext);
   hashAlgo->update(hashContext, encryptionEngine->macKey, encryptionEngine->macKeyLen);
   hashAlgo->update(hashContext, sslPad2, padLen);
   hashAlgo->update(hashContext, mac, hashAlgo->digestSize);
   hashAlgo->final(hashContext, mac);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief CBC padding verification (constant time)
 * @param[in] decryptionEngine Pointer to the decryption engine
 * @param[in] data Pointer to the record payload
 * @param[in] dataLen Length of the payload
 * @param[out] paddingLen Length of the padding string
 * @return The function returns 0 if the padding is correct, 1 on failure
 **/

uint32_t sslVerifyPadding(TlsEncryptionEngine *decryptionEngine,
   const uint8_t *data, size_t dataLen, size_t *paddingLen)
{
   size_t i;
   size_t n;
   uint8_t b;
   uint8_t mask;
   uint32_t c;
   uint32_t bad;
   uint32_t bad1;
   uint32_t bad2;

   //Retrieve the length of the padding string
   n = data[dataLen - 1];

   //Make sure the padding length is valid
   bad = CRYPTO_TEST_GTE_32(n, dataLen);

   //The length of the padding must be less than the cipher's block
   //length (refer to RFC 6101, section 5.2.3.2)
   bad |= CRYPTO_TEST_GTE_32(n, decryptionEngine->cipherAlgo->blockSize);

   //Check the first acceptable padding scheme (each byte in the padding
   //data must be filled with the padding length value)
   for(bad1 = 0, i = 1; i < dataLen && i < 256; i++)
   {
      //Read current byte
      b = data[dataLen - 1 - i];

      //Verify that the padding string is correct
      c = CRYPTO_TEST_LTE_32(i, n);
      mask = CRYPTO_SELECT_8(b, n, c);
      bad1 |= CRYPTO_TEST_NEQ_8(b, mask);
   }

   //Check the second acceptable padding scheme (each byte in the padding
   //data must be filled with zero)
   for(bad2 = 0, i = 1; i < dataLen && i < 256; i++)
   {
      //Read current byte
      b = data[dataLen - 1 - i];

      //Verify that the padding string is correct
      c = CRYPTO_TEST_LTE_32(i, n);
      mask = CRYPTO_SELECT_8(b, 0, c);
      bad2 |= CRYPTO_TEST_NEQ_8(b, mask);
   }

   //Verify that the padding bytes are correct
   bad |= bad1 & bad2;

   //Save the length of the padding string
   *paddingLen = CRYPTO_SELECT_32(n, 0, bad);

   //Return status code
   return bad;
}


/**
 * @brief MAC verification (constant time)
 *
 * Calculate and verify the MAC in constant time without leaking information
 * about what the make-up of the plaintext blocks is in terms of message, MAC
 * field and padding, and whether the format is valid (Adam Langley's method)
 *
 * @param[in] decryptionEngine Pointer to the decryption engine
 * @param[in] data Pointer to the record payload
 * @param[in] dataLen Actual length of the plaintext data (secret information)
 * @param[in] maxDataLen Maximum possible length of the plaintext data
 * @param[in] mac Message authentication code
 * @return The function returns 0 if the MAC verification is successful, else 1
 **/

uint32_t sslVerifyMac(TlsEncryptionEngine *decryptionEngine,
   const TlsRecord *record, const uint8_t *data, size_t dataLen,
   size_t maxDataLen, const uint8_t *mac)
{
   size_t i;
   size_t j;
   size_t n;
   size_t padLen;
   size_t headerLen;
   size_t paddingLen;
   size_t blockSizeMask;
   uint8_t b;
   uint32_t c;
   uint64_t bitLen;
   const HashAlgo *hashAlgo;
   HashContext *hashContext;
   uint8_t temp1[SHA1_DIGEST_SIZE];
   uint8_t temp2[SHA1_DIGEST_SIZE];

   //Point to the hash algorithm to be used
   hashAlgo = decryptionEngine->hashAlgo;
   //Point to the hash context
   hashContext = (HashContext *) decryptionEngine->hmacContext->hashContext;

   //The length of pad1 and pad2 depends on the hash algorithm
   if(hashAlgo == MD5_HASH_ALGO)
   {
      //48-byte long patterns are used with MD5
      padLen = 48;
   }
   else if(hashAlgo == SHA1_HASH_ALGO)
   {
      //40-byte long patterns are used with SHA-1
      padLen = 40;
   }
   else
   {
      //SSL 3.0 supports only MD5 and SHA-1 hash functions
      return 1;
   }

   //The size of the block depends on the hash algorithm
   blockSizeMask = hashAlgo->blockSize - 1;

   //Calculate the length of the additional data that will be hashed in
   //prior to the application data
   headerLen = decryptionEngine->macKeyLen + padLen + 11;

   //Calculate the length of the padding string
   paddingLen = (headerLen + dataLen + hashAlgo->minPadSize - 1) & blockSizeMask;
   paddingLen = hashAlgo->blockSize - paddingLen;

   //Length of the message, in bits
   bitLen = (headerLen + dataLen) << 3;

   //Check endianness
   if(hashAlgo->bigEndian)
   {
      //Encode the length field as a big-endian integer
      bitLen = swapInt64(bitLen);
   }

   //Total number of bytes to process
   n = headerLen + maxDataLen + hashAlgo->minPadSize;
   n = (n + hashAlgo->blockSize - 1) & ~blockSizeMask;
   n -= headerLen;

   //Compute hash(secret + pad1 + seqNum + type + length + data)
   hashAlgo->init(hashContext);
   hashAlgo->update(hashContext, decryptionEngine->macKey, decryptionEngine->macKeyLen);
   hashAlgo->update(hashContext, sslPad1, padLen);
   hashAlgo->update(hashContext, &decryptionEngine->seqNum, sizeof(TlsSequenceNumber));
   hashAlgo->update(hashContext, &record->type, sizeof(record->type));
   hashAlgo->update(hashContext, (void *) &record->length, sizeof(record->length));

   //Point to the first byte of the plaintext data
   i = 0;

   //We can process the first blocks normally because the (secret) padding
   //length cannot affect them
   if(maxDataLen > 255)
   {
      //Digest the first part of the plaintext data
      hashAlgo->update(hashContext, data, maxDataLen - 255);
      i += maxDataLen - 255;
   }

   //The last blocks need to be handled carefully
   while(i < n)
   {
      //Initialize the value of the current byte
      b = 0;

      //Generate the contents of each block in constant time
      c = CRYPTO_TEST_LT_32(i, dataLen);
      b = CRYPTO_SELECT_8(b, data[i], c);

      c = CRYPTO_TEST_EQ_32(i, dataLen);
      b = CRYPTO_SELECT_8(b, 0x80, c);

      j = dataLen + paddingLen;
      c = CRYPTO_TEST_GTE_32(i, j);
      j += 8;
      c &= CRYPTO_TEST_LT_32(i, j);
      b = CRYPTO_SELECT_8(b, bitLen & 0xFF, c);
      bitLen = CRYPTO_SELECT_64(bitLen, bitLen >> 8, c);

      //Digest the current byte
      hashAlgo->update(hashContext, &b, sizeof(uint8_t));

      //Increment byte counter
      i++;

      //End of block detected?
      if(((i + headerLen) & blockSizeMask) == 0)
      {
         //For each block we serialize the hash
         hashAlgo->finalRaw(hashContext, temp1);

         //Check whether the current block of data is the final block
         c = CRYPTO_TEST_EQ_32(i, dataLen + paddingLen + 8);

         //The hash is copied with a mask so that only the correct hash value
         //is copied out, but the amount of computation remains constant
         for(j = 0; j < hashAlgo->digestSize; j++)
         {
            temp2[j] = CRYPTO_SELECT_8(temp2[j], temp1[j], c);
         }
      }
   }

   //Compute hash(secret + pad2 + hash(secret + pad1 + seqNum + type + length + data))
   hashAlgo->init(hashContext);
   hashAlgo->update(hashContext, decryptionEngine->macKey, decryptionEngine->macKeyLen);
   hashAlgo->update(hashContext, sslPad2, padLen);
   hashAlgo->update(hashContext, temp2, hashAlgo->digestSize);
   hashAlgo->final(hashContext, temp1);

   //Debug message
   TRACE_DEBUG("Read sequence number:\r\n");
   TRACE_DEBUG_ARRAY("  ", &decryptionEngine->seqNum, sizeof(TlsSequenceNumber));
   TRACE_DEBUG("Computed MAC:\r\n");
   TRACE_DEBUG_ARRAY("  ", temp1, hashAlgo->digestSize);

   //The calculated MAC is bitwise compared to the received message
   //authentication code
   for(b = 0, i = 0; i < hashAlgo->digestSize; i++)
   {
      b |= mac[i] ^ temp1[i];
   }

   //Return 0 if the message authentication code is correct, else 1
   return CRYPTO_TEST_NEQ_8(b, 0);
}

#endif
