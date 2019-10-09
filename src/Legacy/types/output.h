/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2019

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LEGACY_TYPES_OUTPUT_H
#define NEXUS_LEGACY_TYPES_OUTPUT_H

#include <string>

namespace Legacy
{
    class WalletTx;
    

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


        /** The default constructor. **/
        Output()                             = delete;


        /** Copy Constructor. **/
        Output(const Output& out);


        /** Move Constructor. **/
        Output(Output&& out) noexcept;


        /** Copy Assignment. **/
        Output& operator=(const Output& out) = delete;


        /** Move Assignment. **/
        Output& operator=(Output&& out)      = delete;


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
        Output(const WalletTx& walletTxIn, const uint32_t iIn, const uint32_t nDepthIn);


        /** Destructor **/
        ~Output();


        /** ToString
         *
         *  Generate a string representation of this output.
         *
         *  @return String representation of this output
         *
         **/
        std::string ToString() const;


        /** print
         *
         *  Print a string representation of this output
         *
         **/
        void print() const;

    };

}

#endif
