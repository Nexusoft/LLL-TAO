/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2021

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once

#include <LLC/types/uint1024.h>

#include <TAO/Register/types/state.h>

#include <TAO/Ledger/types/transaction.h>

/* Global TAO namespace. */
namespace TAO::Register
{
    /** Unpack
     *
     *  Unpack a state register from a contract
     *
     *  @param[in] rContract The contract to unpack from.
     *  @param[out] state the unpacked register
     *  @param[out] hashAddress the register address
     *
     *  @return true if register unpacked successfully
     *
     **/
    bool Unpack(const TAO::Operation::Contract& rContract, State &state, uint256_t &hashAddress);


    /** Unpack
     *
     *  Unpack a source register address from a contract
     *
     *  @param[in] rContract The contract to unpack from.
     *  @param[out] hashAddress register address to find.
     *
     *  @return true if the address unpacked successfully
     *
     **/
    bool Unpack(const TAO::Operation::Contract& rContract, uint256_t &hashAddress);


    /** Unpack
     *
     *  Unpack a previous transaction hash from given contract.
     *
     *  @param[in] contract The rContract to unpack from.
     *  @param[out] hashPrevTx finds a previous transaction
     *
     *  @return true if the previous tx hash and contract ID was unpacked successfully
     *
     **/
    bool Unpack(const TAO::Operation::Contract& rContract, uint512_t& hashPrevTx);


    /** Unpack
     *
     *  Unpacks a source txid and contract from an OP::VALIDATE transaction.
     *
     *  @param[in] contract The rContract to unpack from.
     *  @param[out] hashPrevTx finds a previous transaction
     *
     *  @return true if the previous tx hash and contract ID was unpacked successfully
     *
     **/
    bool Unpack(const TAO::Operation::Contract& rContract, uint512_t& hashPrevTx, uint32_t &nContract);


    /** Unpack
     *
     *  Unpacks the values used to read and write proofs for any given transaction type.
     *
     *  @param[in] rContract The contract to unpack from.
     *  @param[in] hashProof The proof included in transfer transaction.
     *  @param[out] hashPrevTx finds a previous transaction
     *  @param[out] nContract the contract ID of the previous transaction
     *
     *  @return true if the previous tx hash and contract ID was unpacked successfully
     *
     **/
    bool Unpack(const TAO::Operation::Contract& rContract, uint256_t& hashProof, uint512_t& hashPrevTx, uint32_t& nContract);


    /** Unpack
     *
     *  Unpack the amount of NXS in contract.
     *
     *  Will unpack amount value from any primitive operator that has an amount.
     *  Stake or unstake unpacks the amount of NXS added to or removed from stake.
     *
     *  Other operations return false with nAmount of zero.
     *
     *  @param[in] rContract The contract to unpack from.
     *  @param[out] nAmount NXS amount included in contract operation
     *
     *  @return true if the amount was unpacked successfully
     *
     **/
    bool Unpack(const TAO::Operation::Contract& rContract, uint64_t& nAmount);


    /** Unpack
     *
     *  Unpack an op legacy contract to find it's output script.
     *
     *  @param[in] rContract the contract to unpack
     *  @param[out] script The script to populate from the contract
     *
     *  @return true if the transaction contains the requested op code
     *
     **/
    bool Unpack(const TAO::Operation::Contract& rContract, Legacy::Script& script);

}
