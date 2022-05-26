/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/include/argon2.h>
#include <LLC/include/encrypt.h>
#include <LLD/include/global.h>

#include <TAO/API/types/authentication.h>
#include <TAO/API/types/commands/sessions.h>
#include <TAO/API/types/commands.h>

#include <TAO/API/include/extract.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Saves the users session into the local DB so that it can be resumed later after a crash */
    encoding::json Sessions::Save(const encoding::json& jParams, const bool fHelp)
    {
        /* Pin parameter. */
        const SecureString strPIN =
            ExtractPIN(jParams);

        /* Get a copy of our linked credentials. */
        const auto& pCredentials =
            Authentication::Credentials(jParams);

        /* Get our genesis hash. */
        const uint256_t hashGenesis =
            Authentication::Caller(jParams);

        /* Use a data stream to help serialize */
        DataStream ssData(SER_LLD, 1);
        ssData << pCredentials->UserName();
        ssData << pCredentials->Password();

        /* Add the pin */
        std::vector<uint8_t> vchKey = hashGenesis.GetBytes();
        vchKey.insert(vchKey.end(), strPIN.begin(), strPIN.end());

        /* Get our symmetric cypher key for encryption. */
        const uint256_t hashKey =
            LLC::Argon2Fast_256(vchKey);

        /* Encrypt the data */
        std::vector<uint8_t> vchCipherText;
        if(!LLC::EncryptAES256(hashKey.GetBytes(), ssData.Bytes(), vchCipherText))
            throw Exception(-270, "Failed to encrypt data.");

        /* Write the session to local DB */
        LLD::Local->WriteSession(hashGenesis, vchCipherText);

        /* populate reponse */;
        const encoding::json jRet =
        {
            { "genesis",  hashGenesis.ToString() },
            { "success", true}
        };

        return jRet;
    }
}
