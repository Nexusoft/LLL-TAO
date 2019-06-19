/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/types/users.h>
#include <TAO/API/types/objects.h>

#include <TAO/API/include/global.h>

#include <TAO/Register/include/enum.h>


/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        /* Get a list of names owned by a signature chain. */
        json::json Users::Names(const json::json& params, bool fHelp)
        {
            /* For privacy reasons Names are only returned for the currently logged in user (multiuser=0) 
                or for the logged in session (multiuser=1). To ensure this, we first obtain the session and then inject this into
                the request, thereby allowing us to use the generic Object::List method */

            /* to make this logic clear to callers we will return an error if they have provided a username parameter */
            if(params.find("username") != params.end())
                throw APIException(-23, "Username parameter not supported for this method.  Names can only be obtained for the logged in user.");

            /* Get the callers hashGenesis */
            uint256_t hashGenesis = users->GetCallersGenesis(params);

            /* Ensure the hashGenesis is valid */
            if(hashGenesis == 0)
                throw APIException(-1, "User not logged in");

            /* Copy of the callers parmaters so that we can modify them */
            json::json paramsCopy = params;

            /* Inject the active session genesis hash into the parameters */
            paramsCopy["genesis"] = hashGenesis.GetHex();

            /* Call the generic List method to output the names */
            return Objects::List(paramsCopy, TAO::Register::REGISTER::OBJECT, TAO::Register::OBJECTS::NAME);
        }
    }
}
