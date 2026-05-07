/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_TAO_OPERATION_INCLUDE_COINBASE_H
#define NEXUS_TAO_OPERATION_INCLUDE_COINBASE_H

#include <LLC/types/uint1024.h>

/* Global TAO namespace. */
namespace TAO
{


    /* Operation Layer namespace. */
    namespace Operation
    {

        /* Forward declarations. */
        class Contract;


        /** Append
         *
         *  Namespace to contain main functions for OP::APPEND
         *
         **/
        namespace Coinbase
        {

            /** Commit
             *
             *  Commit the final state to disk.
             *
             *  @param[in] hashGenesis The genesis hash (event recipient).
             *  @param[in] nAmount The coinbase reward amount.
             *  @param[in] hashTx The transaction that is calling coinbase
             *  @param[in] nFlags Flags to the LLD instance.
             *  @param[in] hashAccount Optional account register to credit directly.
             *
             *  @return true if successful.
             *
             **/
            bool Commit(const uint256_t& hashGenesis, const uint64_t nAmount, const uint512_t& hashTx,
                        const uint8_t nFlags, const uint256_t& hashAccount = uint256_t(0));


            /** HasAutoCreditAccount
             *
             *  Check whether a coinbase operation uses the extended payload:
             *  OP, recipient genesis, recipient account, amount, extra nonce.
             *
             *  @param[in] contract The contract to inspect.
             *
             *  @return true if the operation contains a recipient account.
             *
             **/
            bool HasAutoCreditAccount(const Contract& contract);


            /** Verify
             *
             *  Verify append validation rules and caller.
             *
             *  @param[in] contract The contract to verify.
             *
             *  @return true if successful.
             *
             **/
            bool Verify(const Contract& contract);
        }
    }
}

#endif
