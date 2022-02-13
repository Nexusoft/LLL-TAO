/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/users/types/users.h>

#include <TAO/API/types/commands/crypto.h>
#include <TAO/API/types/commands.h>

#include <TAO/API/include/build.h>
#include <TAO/API/include/extract.h>

#include <TAO/Operation/include/enum.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Change the signature scheme used to generate the public-private keys for the users signature chain */
    encoding::json Crypto::ChangeScheme(const encoding::json& jParams, const bool fHelp)
    {
        /* Get the session to be used for this API call */
        const Session& session =
            Commands::Find<Users>()->GetSession(jParams);

        /* The address of the crypto object register, which is deterministic based on the genesis */
        const uint256_t hashRegister =
            TAO::Register::Address(std::string("crypto"), session.GetAccount()->Genesis(), TAO::Register::Address::CRYPTO);

        /* Read the crypto object register */
        TAO::Register::Object tCrypto;
        if(!LLD::Register->ReadObject(hashRegister, tCrypto, TAO::Ledger::FLAGS::MEMPOOL))
            throw Exception(-13, "Object not found");

        /* Grab our key scheme for generating crypto keys. */
        const uint8_t nKeyScheme =
            ExtractScheme(jParams);

        /* Build an operation stream for OP::WRITE. */
        TAO::Operation::Stream ssWrite;

        /* Get the PIN to be used for generating our new key hashes */
        const SecureString strPIN =
            Commands::Find<Users>()->GetPin(jParams, TAO::Ledger::PinUnlock::TRANSACTIONS);

        /* List our object fields for update. */
        const std::vector<std::string> vFieldNames = tCrypto.ListFields();

        /* Loop through our fields and check for hashes. */
        for(const auto& strField : vFieldNames)
        {
            /* Check for a correct hash-based type in case this register is polymorphic. */
            if(tCrypto.Check(strField, TAO::Operation::OP::TYPES::UINT256_T, true))
            {
                /* Continue if not assigned yet. */
                if(tCrypto.get<uint256_t>(strField) == 0)
                    continue;

                /* Build our operations stream for key generation. */
                ssWrite << std::string(strField) << uint8_t(TAO::Operation::OP::TYPES::UINT256_T);
                ssWrite << session.GetAccount()->KeyHash(strField, 0, strPIN, nKeyScheme);
            }
        }

        /* Add the crypto update contract. */
        std::vector<TAO::Operation::Contract> vContracts(1);
        vContracts[0] << uint8_t(TAO::Operation::OP::WRITE) << hashRegister << ssWrite.Bytes();

        return BuildResponse(jParams, hashRegister, vContracts);
    }
}
