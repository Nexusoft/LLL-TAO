/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/types/commands/profiles.h>

#include <TAO/Operation/include/enum.h>
#include <TAO/Operation/types/contract.h>

#include <TAO/Ledger/types/sigchain.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Update the public keys in crypto object register. */
    TAO::Operation::Contract Profiles::update_crypto_keys(const memory::encrypted_ptr<TAO::Ledger::SignatureChain>& pCredentials,
                                                          const SecureString& strPIN, const uint8_t nKeyType)
    {
        /* Generate register address for crypto register deterministically */
        const TAO::Register::Address addrCrypto =
            TAO::Register::Address(std::string("crypto"), pCredentials->Genesis(), TAO::Register::Address::CRYPTO);

        /* Read the existing crypto object register. */
        TAO::Register::Object oCrypto;
        if(!LLD::Register->ReadObject(addrCrypto, oCrypto))
            throw Exception(-33, "Failed to read crypto object register");

        /* Declare operation stream to serialize all of the field updates*/
        TAO::Operation::Stream ssUpdate;

        /* Update the AUTH key if enabled. */
        if(oCrypto.get<uint256_t>("auth") != 0)
        {
            ssUpdate << std::string("auth") << uint8_t(TAO::Operation::OP::TYPES::UINT256_T);
            ssUpdate << pCredentials->KeyHash("auth", 0, strPIN, nKeyType);
        }

        /* Update the LISP network key if enabled. */
        if(oCrypto.get<uint256_t>("lisp") != 0)
        {
            ssUpdate << std::string("lisp") << uint8_t(TAO::Operation::OP::TYPES::UINT256_T);
            ssUpdate << pCredentials->KeyHash("lisp", 0, strPIN, nKeyType);
        }

        /* Update the NETWORK key if enabled. */
        if(oCrypto.get<uint256_t>("network") != 0)
        {
            ssUpdate << std::string("network") << uint8_t(TAO::Operation::OP::TYPES::UINT256_T);
            ssUpdate << pCredentials->KeyHash("network", 0, strPIN, nKeyType);
        }

        /* Update the SIGN key if enabled. */
        if(oCrypto.get<uint256_t>("sign") != 0)
        {
            ssUpdate << std::string("sign") << uint8_t(TAO::Operation::OP::TYPES::UINT256_T);
            ssUpdate << pCredentials->KeyHash("sign", 0, strPIN, nKeyType);
        }

        /* Update the VERIFY key if enabled. */
        if(oCrypto.get<uint256_t>("verify") != 0)
        {
            ssUpdate << std::string("verify") << uint8_t(TAO::Operation::OP::TYPES::UINT256_T);
            ssUpdate << pCredentials->KeyHash("verify", 0, strPIN, nKeyType);
        }

        /* Add the crypto update contract. */
        TAO::Operation::Contract tContract;
        tContract << uint8_t(TAO::Operation::OP::WRITE) << addrCrypto << ssUpdate.Bytes();

        return tContract;
    }
}
