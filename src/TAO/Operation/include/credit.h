/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_TAO_OPERATION_INCLUDE_CREDIT_H
#define NEXUS_TAO_OPERATION_INCLUDE_CREDIT_H

#include <LLC/types/uint1024.h>

/* Global TAO namespace. */
namespace TAO
{

    /* Register layer. */
    namespace Register
    {
        /* Forward declarations. */
        class Object;
    }


    /* Operation Layer namespace. */
    namespace Operation
    {

        /* Forward declarations. */
        class Contract;


        /** Credit
         *
         *  Namespace to contain main functions for OP::CREDIT
         *
         **/
        namespace Credit
        {

            /** Commit
             *
             *  Commit the final state to disk.
             *
             *  @param[in] account The account to commit.
             *  @param[in] hashProof The proof address to commit.
             *  @param[in] hashAddress The register address to commit.
             *  @param[in] hashTx The transaction-id being claimed.
             *  @param[in] nContract The contract output being claimed.
             *  @param[in] nAmount The amount that is being credited.
             *  @param[in] nFlags Flags to the LLD instance.
             *
             *  @return true if successful.
             *
             **/
            bool Commit(const TAO::Register::Object& account, const uint256_t& hashAddress, const uint256_t& hashProof,
                const uint512_t& hashTx, const uint32_t nContract, const uint64_t nAmount, const uint8_t nFlags);


            /** Execute
             *
             *  Commits funds from an account to an account
             *
             *  @param[out] account The object register to credit to.
             *  @param[in] nAmount The amount to credit to object.
             *  @param[in] nTimestamp The timestamp to update register to.
             *
             *  @return true if successful.
             *
             **/
            bool Execute(TAO::Register::Object &account, const uint64_t nAmount, const uint64_t nTimestamp);


            /** Verify
             *
             *  Verify claim validation rules and caller.
             *
             *  @param[in] debit The contract to claim.
             *  @param[in] credit The contract to verify.
             *
             *  @return true if successful.
             *
             **/
            bool Verify(const Contract& debit, const Contract& credit);
        }
    }
}

#endif
