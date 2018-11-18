/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++

			(c) Copyright The Nexus Developers 2014 - 2018

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_LEGACY_WALLET_ADDRDB_H
#define NEXUS_LEGACY_WALLET_ADDRDB_H

#include <TAO/Legacy/wallet/db.h>

namespace Legacy 
{

    /* forward declaration */    
    namespace Net
    {
        class CAddrMan;
    }

    namespace Wallet
    {
       /** Access to the (IP) address database (addr.dat) */
        class CAddrDB : public CDB
        {
        public:
            CAddrDB(const char* pszMode="r+") : CDB("addr.dat", pszMode) { }
        private:
            CAddrDB(const CAddrDB&);
            void operator=(const CAddrDB&);
        public:
            bool WriteAddrman(const Legacy::Net::CAddrMan& addr);
            bool LoadAddresses();
        };

        bool LoadAddresses();

    }
}

#endif 
