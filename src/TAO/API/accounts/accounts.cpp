/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/include/accounts.h>

#include <TAO/Ledger/types/transaction.h>
#include <TAO/Ledger/types/sigchain.h>

#include <Util/include/hex.h>

namespace TAO::API
{
    /** List of accounts in API. **/
    Accounts accounts;


    /* Standard initialization function. */
    void Accounts::Initialize()
    {
        mapFunctions["login"]               = Function(std::bind(&Accounts::Login,           this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["logout"]              = Function(std::bind(&Accounts::Logout,          this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["create"]              = Function(std::bind(&Accounts::CreateAccount,   this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["transactions"]        = Function(std::bind(&Accounts::GetTransactions, this, std::placeholders::_1, std::placeholders::_2));
    }


    /* Returns a key from the account logged in. */
    uint512_t Accounts::GetKey(uint32_t nKey, SecureString strSecret) const
    {
        LOCK(MUTEX);

        /* Check if you are logged in. */
        if(!user)
            throw APIException(-1, "cannot get key if not logged in.");

        return user->Generate(nKey, strSecret);
    }


    /* Returns the genesis ID from the account logged in. */
    uint256_t Accounts::GetGenesis() const
    {
        LOCK(MUTEX);

        /* Check if you are logged in. */
        if(!user)
            throw APIException(-1, "cannot get genesis if not logged in.");

        return LLC::SK256(user->Generate(0, "genesis").GetBytes()); //TODO: Assess the security of being able to generate genesis. Most likely this should be a localDB thing.
    }
}
