/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2019

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <Legacy/types/account.h>

namespace Legacy
{

    /* The default constructor. */
    Account::Account()
    : vchPubKey ( )
    {
    }


    /* Copy Constructor. */
    Account::Account(const Account& account)
    : vchPubKey (account.vchPubKey)
    {
    }


    /* Move Constructor. */
    Account::Account(Account&& account) noexcept
    : vchPubKey (std::move(account.vchPubKey))
    {
    }


    /* Copy Assignment. */
    Account& Account::operator=(const Account& account)
    {
        vchPubKey = account.vchPubKey;

        return *this;
    }


    /* Move Assignment. **/
    Account& Account::operator=(Account&& account) noexcept
    {
        vchPubKey = std::move(account.vchPubKey);

        return *this;
    }


    /* Default Destructor */
    Account::~Account()
    {
    }


    /* Clears the current public key value. */
    void Account::SetNull()
    {
        vchPubKey.clear();
    }
}
