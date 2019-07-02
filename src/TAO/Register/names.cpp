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
#include <TAO/Register/types/address.h>

#include <Util/include/debug.h>

#include <vector>


/* Global TAO namespace. */
namespace TAO
{

    /* Register Layer namespace. */
    namespace Register
    {



        /* Retrieve the address of the name register for a namespace/name combination. */
        void GetNameAddress(const uint256_t& hashNamespace, const std::string& strName, uint256_t& address)
        {
            /* Build vector to hold the namespace + name data for hashing */
            std::vector<uint8_t> vData;

            /* Insert the namespace hash */
            vData.insert(vData.end(), (uint8_t*)&hashNamespace, (uint8_t*)&hashNamespace + 32);

            /* Insert the name */
            vData.insert(vData.end(), strName.begin(), strName.end());

            /* Build the name register address from the SK256 hash of namespace + name. */
            address = Address(vData, Address::NAME);
        }


        /* Retrieve the name register for a namespace/name combination. */
        bool GetNameRegister(const uint256_t& hashNamespace, const std::string& strName, Object& nameRegister)
        {
            Address hashAddress;

            /* Get the register address for the Name object */
            GetNameAddress(hashNamespace, strName, hashAddress);

            /* Read the Name Object */
            if(!LLD::Register->ReadState(hashAddress, nameRegister, TAO::Ledger::FLAGS::MEMPOOL))
                return debug::error(FUNCTION, "Name register not found: ", strName);

            /* Check that the name object is proper type. */
            if(nameRegister.nType != TAO::Register::REGISTER::OBJECT)
                return debug::error(FUNCTION, "Name register not an object: ", strName);

            /* Parse the object. */
            if(!nameRegister.Parse())
                return debug::error(FUNCTION, "Unable to parse name register: ", strName);

            /* Check that this is a Name register */
            if(nameRegister.Standard() != TAO::Register::OBJECTS::NAME)
                return debug::error(FUNCTION, "Register is not a name register: ", strName);

            return true;
        }


        /* Retrieve the namespace register by namespace name. */
        bool GetNamespaceRegister(const std::string& strNamespace, Object& namespaceRegister)
        {
            /* Namespace hash is a SK256 hash of the namespace name */
            uint256_t hashAddress  = Address(strNamespace, Address::NAMESPACE);

            /* Read the Name Object */
            if(!LLD::Register->ReadState(hashAddress, namespaceRegister, TAO::Ledger::FLAGS::MEMPOOL))
                return debug::error(FUNCTION, "Namespace register not found: ", strNamespace);

            /* Check that the name object is proper type. */
            if(namespaceRegister.nType != TAO::Register::REGISTER::OBJECT)
                return debug::error(FUNCTION, "Namespace register not an object: ", strNamespace);

            /* Parse the object. */
            if(!namespaceRegister.Parse())
                return debug::error(FUNCTION, "Unable to parse namespace register: ", strNamespace);

            /* Check that this is a Name register */
            if(namespaceRegister.Standard() != TAO::Register::OBJECTS::NAMESPACE)
                return debug::error(FUNCTION, "Register is not a namespace register: ", strNamespace);

            return true;
        }
    }
}
