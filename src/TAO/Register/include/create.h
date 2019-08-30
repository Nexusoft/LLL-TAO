/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_TAO_REGISTER_INCLUDE_CREATE_H
#define NEXUS_TAO_REGISTER_INCLUDE_CREATE_H

#include <TAO/Register/types/object.h>

/* Global TAO namespace. */
namespace TAO
{

    /* Register Layer namespace. */
    namespace Register
    {

        /** CreateAccount
         *
         *  Generate a new account object register.
         *
         *  @param[in] hashIdentifier The type of token this account supports.
         *
         *  @return The object register just created.
         *
         **/
        Object CreateAccount(const uint256_t& hashIdentifier);


        /** CreateTrust
         *
         *  Generate a new trust object register.
         *
         *  @return The object register just created.
         *
         **/
        Object CreateTrust();


        /** CreateToken
         *
         *  Generate a new account object register.
         *
         *  @param[in] hashIdentifier The type of token this is
         *  @param[in] nSupply The total supply for token
         *  @param[in] nDecimals The total significant figures
         *
         *  @return The object register just created.
         *
         **/
        Object CreateToken(const uint256_t& hashIdentifier, const uint64_t nSupply, const uint64_t nDecimals);


        /** CreateAsset
         *
         *  Generate a new asset object register.
         *
         *  @return The object register just created.
         *
         **/
        Object CreateAsset();


        /** CreateName
         *
         *  Generate a new name object register.
         *
         *  @param[in] strNamespace The namespace that the object should be crated in
         *  @param[in] strName The name of the Object that this Name record represents
         *  @param[in] hashRegister The register address that this name is aliasing
         *
         *  @return The object register just created.
         *
         **/
        Object CreateName(const std::string& strNamespace, const std::string& strName, const uint256_t& hashRegister);



        /** CreateNamespace
         *
         *  Generate a new namespace object register.
         *
         *  @param[in] strName The name of the namespace Object
         *  @param[in] hashRegister The register address that this name is aliasing
         *
         *  @return The object register just created.
         *
         **/
        Object CreateNamespace(const std::string& strName);


    }
}

#endif
