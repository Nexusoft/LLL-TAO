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

#include <Util/templates/serialize.h>

namespace Legacy 
{

    namespace Wallet
    {
        /** @class CAccountingEntry
         * 
         * Supports accounting entries that internally transfer balance between wallet accounts
         * without a corresponding blockchain transaction.
         *
         * Accounting entries should be created in pairs. One will debit the "from" account
         * while the other will credit the "to" account. These accounts are recorded
         * as the strAccount value. Each entry also references the account on the other
         * side as the strOtherAccount value.
         *
         * Example: to record an internal transfer of 10 NXS from account A to account B
         * 
         * First accounting entry, debit from account A:
         *   - strAccount = account A
         *   - credit/debit amount = -10 
         *   - strOtherAccount = account B
         *
         * Second accounting entry, creidt to account B:
         *   - strAccount = account B
         *   - credit/debit amount = 10 
         *   - strOtherAccount = account A
         * 
         * Database key is acentry<account><counter>
         */
        class CAccountingEntry
        {
        public:
            /** Accounting entry is for this account **/
            std::string strAccount;


            /** References the account on the other side of this accounting entry**/
            std::string strOtherAccount;


            /** Accounting entry amount
             *
             *  Positive amount
             *  Negative amount
             **/
            int64_t nCreditDebit;


            /** Timestamp for creation of accounting entry **/
            int64_t nTime;


            /** General comment related to this accounting entry **/
            std::string strComment;


            /** Constructor
             *
             *  Calls SetNull() to initialize a null accounting entry 
             *
             **/
            CAccountingEntry()
            {
                SetNull();
            }


            /** SetNull
             *
             *  Clears an accounting entry by setting the credit/debit amount and
             *  and time timestamp to zero, and the account and comment strings to empty strings.
             *
             **/
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
