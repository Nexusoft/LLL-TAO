/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_TAO_OPERATION_INCLUDE_COST_H
#define NEXUS_TAO_OPERATION_INCLUDE_COST_H

#include <cstdint>

/* Global TAO namespace. */
namespace TAO
{
    /* Operation Layer namespace. */
    namespace Operation
    {

        /* Forward declarations. */
        class Contract;


        /** Cost
         *
         *  Run the contract for costs only.
         *
         *  @param[in] contract The contract to get costs for.
         *  @param[out] nCost The total cost for this contract.
         *
         **/
        void Cost(const Contract& contract, uint64_t &nCost);


        /** TxCost
         *
         *  Calculates the transaction cost for including this contract in a transaction.  This method gives us the ability to
         *  modulate the transaction cost depending on the contract type, for example to have no costs for credit or claim contracts.  
         *
         *  @param[in] contract The contract to get the transaction cost for.
         *  @param[out] nCost The transaction cost for this contract.
         *
         **/
        void TxCost(const Contract& contract, uint64_t &nCost);

    }
}

#endif
