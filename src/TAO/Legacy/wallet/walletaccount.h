/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++

			(c) Copyright The Nexus Developers 2014 - 2018

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_LEGACY_WALLET_WALLETACCOUNT_H
#define NEXUS_LEGACY_WALLET_WALLETACCOUNT_H

#include <vector>

#include <LLC/types/uint1024.h>

#include <Util/templates/serialize.h>

namespace Legacy
{
    
    namespace Wallet
    {
        /** Account information.
        * Stored in wallet with key "acc"+string account name.
        */
        class CAccount
        {
        public:
            std::vector<uint8_t> vchPubKey;

            CAccount()
            {
                SetNull();
            }

            void SetNull()
            {
                vchPubKey.clear();
            }

            IMPLEMENT_SERIALIZE
            (
                if (!(nSerType & SER_GETHASH))
                    READWRITE(nVersion);
                READWRITE(vchPubKey);
            )
        };

    }
}

#endif
