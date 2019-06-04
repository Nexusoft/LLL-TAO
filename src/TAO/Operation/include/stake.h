/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_TAO_OPERATION_INCLUDE_STAKE_H
#define NEXUS_TAO_OPERATION_INCLUDE_STAKE_H

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
         *  Namespace to contain main functions for OP::WRITE
         *
         **/
        namespace Stake
        {

            /** Commit
             *
             *  Commit the final state to disk.
             *
             *  @param[in] state The state to commit.
             *  @param[in] nFlags Flags to the LLD instance.
             *
             *  @return true if successful.
             *
             **/
            bool Commit(const TAO::Register::State& state, const uint8_t nFlags);


            /** Execute
             *
             *  Move balance to stake for trust account.
             *
             *  @param[out] object The trust account object register.
             *  @param[in] nAmount The amount of balance to stake.
             *  @param[in] nTimestamp The timestamp to update register to.
             *
             *  @return true if successful.
             *
             **/
            bool Execute(TAO::Register::Object &object, const uint64_t nAmount, const uint64_t nTimestamp);


            /** Verify
             *
             *  Verify trust validation rules and caller.
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
