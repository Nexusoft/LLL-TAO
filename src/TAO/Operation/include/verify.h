/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_TAO_OPERATION_INCLUDE_VERIFY_H
#define NEXUS_TAO_OPERATION_INCLUDE_VERIFY_H

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

        /** Verify
         *
         *  Namespace to hold primitive operation verify functions.
         *  These functions ONLY verify inputs, they DO NOT update the state.
         *
         **/
        namespace Verify
        {

            /** Write
             *
             *  Verify write and caller.
             *
             *  @param[in] state The state register to verify.
             *  @param[in] hashCaller The contract caller.
             *
             *  @return true if successful.
             *
             **/
            bool Write(const TAO::Register::State& state, const uint256_t& hashCaller);


            /** Append
             *
             *  Appends data to a register.
             *
             *  @param[out] state The state register to operate on.
             *  @param[in] vchData The data script to write into register.
             *  @param[in] nTimestamp The timestamp to update register to.
             *
             *  @return true if successful.
             *
             **/
            bool Append(TAO::Register::State &state, const std::vector<uint8_t>& vchData), const uint64_t nTimestamp);


            /** Create
             *
             *  Creates a new register if it doesn't exist.
             *
             *  @param[out] state The state register to operate on.
             *  @param[in] vchData The data script to write into register.
             *  @param[in] nTimestamp The timestamp to update register to.
             *
             *  @return true if successful.
             *
             **/
            bool Create(TAO::Register::State &state, const std::vector<uint8_t>& vchData, const uint64_t nTimestamp);


            /** Transfer
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
            bool Transfer(TAO::Register::State &state, const uint256_t& hashTransfer, const uint64_t nTimestamp);


            /** Claim
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
            bool Claim(TAO::Register::State &state, const uint256_t& hashClaim, const uint64_t nTimestamp);


            /** Debit
             *
             *  Authorizes funds from an account to an account
             *
             *  @param[out] account The object register to debit from.
             *  @param[in] nAmount The amount to debit from object.
             *  @param[in] nTimestamp The timestamp to update register to.
             *
             *  @return true if successful.
             *
             **/
            bool Debit(TAO::Register::Object &account, const uint64_t nAmount, const uint64_t nTimestamp)


            /** Credit
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
            bool Credit(TAO::Register::Object &account, const uint64_t nAmount, const uint64_t nTimestamp)


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
