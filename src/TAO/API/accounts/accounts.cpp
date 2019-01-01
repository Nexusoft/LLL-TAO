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
    uint512_t Accounts::GetKey(uint32_t nKey, SecureString strSecret, uint64_t nSession) const
    {
        LOCK(MUTEX);

        /* Check if you are logged in. */
        if(!mapSessions.count(nSession))
            throw APIException(-1, debug::strprintf("session %" PRIu64 " doesn't exist", nSession));

        return mapSessions[nSession]->Generate(nKey, strSecret);
    }


    /* Returns the genesis ID from the account logged in. */
    uint256_t Accounts::GetGenesis(uint64_t nSession) const
    {
        LOCK(MUTEX);

        /* Check if you are logged in. */
        if(!mapSessions.count(nSession))
            throw APIException(-1, debug::strprintf("session %" PRIu64 " doesn't exist", nSession));

        return LLC::SK256(mapSessions[nSession]->Generate(0, "genesis").GetBytes()); //TODO: Assess the security of being able to generate genesis. Most likely this should be a localDB thing.
    }
}
