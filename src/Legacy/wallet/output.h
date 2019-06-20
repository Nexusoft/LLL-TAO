/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2019

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LEGACY_WALLET_OUTPUT_H
#define NEXUS_LEGACY_WALLET_OUTPUT_H

#include <string>

#include <Legacy/wallet/wallettx.h>

#include <Util/include/debug.h>
#include <Util/include/string.h> /* for FormatMoney() */

namespace Legacy
{

     /** @class Output
      *
      *  Class to determine the value and depth of a specific transaction output.
      *
      *  Used for the Available Coins Method located in Wallet.cpp mainly.
      *  To be used for further purpose in the future.
      **/
    class Output
    {
    public:
        /** The wallet transaction containing this output **/
        const WalletTx& walletTx;


        /** Index for this output in the transaction vout **/
        uint32_t i;


        /** Depth of transaction in chain at the time this Output created **/
        uint32_t nDepth;


        /** Constructor
         *
         *  Initializes this Output with the provided parameter values
         *
         *  @param[in] walletTxIn The transaction containing the corresponding output
         *
         *  @param[in] iIn The index of the output within the transactions's vout
         *
         *  @param[in] nDepthIn The depth of the transaction at time of Output creation
         *
         **/
        Output(const WalletTx& walletTxIn, const uint32_t iIn, const uint32_t nDepthIn)
        : walletTx(walletTxIn)
        , i(iIn)
        , nDepth(nDepthIn)
        {
        }


        /** Destructor **/
        ~Output()
        {
        }


        /** ToString
         *
         *  Generate a string representation of this output.
         *
         *  @return String representation of this output
         *
         **/
        std::string ToString() const
        {
            return debug::safe_printstr("Output(", walletTx.GetHash().SubString(10), ", ", i, ", ", nDepth, ") [", FormatMoney(walletTx.vout[i].nValue), "]");
        }


        /** print
         *
         *  Print a string representation of this output
         *
         **/
        inline void print() const
        {
            debug::log(0, ToString());
        }
    };

}

#endif
