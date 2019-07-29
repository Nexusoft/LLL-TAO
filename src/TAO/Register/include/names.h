/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_TAO_REGISTER_INCLUDE_UTILS_H
#define NEXUS_TAO_REGISTER_INCLUDE_UTILS_H

#include <LLC/types/uint1024.h>

#include <TAO/Register/types/object.h>

#include <string>


/* Global TAO namespace. */
namespace TAO
{

    /* Register Layer namespace. */
    namespace Register
    {
        class Address;

        /** GetNameAddress
         *
         *  Retrieve the address of the name register for a namespace/name combination.
         *
         *  @param[in] hashNamespace
         *  @param[in] strName
         *  @param[out] hashAddress the name register address
         *
         **/
        void GetNameAddress(const uint256_t& hashNamespace, const std::string& strName, uint256_t& hashAddress);


        /** GetNameRegister
         *
         *  Retrieve the name register for a namespace/name combination.
         *
         *  @param[in] hashNamespace
         *  @param[in] strName
         *  @param[out] nameRegister
         *
         *  @return true if register retrieved successfully
         *
         **/
        bool GetNameRegister(const uint256_t& hashNamespace, const std::string& strName, Object& nameRegister);


        /** GetNamespaceRegister
         *
         *  Retrieve the namespace register by namespace name.
         *
         *  @param[in] strName
         *  @param[out] nameRegister
         *
         *  @return true if register retrieved successfully
         *
         **/
        bool GetNamespaceRegister(const std::string& strNamespace, Object& namespaceRegister);


    }
}

#endif
