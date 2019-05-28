/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_TAO_OPERATION_INCLUDE_CLAIM_H
#define NEXUS_TAO_OPERATION_INCLUDE_CLAIM_H

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


        /** Claim
         *
         *  Namespace to contain main functions for OP::CLAIM
         *
         **/
        namespace Claim
        {

            /** Commit
             *
             *  Commit the final state to disk.
             *
             *  @param[in] state The state to commit.
             *  @param[in] hashAddress The register address to commit.
             *  @param[in] hashTx The transaction-id being claimed.
             *  @param[in] nContract The contract output being claimed.
             *  @param[in] nFlags Flags to the LLD instance.
             *
             *  @return true if successful.
             *
             **/
            bool Commit(const TAO::Register::State& state,
                const uint256_t& hashAddress, const uint512_t& hashTx, const uint32_t nContract, const uint8_t nFlags);


            /** Execute
             *
             *  Claims a register between sigchains.
             *
             *  @param[out] state The state register to operate on.
             *  @param[in] hashClaim The public-id being claimed to
             *  @param[in] nTimestamp The timestamp to update register to.
             *
             *  @return true if successful.
             *
             **/
            bool Execute(TAO::Register::State &state, const uint256_t& hashClaim, const uint64_t nTimestamp);


            /** Verify
             *
             *  Verify claim validation rules and caller.
             *
             *  @param[in] claim The 'transferred' contract to verify.
             *  @param[in] hashCaller The contract caller.
             *
             *  @return true if successful.
             *
             **/
            bool Verify(const Contract& claim, const uint256_t& hashCaller);
        }
    }
}

#endif
