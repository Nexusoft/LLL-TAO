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
            if(!LLD::regDB->ReadState(hashAddress, nameRegister, TAO::Ledger::FLAGS::MEMPOOL))
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

    }
}
