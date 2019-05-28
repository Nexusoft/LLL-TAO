/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_TAO_OPERATION_INCLUDE_EXECUTE_H
#define NEXUS_TAO_OPERATION_INCLUDE_EXECUTE_H

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


        /** Execute
         *
         *  Executes a given contract
         *
         *  @param[in] contract The contract to execute
         *  @param[in] nFlags The flags to execute with.
         *
         *  @return True if operations executed successfully, false otherwise.
         *
         **/
        bool Execute(const Contract& contract, const uint8_t nFlags);


        /** Execute
         *
         *  Namespace to hold primitive operation execute functions.
         *  These functions ONLY update the state of the register.
         *
         **/
        namespace Execute
        {





            /** Trust
             *
             *  Handles the locking of stake in a stake register.
             *
             *  @param[out] trust The trust object register to stake.
             *  @param[in] nReward The reward to apply to trust account.
             *  @param[in] nScore The score to apply to trust account.
             *  @param[in] nTimestamp The timestamp to update register to.
             *
             *  @return true if successful.
             *
             **/
            bool Trust(TAO::Register::Object &trust, const uint64_t nReward, const uint64_t nScore, const uint64_t nTimestamp)


            /** Genesis
             *
             *  Handles the locking of stake in a stake register.
             *
             *  @param[out] trust The trust object register to stake.
             *  @param[in] nReward The reward to apply to trust account.
             *  @param[in] nTimestamp The timestamp to update register to.
             *
             *  @return true if successful.
             *
             **/
            bool Genesis(TAO::Register::Object &trust, const uint64_t nReward, const uint64_t nTimestamp)

        }
    }
}

#endif
