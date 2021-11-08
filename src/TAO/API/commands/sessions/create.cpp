/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/include/check.h>
#include <TAO/API/include/extract.h>

#include <TAO/API/types/commands/sessions.h>

#include <TAO/Ledger/types/sigchain.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Login to a user account. */
    encoding::json Sessions::Create(const encoding::json& jParams, const bool fHelp)
    {
        /* Pin parameter. */
        const SecureString strPIN = ExtractPIN(jParams);

        /* Check for username parameter. */
        if(!CheckParameter(jParams, "username", "string"))
            throw Exception(-127, "Missing username");

        /* Parse out username. */
        const SecureString strUser =
            SecureString(jParams["username"].get<std::string>().c_str());

        /* Check for password parameter. */
        if(!CheckParameter(jParams, "password", "string"))
            throw Exception(-128, "Missing password");

        /* Parse out password. */
        const SecureString strPass =
            SecureString(jParams["password"].get<std::string>().c_str());

        /* Create a temp sig chain for checking credentials */
        const TAO::Ledger::SignatureChain tUser =
            TAO::Ledger::SignatureChain(strUser, strPass);

        /* Get the genesis ID of the user logging in. */
        const uint256_t hashGenesis = tUser.Genesis();

        /* JSON return value. */
        encoding::json jRet;

        return jRet;
    }
}
