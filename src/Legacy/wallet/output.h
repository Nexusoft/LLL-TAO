/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2018

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_LEGACY_WALLET_OUTPUT_H
#define NEXUS_LEGACY_WALLET_OUTPUT_H

#include <string>

#include <Legacy/wallet/wallettx.h>

#include <Util/include/debug.h> 
#include <Util/include/string.h> /* for FormatMoney() */

namespace Legacy
{
    
     /** @class COutput
      *
      *  Class to determine the value and depth of a specific transaction output.
      *
      *  Used for the Available Coins Method located in Wallet.cpp mainly.
      *  To be used for further purpose in the future. 
      **/
    class COutput
    {
    public:
        /** The wallet transaction containing this output **/
        const CWalletTx& walletTx;


        /** Index for this output in the transaction vout **/
        uint32_t i;


        /** Depth of transaction in chain at the time this COutput created **/
        uint32_t nDepth;


        /** Constructor
         *
         *  Initializes this COutput with the provided parameter values
         *
         *  @param[in] walletTxIn The transaction containing the corresponding output
         *
         *  @param[in] iIn The index of the output within the transactions's vout
         * 
         *  @param[in] nDepthIn The depth of the transaction at time of COutput creation
         *
         **/
        COutput(const CWalletTx& walletTxIn, uint32_t iIn, uint32_t nDepthIn) :
            walletTx(walletTxIn), 
            i(iIn),
            nDepth(nDepthIn)
        { }


        /** ToString
         *
         *  Generate a string representation of this output.
         * 
         *  @return String representation of this output
         *
         **/
        std::string ToString() const
        {
            return debug::strprintf("COutput(%s, %d, %d) [%s]", walletTx.GetHash().ToString().substr(0,10).c_str(), i, nDepth, FormatMoney(walletTx.vout[i].nValue).c_str());
        }


        /** print
         *
         *  Print a string representation of this output
         *
         **/
        inline void print() const
        {
            printf("%s", ToString().c_str());
        }
    };

}

#endif
