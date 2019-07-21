/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

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
             *  @param[in] state The state to commit.
             *  @param[in] hashAddress The register address to commit.
             *  @param[in] hashTx The transaction that is calling coinbase
             *  @param[in] nFlags Flags to the LLD instance.
             *
             *  @return true if successful.
             *
             **/
            bool Commit(const uint256_t& hashAddress, const uint512_t& hashTx, const uint8_t nFlags);


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
