/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_TAO_REGISTER_INCLUDE_UNPACK_H
#define NEXUS_TAO_REGISTER_INCLUDE_UNPACK_H

#include <LLC/types/uint1024.h>

#include <TAO/Register/types/state.h>

#include <TAO/Ledger/types/transaction.h>

/* Global TAO namespace. */
namespace TAO
{

    /* Register Layer namespace. */
    namespace Register
    {

        /** Unpack
         *
         *  Unpack a state register from operation scripts.
         *
         *  @param[in] tx - the transaction to unpack
         *  @param[out] state - the unpacked register
         *  @param[out] hashAddress - the register address
         *
         *  @return true if register unpacked successfully
         *
         **/
        bool Unpack(const TAO::Ledger::Transaction& tx, State &state, uint256_t &hashAddress);


        /** Unpack
         *
         *  Unpack a source register address from operation scripts.
         *
         *  @param[in] tx - the transaction to unpack
         *  @param[out] hashAddress - one or more op code values (combine multiple with bitwise | )
         *
         *  @return true if the address unpacked successfully
         *
         **/
        bool Unpack(const TAO::Ledger::Transaction& tx, uint256_t &hashAddress);


        /** Unpack
         *
         *  Unpack a previous transaction from operation scripts.
         *
         *  @param[in] tx - the transaction to unpack
         *  @param[out] hashPrevTx 
         *
         *  @return true if the previous tx hash was unpacked successfully
         *
         **/
        bool Unpack(const TAO::Ledger::Transaction& tx, uint512_t& hashPrevTx);


        /** Unpack
         *
         *  Unpack a previous transaction and test for the operation it contains.
         *
         *  @param[in] tx - the transaction to unpack
         *  @param[in] opCodes - one or more op code values (combine multiple with bitwise | )
         *
         *  @return true if the transaction contains a requested op code
         *
         **/
        bool Unpack(const TAO::Ledger::Transaction& tx, const uint8_t opCodes);

    }
}

#endif
