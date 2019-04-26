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
         *  @param[in] nIdentifier The type of token this account supports.
         *
         *  @return The object register just created.
         *
         **/
        Object CreateAccount(const uint32_t nIdentifier);


        /** CreateToken
         *
         *  Generate a new account object register.
         *
         *  @param[in] nIdentifier The type of token this is
         *  @param[in] nSupply The total supply for token
         *  @param[in] nDigits The total significant figures
         *
         *  @return The object register just created.
         *
         **/
        Object CreateToken(const uint32_t nIdentifier, const uint64_t nSupply, const uint64_t nDigits);

    }
}

#endif
