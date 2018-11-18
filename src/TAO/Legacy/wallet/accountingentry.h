/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++

			(c) Copyright The Nexus Developers 2014 - 2018

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_LEGACY_WALLET_ACCOUNTINGENTRY_H
#define NEXUS_LEGACY_WALLET_ACCOUNTINGENTRY_H

#include <string>

#include <LLC/types/uint1024.h>

#include <Util/templates/serialize.h>

namespace Legacy 
{

    namespace Wallet
    {
         /** Internal transfers.
        * Database key is acentry<account><counter>.
        */
        class CAccountingEntry
        {
        public:
            std::string strAccount;
            int64_t nCreditDebit;
            int64_t nTime;
            std::string strOtherAccount;
            std::string strComment;

            CAccountingEntry()
            {
                SetNull();
            }

            void SetNull()
            {
                nCreditDebit = 0;
                nTime = 0;
                strAccount.clear();
                strOtherAccount.clear();
                strComment.clear();
            }

            IMPLEMENT_SERIALIZE
            (
                if (!(nSerType & SER_GETHASH))
                    READWRITE(nVersion);
                // Note: strAccount is serialized as part of the key, not here.
                READWRITE(nCreditDebit);
                READWRITE(nTime);
                READWRITE(strOtherAccount);
                READWRITE(strComment);
            )
        };

    }
}

#endif
