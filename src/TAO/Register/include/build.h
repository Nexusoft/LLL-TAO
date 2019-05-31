/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_TAO_REGISTER_INCLUDE_CALCULATE_H
#define NEXUS_TAO_REGISTER_INCLUDE_CALCULATE_H

#include <LLC/types/uint1024.h>

#include <map>

/* Global TAO namespace. */
namespace TAO
{
    /* Operation layer. */
    namespace Operation
    {
        /* Forward declarations. */
        class Contract;
    }


    /* Register Layer namespace. */
    namespace Register
    {
        /* Forward declarations. */
        class State;


        /** Build
         *
         *  Build the pre-states and post-state checksums for a contract.
         *
         *  @param[out] contract The contract to calculate for
         *  @param[out] mapStates The temporary states if pre-states rely on previous contracts.
         *
         *  @return true if verified correctly, false otherwise.
         *
         **/
        bool Build(TAO::Operation::Contract& contract, std::map<uint256_t, State>& mapStates);

    }
}

#endif
