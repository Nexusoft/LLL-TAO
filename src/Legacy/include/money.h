/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2019

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To The Voice of The People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LEGACY_INCLUDE_MONEY_H
#define NEXUS_LEGACY_INCLUDE_MONEY_H

#include <TAO/Ledger/include/timelocks.h>

#include <Util/include/runtime.h>
#include <Util/include/args.h>
#include <Util/include/convert.h>

#include <inttypes.h>


namespace Legacy
{

    /** Total Significant figures in a coin. **/
    const uint64_t COIN = 1000000;


    /** Total Significant figures in a cent. **/
    const uint64_t CENT = 10000;


    /** Minimum Output can be 1 Satoshi. **/
    const uint64_t MIN_TXOUT_AMOUNT = 1;


    /** Minimum Transaction Fees are 1 cent. **/
    const uint64_t MIN_TX_FEE = CENT;


    /** Minimum Transaction Relay Fees are 1 cent. */
    const uint64_t MIN_RELAY_TX_FEE = CENT;


    /** Max TxOut
     *
     *  Maximum value out per transaction
     *
     *  @return the maximum value out
     *
     **/
    inline int64_t MaxTxOut()
    {
        if(runtime::unifiedtimestamp() > (config::fTestNet ? TAO::Ledger::TESTNET_VERSION_TIMELOCK[3] : TAO::Ledger::NETWORK_VERSION_TIMELOCK[3]))
            return 50000000 * COIN;

        return 1000000 * COIN;
    }


    /** Money Range
     *
     *  Small function to prevent overflow bugs.
     *
     *  @param[in] nValue The value to check range for.
     *
     *  @return true if value is within range.
     *
     **/
    inline bool MoneyRange(int64_t nValue)
    {
        return (nValue >= 0 && nValue <= MaxTxOut());
    }

    /** SatoshisToAmount
    *
    *  Converts a value represented in satoshis to a coin Amount
    *
    *  @param[in] satoshis The value in satoshis to be converted.
    *
    *  @return satoshis value converted to coin Amount.
    *
    **/
    inline double SatoshisToAmount(int64_t viz)
    {
        return (double)viz / (double)Legacy::COIN;
    }

    inline int64_t AmountToSatoshis(double dAmount)
    {
        if (dAmount <= 0.0 || dAmount > static_cast<double>(Legacy::MaxTxOut()))
            throw std::runtime_error( "Invalid amount");
        int64_t nAmount = convert::roundint64(dAmount * COIN);
        if (!MoneyRange(nAmount))
            throw std::runtime_error("Invalid amount");
        return nAmount;
    }

}

#endif
