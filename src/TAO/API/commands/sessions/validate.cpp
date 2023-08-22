/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2023

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/types/commands/sessions.h>

#include <TAO/API/types/authentication.h>
#include <TAO/API/types/transaction.h>

#include <TAO/Ledger/types/mempool.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Authenticate a session to crypto object register to make sure credentials are correct. */
    bool Sessions::validate_session(const Authentication::Session& tSession, const SecureString& strPIN)
    {
        /* Read our first entry and accept to local mempool if we login. */
        TAO::API::Transaction tx;
        if(LLD::Logical->ReadFirst(tSession.Genesis(), tx) && !tx.Confirmed())
            TAO::Ledger::mempool.Accept(tx); //we don't care if it fails here because we will get invalid credentials if it did

        /* Check for crypto object register. */
        const TAO::Register::Address hashCrypto =
            TAO::Register::Address(std::string("crypto"), tSession.Genesis(), TAO::Register::Address::CRYPTO);

        /* Read the crypto object register. */
        TAO::Register::Object oCrypto;
        if(!LLD::Register->ReadObject(hashCrypto, oCrypto, TAO::Ledger::FLAGS::FORCED))
            throw Exception(-130, "Account doesn't exist or connection failed.");

        /* Read the key type from crypto object register. */
        const uint256_t hashAuth =
            oCrypto.get<uint256_t>("auth");

        /* Check if the auth has is deactivated. */
        if(hashAuth == 0)
            throw Exception(-130, "Auth hash deactivated, please call crypto/create/auth");

        /* Generate a key to check credentials against. */
        const uint256_t hashCheck =
            tSession.Credentials()->SignatureKey("auth", strPIN, hashAuth.GetType());

        /* Check for invalid authorization hash. */
        return (hashAuth == hashCheck);
    }
}
