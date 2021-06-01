/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/users/types/users.h>
#include <TAO/API/objects/types/objects.h>

#include <TAO/Register/include/enum.h>


/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        /* Get a list of tokens owned by a signature chain. */
        encoding::json Users::Tokens(const encoding::json& params, const bool fHelp)
        {
            return Objects::List(params, TAO::Register::REGISTER::OBJECT, TAO::Register::OBJECTS::TOKEN);
        }
    }
}
