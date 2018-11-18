/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++

			(c) Copyright The Nexus Developers 2014 - 2018

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_LEGACY_WALLET_TIMEDB_H
#define NEXUS_LEGACY_WALLET_TIMEDB_H


#include <TAO/Legacy/wallet/db.h>

namespace Legacy
{
    
    namespace Wallet
    {
        /** Access to the Nexus Time Database (time.dat).
            This contains the Unified Time Data collected over time. */
        class CTimeDB : public CDB
        {
        public:
            CTimeDB(const char* pszMode="r+") : CDB("time.dat", pszMode) { }

        private:
            CTimeDB(const CTimeDB&);
            void operator=(const CTimeDB&);

        public:
            bool WriteTimeData(int nOffset);
            bool ReadTimeData(int& nOffset);
        };

    }
}

#endif
