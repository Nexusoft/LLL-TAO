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

#include <TAO/Ledger/types/transaction.h>

/* Global TAO namespace. */
namespace TAO
{

    /* Register Layer namespace. */
    namespace Register
    {

        /** Calculate
         *
         *  Calculate the pre-states and post-state checksums for a contract.
         *
         *  @param[in] contract The contract to calculate for
         *
         *  @return true if verified correctly, false otherwise.
         *
         **/
        bool Calculate(Contract& contract);

    }
}

#endif
