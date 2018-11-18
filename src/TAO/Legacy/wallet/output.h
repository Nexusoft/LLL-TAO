/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++

			(c) Copyright The Nexus Developers 2014 - 2018

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_LEGACY_WALLET_OUTPUT_H
#define NEXUS_LEGACY_WALLET_OUTPUT_H

#include <Util/include/parse.h> /* for FormatMoney() */

namespace Legacy
{
    
    namespace Wallet
    {
        /* forward declaration */
        class CWalletTx;

         /** Class to determine the value and depth of a specific transaction.
            Used for the Available Coins Method located in Wallet.cpp mainly.
            To be used for further purpose in the future. **/
        class COutput
        {
        public:
            const CWalletTx *tx;
            int i;
            int nDepth;

            COutput(const CWalletTx *txIn, int iIn, int nDepthIn)
            {
                tx = txIn; i = iIn; nDepth = nDepthIn;
            }

            std::string ToString() const
            {
                return strprintf("COutput(%s, %d, %d) [%s]", tx->GetHash().ToString().substr(0,10).c_str(), i, nDepth, FormatMoney(tx->vout[i].nValue).c_str());
            }

            void print() const
            {
                printf("%s\n", ToString().c_str());
            }
        };

    }
}

#endif
