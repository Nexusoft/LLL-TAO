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
         *  @param[in] hashAddress The register address to write to.
         *  @param[in] hashCaller The calling signature chain.
         *  @param[in] nFlags The flag to determine if database state should be written.
         *  @param[out] tx The transaction calling operations
         *
         *  @return true if successful.
         *
         **/
        bool Write(const uint256_t &hashAddress, const std::vector<uint8_t> &vchData,
            const uint256_t &hashCaller, const uint8_t nFlags, TAO::Ledger::Transaction &tx);


        /** Append
         *
         *  Appends data to a register.
         *
         *  @param[in] hashAddress The register address to write to.
         *  @param[in] hashCaller The calling signature chain.
         *  @param[in] nFlags The flag to determine if database state should be written.
         *  @param[out] tx The transaction calling operations
         *
         *  @return true if successful.
         *
         **/
        bool Append(const uint256_t &hashAddress, const std::vector<uint8_t> &vchData,
            const uint256_t &hashCaller, const uint8_t nFlags, TAO::Ledger::Transaction &tx);


        /** Register
         *
         *  Creates a new register if it doesn't exist.
         *
         *  @param[in] hashAddress The register address to create.
         *  @param[in] nType The type of register being written.
         *  @param[in] vchData The binary data to record in register.
         *  @param[in] hashCaller The calling signature chain.
         *  @param[in] nFlags The flag to determine if database state should be written.
         *  @param[out] tx The transaction calling operations
         *
         *  @return true if successful.
         *
         **/
        bool Register(const uint256_t &hashAddress, const uint8_t nType,
            const std::vector<uint8_t> &vchData, const uint256_t &hashCaller, const uint8_t nFlags,
            TAO::Ledger::Transaction &tx);


        /** Transfer
         *
         *  Transfers a register between sigchains.
         *
         *  @param[in] hashAddress The register address to transfer.
         *  @param[in] hashTransfer The register to transfer to.
         *  @param[in] hashCaller The calling signature chain.
         *  @param[in] nFlags The flag to determine if database state should be written.
         *  @param[out] tx The transaction calling operations
         *
         *  @return true if successful.
         *
         **/
        bool Transfer(const uint256_t &hashAddress, const uint256_t &hashTransfer,
            const uint256_t &hashCaller, const uint8_t nFlags, TAO::Ledger::Transaction &tx);


        /** Debit
         *
         *  Authorizes funds from an account to an account
         *
         *  @param[in] hashFrom The account being transferred from.
         *  @param[in] hashTo The account being transferred to.
         *  @param[in] nAmount The amount being transferred
         *  @param[in] hashCaller The calling signature chain.
         *  @param[in] nFlags The flag to determine if database state should be written.
         *  @param[out] tx The transaction calling operations
         *
         *  @return true if successful.
         *
         **/
        bool Debit(const uint256_t &hashFrom, const uint256_t &hashTo, const uint64_t nAmount,
            const uint256_t &hashCaller, const uint8_t nFlags, TAO::Ledger::Transaction &tx);


        /** Credit
         *
         *  Commits funds from an account to an account
         *
         *  @param[in] hashTx The account being transferred from.
         *  @param[in] hashProof The proof address used in this credit.
         *  @param[in] hashTo The account being transferred to.
         *  @param[in] nCredit The amount being transferred
         *  @param[in] hashCaller The calling signature chain.
         *  @param[in] nFlags The flag to determine if database state should be written.
         *  @param[out] tx The transaction calling operations
         *
         *  @return true if successful.
         *
         **/
        bool Credit(const uint512_t &hashTx, const uint256_t &hashProof,
            const uint256_t &hashTo, const uint64_t nCredit, const uint256_t &hashCaller, const uint8_t nFlags,
            TAO::Ledger::Transaction &tx);


        /** Coinbase
         *
         *  Commits funds from a coinbase transaction
         *
         *  @param[in] hashAccount The account being transferred to.
         *  @param[in] nAmount The amount being transferred
         *  @param[in] hashCaller The calling signature chain.
         *  @param[in] nFlags The flag to determine if database state should be written.
         *  @param[out] tx The transaction calling operations
         *
         *  @return true if successful.
         *
         **/
        bool Coinbase(const uint256_t &hashAccount, const uint64_t nAmount,
            const uint256_t &hashCaller, const uint8_t nFlags, TAO::Ledger::Transaction &tx);


        /** Trust
         *
         *  Handles the locking of stake in a stake register.
         *
         *  @param[in] hashAccount The account being staked to
         *  @param[in] hashLastTrust The last trust block.
         *  @param[in] nSequence The last sequence number.
         *  @param[in] nLastTrust The last trust score.
         *  @param[in] nAmount The amount being transferred
         *  @param[in] hashCaller The calling signature chain.
         *  @param[in] nFlags The flag to determine if database state should be written.
         *  @param[out] tx The transaction calling operations
         *
         *  @return true if successful.
         *
         **/
        bool Trust(const uint256_t &hashAddress, const uint1024_t &hashLastTrust,
            const uint32_t nSequence, const uint64_t nLastTrust, const uint64_t nAmount, const uint256_t &hashCaller,
            const uint8_t nFlags, TAO::Ledger::Transaction &tx);


        /** Authorize
         *
         *  Authorizes an action if holder of a token.
         *
         *  @param[in] hashTx The transaction being authorized for.
         *  @param[in] hashProof The register temporal proof to use.
         *  @param[in] hashCaller The calling signature chain.
         *  @param[in] nFlags The flag to determine if database state should be written.
         *  @param[out] tx The transaction calling operations
         *
         *  @return true if successful.
         *
         **/
        bool Authorize(const uint512_t &hashTx, const uint256_t &hashProof,
            const uint256_t &hashCaller, const uint8_t nFlags, TAO::Ledger::Transaction &tx);
    }
}

#endif
