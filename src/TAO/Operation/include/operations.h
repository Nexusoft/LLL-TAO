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
         *  @param[out] contract The contract being executed
         *  @param[in] nTimestamp The timestamp of operation execution.
         *  @param[in] hashCaller The calling sigchain.
         *  @param[in] nFlags Flag to determine what states to write.
         *
         *  @return true if successful.
         *
         **/
        bool Write(Contract &contract, const uint64_t nTimestamp,
            const uint256_t& hashCaller, const uint8_t nFlags);


        /** Append
         *
         *  Appends data to a register.
         *
         *  @param[out] contract The contract being executed
         *  @param[in] nTimestamp The timestamp of operation execution.
         *  @param[in] hashCaller The calling sigchain.
         *  @param[in] nFlags Flag to determine what states to write.
         *
         *  @return true if successful.
         *
         **/
        bool Append(Contract &contract, const uint64_t nTimestamp,
            const uint256_t& hashCaller, const uint8_t nFlags);


        /** Create
         *
         *  Creates a new register if it doesn't exist.
         *
         *  @param[out] contract The contract being executed
         *  @param[in] nTimestamp The timestamp of operation execution.
         *  @param[in] hashCaller The calling sigchain.
         *  @param[in] nFlags Flag to determine what states to write.
         *
         *  @return true if successful.
         *
         **/
        bool Create(Contract &contract, const uint64_t nTimestamp,
            const uint256_t& hashCaller, const uint8_t nFlags);


        /** Transfer
         *
         *  Transfers a register between sigchains.
         *
         *  @param[in] hashAddress The register address to transfer.
         *  @param[in] hashTransfer The register to transfer to.
         *  @param[in] nFlags The flag to determine if database state should be written.
         *  @param[out] contract The contract being executed
         *
         *  @return true if successful.
         *
         **/
        bool Transfer(Contract &contract, const uint256_t& hashAddress,
            const uint256_t& hashTransfer, const uint8_t nFlags);


        /** Claim
         *
         *  Claims a register between sigchains.
         *
         *  @param[out] contract The contract being executed
         *  @param[in] hashTx The tx that is being claimed.
         *  @param[in] hashAddress The register address to claim.
         *  @param[in] nFlags The flag to determine if database state should be written.
         *
         *  @return true if successful.
         *
         **/
        bool Claim(Contract &contract, const uint512_t& hashTx,
            const uint8_t nFlags, const uint32_t nContract = 0);


        /** Debit
         *
         *  Authorizes funds from an account to an account
         *
         *  @param[out] contract The contract being executed
         *  @param[in] hashFrom The account being transferred from.
         *  @param[in] hashTo The account being transferred to.
         *  @param[in] nAmount The amount being transferred
         *  @param[in] nFlags The flag to determine if database state should be written.
         *
         *  @return true if successful.
         *
         **/
        bool Debit(Contract &contract, const uint256_t& hashFrom,
            const uint256_t& hashTo, const uint64_t nAmount,
            const uint8_t nFlags);


        /** Credit
         *
         *  Commits funds from an account to an account
         *
         *  @param[out] contract The contract being executed
         *  @param[in] hashTx The account being transferred from.
         *  @param[in] hashProof The proof address used in this credit.
         *  @param[in] hashTo The account being transferred to.
         *  @param[in] nCredit The amount being transferred
         *  @param[in] nFlags The flag to determine if database state should be written.
         *  @param[in] nContrat The contract output number (default: 0)
         *
         *  @return true if successful.
         *
         **/
        bool Credit(Contract &contract, const uint512_t& hashTx,
            const uint256_t& hashProof, const uint256_t& hashTo,
            const uint8_t nFlags, const uint32_t nContract = 0);


        /** Coinbase
         *
         *  Commits funds from a coinbase transaction
         *
         *  @param[out] contract The contract being executed
         *  @param[in] hashAccount The account being transferred to.
         *  @param[in] nAmount The amount being transferred
         *  @param[in] nFlags The flag to determine if database state should be written.
         *
         *  @return true if successful.
         *
         **/
        bool Coinbase(Contract &contract, const uint256_t& hashAccount,
            const uint64_t nAmount, const uint8_t nFlags);


        /** Trust
         *
         *  Handles the locking of stake in a stake register.
         *
         *  @param[out] contract The contract being executed
         *  @param[in] hashLastTrust The last stake transaction for the register.
         *  @param[in] nTrustScore The trust score for the operation.
         *  @param[in] nCoinstakeReward Coinstake reward paid to register by this operation
         *  @param[in] nFlags The flag to determine if database state should be written.
         *
         *  @return true if successful.
         *
         **/
        bool Trust(Contract &contract, const uint512_t& hashLastTrust,
            const uint64_t nTrustScore, const uint64_t nCoinstakeReward,
            const uint8_t nFlags);


        /** Genesis
         *
         *  Handles the locking of stake in a stake register.
         *
         *  @param[out] contract The contract being executed
         *  @param[in] hashAccount The account being staked to
         *  @param[in] nCoinstakeReward Coinstake reward paid to register by this operation
         *  @param[in] nFlags The flag to determine if database state should be written.
         *
         *  @return true if successful.
         *
         **/
        bool Genesis(Contract &contract, const uint256_t& hashAddress,
            const uint64_t nCoinstakeReward, const uint8_t nFlags);


        /** Authorize
         *
         *  Authorizes an action if holder of a token.
         *
         *  @param[out] contract The contract being executed
         *  @param[in] hashTx The transaction being authorized for.
         *  @param[in] hashProof The register temporal proof to use.
         *  @param[in] nFlags The flag to determine if database state should be written.
         *
         *  @return true if successful.
         *
         **/
        bool Authorize(Contract &contract, const uint512_t& hashTx,
            const uint256_t& hashProof, const uint8_t nFlags);
    }
}

#endif
