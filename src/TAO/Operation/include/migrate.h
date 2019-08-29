/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_TAO_OPERATION_INCLUDE_MIGRATE_H
#define NEXUS_TAO_OPERATION_INCLUDE_MIGRATE_H

#include <LLC/types/uint1024.h>

/* Global TAO namespace. */
namespace TAO
{


    /* Register layer. */
    namespace Register
    {
        /* Forward declarations. */
        class State;
        class Object;
    }


    /* Operation Layer namespace. */
    namespace Operation
    {

        /* Forward declarations. */
        class Contract;


        /** Write
         *
         *  Namespace to contain main functions for OP::MIGRATE
         *
         **/
        namespace Migrate
        {

            /** Commit
             *
             *  Commit the final state to disk.
             *
             *  @param[in] account The account to commit.
             *  @param[in] hashAddress The register address to commit.
             *  @param[in] hashCaller The contract caller.
             *  @param[in] hashTx The transaction-id being claimed.
             *  @param[in] hashKey The Legacy trust key hash being migrated from.
             *  @param[in] hashLast The hash last trust for the legacy trust key being migrated.
             *  @param[in] nFlags Flags to the LLD instance.
             *
             *  @return true if successful.
             *
             **/
            bool Commit(const TAO::Register::Object& account, const TAO::Register::Address& hashAddress,
                        const uint256_t& hashCaller, const uint512_t& hashTx, const uint576_t& hashKey,
                        const uint512_t& hashLast, const uint8_t nFlags);


            /** Execute
             *
             *  Migrate trust key data to trust account register.
             *
             *  @param[out] object The trust account object register.
             *  @param[in] nAmount The amount of balance to migrate.
             *  @param[in] nScore The amount of trust to migrate.
             *  @param[in] nTimestamp The timestamp to update register to.
             *
             *  @return true if successful.
             *
             **/
            bool Execute(TAO::Register::Object &object, const uint64_t nAmount,
                         const uint32_t nScore, const uint64_t nTimestamp);


            /** Verify
             *
             *  Verify trust migration rules.
             *
             *  @param[in] contract The contract to verify.
             *  @param[in] debit The contract to claim.
             *
             *  @return true if successful.
             *
             **/
            bool Verify(const Contract& contract, const Contract& debit);
        }
    }
}

#endif
