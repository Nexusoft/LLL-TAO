/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++

			(c) Copyright The Nexus Developers 2014 - 2018

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_LEGACY_WALLET_WALLETKEY_H
#define NEXUS_LEGACY_WALLET_WALLETKEY_H

#include <LLC/types/uint1024.h>

#include <Util/templates/serialize.h>

/* forward declaration */    
namespace LLC 
{
    class CPrivKey;
}

namespace Legacy
{
    
    namespace Wallet
    {
         /** Class to hold Private key binary data. */
        class CWalletKey
        {
        public:
            LLC::CPrivKey vchPrivKey;
            int64_t nTimeCreated;
            int64_t nTimeExpires;
            std::string strComment;
            //// todo: add something to note what created it (user, getnewaddress, change)
            ////   maybe should have a map<string, string> property map

            CWalletKey(int64_t nExpires=0)
            {
                nTimeCreated = (nExpires ? GetUnifiedTimestamp() : 0);
                nTimeExpires = nExpires;
            }

            IMPLEMENT_SERIALIZE
            (
                if (!(nSerType & SER_GETHASH))
                    READWRITE(nVersion);
                READWRITE(vchPrivKey);
                READWRITE(nTimeCreated);
                READWRITE(nTimeExpires);
                READWRITE(strComment);
            )
        };

    }
}

#endif
