/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_TAO_OPERATION_INCLUDE_TRANSFER_H
#define NEXUS_TAO_OPERATION_INCLUDE_TRANSFER_H

#include <LLC/types/uint1024.h>

/* Global TAO namespace. */
namespace TAO
{

    /* Register layer. */
    namespace Register
    {
        /* Forward declarations. */
        class State;
    }


    /* Operation Layer namespace. */
    namespace Operation
    {

        /* Forward declarations. */
        class Contract;


        /** Transfer
         *
         *  Namespace to contain main functions for OP::TRANSFER
         *
         **/
        namespace Transfer
        {

            /** Commit
             *
             *  Commit the final state to disk.
             *
             *  @param[in] state The state to commit.
             *  @param[in] hashTx The tx-id of calling transaction.
             *  @param[in] hashAddress The register address to commit.
             *  @param[in] hashTransfer The new user to be transferred to..
             *  @param[in] nFlags Flags to the LLD instance.
             *
             *  @return true if successful.
             *
             **/
            bool Commit(const TAO::Register::State& state, const uint512_t& hashTx,
                        const uint256_t& hashAddress, const uint256_t& hashTransfer, const uint8_t nFlags);


            /** Execute
             *
             *  Transfers a register between sigchains.
             *
             *  @param[out] state The state register to operate on.
             *  @param[in] hashTransfer The object to transfer
             *  @param[in] nTimestamp The timestamp to update register to.
             *
             *  @return true if successful.
             *
             **/
            bool Execute(TAO::Register::State &state, const uint256_t& hashTransfer, const uint64_t nTimestamp);


            /** Verify
             *
             *  Verify transfer validation rules and caller.
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
