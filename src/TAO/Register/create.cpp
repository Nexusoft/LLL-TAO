/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/Register/include/create.h>

#include <TAO/Register/include/enum.h>

/* Global TAO namespace. */
namespace TAO
{

    /* Register Layer namespace. */
    namespace Register
    {

        /* Generate a new account object register. */
        Object CreateAccount(const uint256_t& nIdentifier)
        {
            /* Create an account object register. */
            TAO::Register::Object account;

            /* Generate the object register values. */
            account << std::string("balance")    << uint8_t(TYPES::MUTABLE)  << uint8_t(TYPES::UINT64_T) << uint64_t(0)
                    << std::string("token_address") << uint8_t(TYPES::UINT256_T) << nIdentifier;

            return account;
        }


        /* Generate a new trust object register. */
        Object CreateTrust()
        {
            /* Create an account object register. */
            TAO::Register::Object trust;

            /* Generate the object register values. */
            trust   << std::string("balance")    << uint8_t(TYPES::MUTABLE)  << uint8_t(TYPES::UINT64_T) << uint64_t(0)
                    << std::string("trust")      << uint8_t(TYPES::MUTABLE)  << uint8_t(TYPES::UINT64_T) << uint64_t(0)
                    << std::string("stake")      << uint8_t(TYPES::MUTABLE)  << uint8_t(TYPES::UINT64_T) << uint64_t(0)
                    << std::string("token_address") << uint8_t(TYPES::UINT256_T) << uint256_t(0);

            return trust;
        }


        /* Generate a new token object register. */
        Object CreateToken(const uint256_t& nIdentifier, const uint64_t nSupply, const uint64_t nDigits)
        {
            /* Create an token object register. */
            TAO::Register::Object token;

            /* Generate the object register values. */
            token   << std::string("balance")    << uint8_t(TYPES::MUTABLE)  << uint8_t(TYPES::UINT64_T) << nSupply
                    << std::string("token_address") << uint8_t(TYPES::UINT256_T) << nIdentifier
                    << std::string("supply")     << uint8_t(TYPES::UINT64_T) << nSupply
                    << std::string("digits")     << uint8_t(TYPES::UINT64_T) << nDigits;

            return token;
        }

    }
}
