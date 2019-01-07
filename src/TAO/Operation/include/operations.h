/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_TAO_OPERATION_INCLUDE_OPERATIONS_H
#define NEXUS_TAO_OPERATION_INCLUDE_OPERATIONS_H

#include <TAO/Register/include/stream.h>

namespace TAO::Operation
{

    /** Write
     *
     *  Writes data to a register.
     *
     *  @param[in] hashAddress The register address to write to.
     *  @param[in] hashCaller The calling signature chain.
     *  @param[in] nFlags The flag to determine if database state should be written.
     *  @param[ou] ssRegister The register pre and post states if applicable.
     *
     *  @return true if successful.
     *
     **/
    bool Write(uint256_t hashAddress, std::vector<uint8_t> vchData,
        uint256_t hashCaller, uint8_t nFlags, TAO::Register::Stream &ssRegister);


    /** Append
     *
     *  Appends data to a register.
     *
     *  @param[in] hashAddress The register address to write to.
     *  @param[in] hashCaller The calling signature chain.
     *  @param[in] nFlags The flag to determine if database state should be written.
     *  @param[ou] ssRegister The register pre and post states if applicable.
     *
     *  @return true if successful.
     *
     **/
    bool Append(uint256_t hashAddress, std::vector<uint8_t> vchData,
        uint256_t hashCaller, uint8_t nFlags, TAO::Register::Stream &ssRegister);


    /** Register
     *
     *  Creates a new register if it doesn't exist.
     *
     *  @param[in] hashAddress The register address to create.
     *  @param[in] nType The type of register being written.
     *  @param[in] vchData The binary data to record in register.
     *  @param[in] hashCaller The calling signature chain.
     *  @param[in] nFlags The flag to determine if database state should be written.
     *  @param[ou] ssRegister The register pre and post states if applicable.
     *
     *  @return true if successful.
     *
     **/
    bool Register(uint256_t hashAddress, uint8_t nType,
        std::vector<uint8_t> vchData, uint256_t hashCaller, uint8_t nFlags,
        TAO::Register::Stream &ssRegister);


    /** Transfer
     *
     *  Transfers a register between sigchains.
     *
     *  @param[in] hashAddress The register address to transfer.
     *  @param[in] hashTransfer The register to transfer to.
     *  @param[in] hashCaller The calling signature chain.
     *  @param[in] nFlags The flag to determine if database state should be written.
     *  @param[ou] ssRegister The register pre and post states if applicable.
     *
     *  @return true if successful.
     *
     **/
    bool Transfer(uint256_t hashAddress, uint256_t hashTransfer,
        uint256_t hashCaller, uint8_t nFlags, TAO::Register::Stream &ssRegister);


    /** Debit
     *
     *  Authorizes funds from an account to an account
     *
     *  @param[in] hashFrom The account being transferred from.
     *  @param[in] hashTo The account being transferred to.
     *  @param[in] nAmount The amount being transferred
     *  @param[in] hashCaller The calling signature chain.
     *  @param[in] nFlags The flag to determine if database state should be written.
     *  @param[ou] ssRegister The register pre and post states if applicable.
     *
     *  @return true if successful.
     *
     **/
    bool Debit(uint256_t hashFrom, uint256_t hashTo, uint64_t nAmount,
        uint256_t hashCaller, uint8_t nFlags, TAO::Register::Stream &ssRegister);


    /** Credit
     *
     *  Commits funds from an account to an account
     *
     *  @param[in] hashTx The account being transferred from.
     *  @param[in] hashProof The proof address used in this credit.
     *  @param[in] hashTo The account being transferred to.
     *  @param[in] nAmount The amount being transferred
     *  @param[in] hashCaller The calling signature chain.
     *  @param[in] nFlags The flag to determine if database state should be written.
     *  @param[ou] ssRegister The register pre and post states if applicable.
     *
     *  @return true if successful.
     *
     **/
    bool Credit(uint512_t hashTx, uint256_t hashProof,
        uint256_t hashTo, uint64_t nAmount, uint256_t hashCaller, uint8_t nFlags,
        TAO::Register::Stream &ssRegister);


    /** Coinbase
     *
     *  Commits funds from a coinbase transaction
     *
     *  @param[in] hashAccount The account being transferred to.
     *  @param[in] nAmount The amount being transferred
     *  @param[in] hashCaller The calling signature chain.
     *  @param[in] nFlags The flag to determine if database state should be written.
     *  @param[ou] ssRegister The register pre and post states if applicable.
     *
     *  @return true if successful.
     *
     **/
    bool Coinbase(uint256_t hashAccount, uint64_t nAmount,
        uint256_t hashCaller, uint8_t nFlags, TAO::Register::Stream &ssRegister);


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
     *  @param[ou] ssRegister The register pre and post states if applicable.
     *
     *  @return true if successful.
     *
     **/
    bool Trust(uint256_t hashAddress, uint1024_t hashLastTrust,
        uint32_t nSequence, uint32_t nLastTrust, uint64_t nAmount, uint256_t hashCaller,
        uint8_t nFlags, TAO::Register::Stream &ssRegister);


    /** Authorize
     *
     *  Authorizes an action if holder of a token.
     *
     *  @param[in] hashTx The transaction being authorized for.
     *  @param[in] hashProof The register temporal proof to use.
     *  @param[in] hashCaller The calling signature chain.
     *  @param[in] nFlags The flag to determine if database state should be written.
     *  @param[ou] ssRegister The register pre and post states if applicable.
     *
     *  @return true if successful.
     *
     **/
    bool Authorize(uint512_t hashTx, uint256_t hashProof,
        uint256_t hashCaller, uint8_t nFlags, TAO::Register::Stream &ssRegister);

}

#endif
