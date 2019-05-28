/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_TAO_OPERATION_INCLUDE_OPERATIONS_H
#define NEXUS_TAO_OPERATION_INCLUDE_OPERATIONS_H

#include <TAO/Register/types/stream.h>
#include <TAO/Ledger/types/transaction.h>

/* Global TAO namespace. */
namespace TAO
{

    /* Operation Layer namespace. */
    namespace Operation
    {

        /** Write
         *
         *  Writes data to a register.
         *
         *  @param[out] state The state register to operate on.
         *  @param[in] vchData The data script to write into register.
         *
         *  @return true if successful.
         *
         **/
        bool Write(TAO::Register::State &state, const std::vector<uint8_t>& vchData);


        /** Append
         *
         *  Appends data to a register.
         *
         *  @param[out] state The state register to operate on.
         *  @param[in] vchData The data script to write into register.
         *
         *  @return true if successful.
         *
         **/
        bool Append(TAO::Register::State &state, const std::vector<uint8_t>& vchData);


        /** Create
         *
         *  Creates a new register if it doesn't exist.
         *
         *  @param[out] state The state register to operate on.
         *  @param[in] vchData The data script to write into register.
         *
         *  @return true if successful.
         *
         **/
        bool Create(TAO::Register::State &state, const std::vector<uint8_t>& vchData);


        /** Transfer
         *
         *  Transfers a register between sigchains.
         *
         *  @param[out] state The state register to operate on.
         *  @param[in] hashTransfer The object to transfer
         *
         *  @return true if successful.
         *
         **/
        bool Transfer(TAO::Register::State &state, const uint256_t& hashTransfer);


        /** Claim
         *
         *  Claims a register between sigchains.
         *
         *  @param[out] state The state register to operate on.
         *  @param[in] hashClaim The public-id being claimed to
         *
         *  @return true if successful.
         *
         **/
        bool Claim(TAO::Register::State &state, const uint256_t& hashClaim);


        /** Debit
         *
         *  Authorizes funds from an account to an account
         *
         *  @param[out] account The object register to debit from.
         *  @param[in] nAmount The amount to debit from object.
         *
         *  @return true if successful.
         *
         **/
        bool Debit(TAO::Register::Object &account, const uint64_t nAmount);


        /** Credit
         *
         *  Commits funds from an account to an account
         *
         *  @param[out] account The object register to credit to.
         *  @param[in] nAmount The amount to credit to object.
         *
         *  @return true if successful.
         *
         **/
        bool Credit(TAO::Register::Object &account, const uint64_t nAmount);


        /** Trust
         *
         *  Handles the locking of stake in a stake register.
         *
         *  @param[out] trust The trust object register to stake.
         *  @param[in] nReward The reward to apply to trust account.
         *  @param[in] nScore The score to apply to trust account.
         *
         *  @return true if successful.
         *
         **/
        bool Trust(TAO::Register::Object &trust, const uint64_t nReward, const uint64_t nScore);


        /** Genesis
         *
         *  Handles the locking of stake in a stake register.
         *
         *  @param[out] trust The trust object register to stake.
         *  @param[in] nReward The reward to apply to trust account.
         *
         *  @return true if successful.
         *
         **/
        bool Genesis(TAO::Register::Object &trust, const uint64_t nReward);

    }
}

#endif
