/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_TAO_REGISTER_INCLUDE_CONSTANTS_H
#define NEXUS_TAO_REGISTER_INCLUDE_CONSTANTS_H

/* Global TAO namespace. */
namespace TAO
{

    /* Register Layer namespace. */
    namespace Register
    {

        /** Maximum register sizes. **/
        const uint32_t MAX_REGISTER_SIZE = 1024; //1 KB


        /** Wildcard register address. **/
        const uint256_t WILDCARD_ADDRESS = ~uint256_t(0);

    }
}

#endif
