/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/include/argon2.h>

#include <Util/include/debug.h>

namespace LLC
{
    

    /* 256-bit hashing function */
    uint256_t Argon2_256(const std::vector<uint8_t>& vchData, 
                        const std::vector<uint8_t>& vchSalt, 
                        const std::vector<uint8_t>& vchSecret,
                        uint32_t nCost, uint32_t nMemory)
    {
        /* The return value hash */
        std::vector<uint8_t> vHash(32);

        /* Create the hash context. */
        argon2_context context =
        {
            /* Hash Return Value. */
            &vHash[0],
            32,

            /* The secret . */
            &(const_cast<std::vector<uint8_t>&>(vchData))[0],
            static_cast<uint32_t>(vchData.size()),

            /* The salt */
            &(const_cast<std::vector<uint8_t>&>(vchSalt))[0],
            static_cast<uint32_t>(vchSalt.size()),

            /* Optional secret data */
            &(const_cast<std::vector<uint8_t>&>(vchSecret))[0],
            static_cast<uint32_t>(vchSecret.size()),

            /* Optional associated data */
            NULL, 0,

            /* Computational Cost. */
            nCost,

            /* Memory Cost (64 MB). */
            nMemory,

            /* The number of threads and lanes */
            1, 1,

            /* Algorithm Version */
            ARGON2_VERSION_13,

            /* Custom memory allocation / deallocation functions. */
            NULL, NULL,

            /* By default only internal memory is cleared (pwd is not wiped) */
            ARGON2_DEFAULT_FLAGS
        };

        /* Run the argon2 computation. */
        int32_t nRet = argon2id_ctx(&context);
        if(nRet != ARGON2_OK)
            throw std::runtime_error(debug::safe_printstr(FUNCTION, "Argon2 failed with code ", nRet));
        
        uint256_t hashData;
        hashData.SetBytes(vHash);

        return hashData;
    }


    /* 256-bit hashing function */
    uint256_t Argon2Fast_256(const std::vector<uint8_t>& vchData, 
                        const std::vector<uint8_t>& vchSalt, 
                        const std::vector<uint8_t>& vchSecret)
    {
        return Argon2_256(vchData, vchSalt, vchSecret, 2, (1 << 4));
    }


    /* 512-bit hashing function */
    uint512_t Argon2_512(const std::vector<uint8_t>& vchData, 
                        const std::vector<uint8_t>& vchSalt, 
                        const std::vector<uint8_t>& vchSecret,
                        uint32_t nCost, uint32_t nMemory)
    {
        /* The return value hash */
        std::vector<uint8_t> vHash(64);

        /* Create the hash context. */
        argon2_context context =
        {
            /* Hash Return Value. */
            &vHash[0],
            64,

            /* The secret . */
            &(const_cast<std::vector<uint8_t>&>(vchData))[0],
            static_cast<uint32_t>(vchData.size()),

            /* The salt */
            &(const_cast<std::vector<uint8_t>&>(vchSalt))[0],
            static_cast<uint32_t>(vchSalt.size()),

            /* Optional secret data */
            &(const_cast<std::vector<uint8_t>&>(vchSecret))[0],
            static_cast<uint32_t>(vchSecret.size()),

            /* Optional associated data */
            NULL, 0,

            /* Computational Cost. */
            nCost,

            /* Memory Cost (64 MB). */
            nMemory,

            /* The number of threads and lanes */
            1, 1,

            /* Algorithm Version */
            ARGON2_VERSION_13,

            /* Custom memory allocation / deallocation functions. */
            NULL, NULL,

            /* By default only internal memory is cleared (pwd is not wiped) */
            ARGON2_DEFAULT_FLAGS
        };

        /* Run the argon2 computation. */
        int32_t nRet = argon2id_ctx(&context);
        if(nRet != ARGON2_OK)
            throw std::runtime_error(debug::safe_printstr(FUNCTION, "Argon2 failed with code ", nRet));
        
        uint512_t hashData;
        hashData.SetBytes(vHash);

        return hashData;
    }


    /* 512-bit hashing function */
    uint512_t Argon2Fast_512(const std::vector<uint8_t>& vchData, 
                        const std::vector<uint8_t>& vchSalt, 
                        const std::vector<uint8_t>& vchSecret)
    {
        return Argon2_512(vchData, vchSalt, vchSecret, 2, (1 << 4));
    }
}
