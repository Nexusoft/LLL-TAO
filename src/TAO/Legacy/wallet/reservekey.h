/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++

			(c) Copyright The Nexus Developers 2014 - 2018

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_LEGACY_WALLET_RESERVEKEY_H
#define NEXUS_LEGACY_WALLET_RESERVEKEY_H

#include <vector>

#include <LLC/types/uint1024.h>

namespace Legacy
{
    
    namespace Wallet
    {
        /* forward declaration */
        class CWallet;

       /** A key allocated from the key pool. */
        class CReserveKey
        {
        protected:
            CWallet* pwallet;
            int64_t nIndex;
            std::vector<uint8_t> vchPubKey;
 
        public:
            CReserveKey(CWallet* pwalletIn)
            {
                nIndex = -1;
                pwallet = pwalletIn;
            }

            ~CReserveKey()
            {
                if (!fShutdown)
                    ReturnKey();
            }

            void ReturnKey();
            std::vector<uint8_t> GetReservedKey();
            void KeepKey();
        };

    }
}

#endif
