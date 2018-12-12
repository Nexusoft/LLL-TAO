/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/Operation/include/operations.h>

#include <TAO/Register/include/enum.h>
#include <TAO/Register/include/state.h>
#include <TAO/Register/objects/account.h>

namespace TAO::Operation
{

    /* Authorizes funds from an account to an account */
    bool Debit(uint256_t hashFrom, uint256_t hashTo, uint64_t nAmount, uint256_t hashCaller)
    {
        /* Read the register from the database. */
        TAO::Register::State regFrom = TAO::Register::State();
        if(!LLD::regDB->ReadState(hashFrom, regFrom))
            return debug::error(FUNCTION "register %s doesn't exist in register DB", __PRETTY_FUNCTION__, hashFrom.ToString().c_str());

        /* Check ownership of register. */
        if(regFrom.hashOwner != hashCaller)
            return debug::error(FUNCTION "%s caller not authorized to debit from register", __PRETTY_FUNCTION__, hashCaller.ToString().c_str());

        /* Skip all non account registers for now. */
        if(regFrom.nType != TAO::Register::OBJECT::ACCOUNT)
            return debug::error(FUNCTION "%s is not an account object", __PRETTY_FUNCTION__, hashFrom.ToString().c_str());

        /* Get the account object from register. */
        TAO::Register::Account acctFrom;
        regFrom >> acctFrom;

        /* Check the balance of the from account. */
        if(acctFrom.nBalance < nAmount)
            return debug::error(FUNCTION "%s doesn't have sufficient balance", __PRETTY_FUNCTION__, hashFrom.ToString().c_str());

        /* Change the state of account register. */
        acctFrom.nBalance -= nAmount;

        /* Clear the state of register. */
        regFrom.ClearState();
        regFrom << acctFrom;

        /* Write this to operations stream. */
        Write(hashFrom, regFrom.GetState(), hashCaller);
        //LLD::regDB->WriteState(hashFrom, regFrom); //need to do this in operation script to check ownership of register

        return true;
    }
}
