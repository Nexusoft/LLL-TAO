/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/Register/include/create.h>

/* Global TAO namespace. */
namespace TAO
{

    /* Register Layer namespace. */
    namespace Register
    {

        /** CreateAccount
         *
         *  Generate a new account object register.
         *
         *  @return The object register just created.
         *
         **/
        Object CreateAccount(const uint32_t nIdentifier)
        {
            /* Create an account object register. */
            TAO::Register::Object account;

            /* Generate the object register values. */
            account << std::string("balance")    << uint8_t(TAO::Register::TYPES::MUTABLE)  << uint8_t(TAO::Register::TYPES::UINT64_T) << uint64_t(0)
                    << std::string("identifier") << uint8_t(TAO::Register::TYPES::UINT32_T) << nIdentifier;

            return account;
        }


        /** CreateToken
         *
         *  Generate a new token object register.
         *
         *  @return The object register just created.
         *
         **/
        Object CreateToken(const uint32_t nIdentifier, const uint64_t nSupply, const uint64_t nDigits)
        {
            /* Create an token object register. */
            TAO::Register::Object token;

            /* Generate the object register values. */
            token   << std::string("balance")    << uint8_t(TAO::Register::TYPES::MUTABLE)  << uint8_t(TAO::Register::TYPES::UINT64_T) << nSupply
                    << std::string("identifier") << uint8_t(TAO::Register::TYPES::UINT32_T) << nIdentifier
                    << std::string("supply")     << uint8_t(TAO::Register::TYPES::UINT64_T) << nSupply
                    << std::string("digits")     << uint8_t(TAO::Register::TYPES::UINT64_T) << nDigits;

            return token;
        }

    }
}
