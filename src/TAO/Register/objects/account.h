/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_TAO_REGISTER_OBJECTS_ACCOUNT_H
#define NEXUS_TAO_REGISTER_OBJECTS_ACCOUNT_H

#include <stdio.h>
#include <cstdint>

namespace TAO
{

    namespace Register
    {

        /** Account Object.
         *
         *  Holds the state of account identifier and account balance.
         *
         **/
        class Account
        {
        public:

            /** The version of this account. **/
            uint32_t nVersion;


            /** The identifier of the account token. **/
            uint32_t nIdentifier;


            /** The balance of total tokens in account. **/
            uint64_t nBalance;


            /** Default Constructor. **/
            Account()
            {
                SetNull();
            }


            /** Consturctor
             *
             *  @param[in] nIdentifierIn The identifier in
             *  @param[in] nBalanceIn The balance in
             *
             **/
            Account(uint32_t nIdentifierIn, uint64_t nBalanceIn) : nVersion(1), nIdentifier(nIdentifierIn), nBalance(nBalanceIn) {}


            /** SetNull
             *
             *  Set this object to null state.
             *
             **/
            void SetNull()
            {
                nVersion       = 0;
                nIdentifier = 0;
                nBalance       = 0;
            }


            /** IsNull
             *
             *  Checks if object is in null state
             *
             **/
            bool IsNull() const
            {
                return (nVersion == 0 && nIdentifier == 0 && nBalance == 0);
            }


            /** print
             *
             *  Output the state of this object.
             *
             **/
            void print() const
            {
                debug::log("Account(version=%u, id=%u, balance=%" PRIu64 ")\n", nVersion, nIdentifier, nBalance);
            }
        };
    }
}

#endif
