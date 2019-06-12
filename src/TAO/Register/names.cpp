/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/hash/SK.h>

#include <LLD/include/global.h>

#include <TAO/Register/include/names.h>

#include <Util/include/debug.h>

#include <vector>
#include <LLC/hash/argon2.h>


/* Global TAO namespace. */
namespace TAO
{

    /* Register Layer namespace. */
    namespace Register
    {

        

        /* Retrieve the address of the name register for a namespace/name combination. */
        void GetNameAddress(const uint256_t& hashNamespace, const std::string& strName, uint256_t& hashAddress)
        {
            /* Build vector to hold the namespace + name data for hashing */
            std::vector<uint8_t> vData;

            /* Insert the namespace hash */
            vData.insert(vData.end(), (uint8_t*)&hashNamespace, (uint8_t*)&hashNamespace + 32);

            /* Insert the name */
            vData.insert(vData.end(), strName.begin(), strName.end());

            /* Build the name register address from the SK256 hash of namespace + name. */
            hashAddress = LLC::SK256(vData);

        }


        /* Retrieve the name register for a namespace/name combination. */
        bool GetNameRegister(const uint256_t& hashNamespace, const std::string& strName, Object& nameRegister)
        {
            uint256_t hashAddress;

            GetNameAddress(hashNamespace, strName, hashAddress);

            /* Read the Name Object */
            if(!LLD::Register->ReadState(hashAddress, nameRegister, TAO::Ledger::FLAGS::MEMPOOL))
            {
                debug::log(2, FUNCTION, "Name register not found: ", strName);
                return false;
            }

            /* Check that the name object is proper type. */
            if(nameRegister.nType != TAO::Register::REGISTER::OBJECT)
                return debug::error( FUNCTION, "Name register not an object: ", strName);

            /* Parse the object. */
            if(!nameRegister.Parse())
                return debug::error(FUNCTION, "Unable to parse name register: ", strName);

            /* Check that this is a Name register */
            if(nameRegister.Standard() != TAO::Register::OBJECTS::NAME)
                return debug::error(FUNCTION, "Register is not a name register: ", strName);

            return true;
        }


        /* Retrieve the namespace register for a namespace/name combination. */
        bool GetNamespaceRegister(const std::string& strNamespace, Object& namespaceRegister)
        {
            uint256_t hashAddress = TAO::Register::NamespaceHash(strNamespace);

            /* Read the Name Object */
            if(!LLD::Register->ReadState(hashAddress, namespaceRegister, TAO::Ledger::FLAGS::MEMPOOL))
            {
                debug::log(2, FUNCTION, "Name register not found: ", strNamespace);
                return false;
            }

            /* Check that the name object is proper type. */
            if(namespaceRegister.nType != TAO::Register::REGISTER::OBJECT)
                return debug::error( FUNCTION, "Name register not an object: ", strNamespace);

            /* Parse the object. */
            if(!namespaceRegister.Parse())
                return debug::error(FUNCTION, "Unable to parse namespace register: ", strNamespace);

            /* Check that this is a Name register */
            if(namespaceRegister.Standard() != TAO::Register::OBJECTS::NAMESPACE)
                return debug::error(FUNCTION, "Register is not a namespace register: ", strNamespace);

            return true;
        }


        /* Generates a lightweight argon2 hash of the namespace string.*/
        uint256_t NamespaceHash(const std::string& strNamespace)
        {
            /* Generate the Secret Phrase */
            std::vector<uint8_t> vNamespace(strNamespace.begin(), strNamespace.end());

            // low-level API
            std::vector<uint8_t> vHash(32);
            std::vector<uint8_t> vSalt(16);

            /* Create the hash context. */
            argon2_context context =
            {
                /* Hash Return Value. */
                &vHash[0],
                32,

                /* Password input data. */
                &vNamespace[0],
                static_cast<uint32_t>(vNamespace.size()),

                /* The salt for usernames */
                &vSalt[0],
                static_cast<uint32_t>(vSalt.size()),

                /* Optional secret data */
                NULL, 0,

                /* Optional associated data */
                NULL, 0,

                /* Computational Cost. */
                10,

                /* Memory Cost (4 MB). */
                (1 << 12),

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

            /* Set the bytes for the key. */
            uint256_t hashKey;
            hashKey.SetBytes(vHash);

            return hashKey;
        }

    }
}
