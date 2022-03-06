/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/types/commands/sessions.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Standard initialization function. */
    void Sessions::Initialize()
    {
        /* Handle for all CREATE operations. */
        mapFunctions["create/master"] = Function
        (
            std::bind
            (
                &Sessions::Create,
                this,
                std::placeholders::_1,
                std::placeholders::_2
            )
        );
    }
}
