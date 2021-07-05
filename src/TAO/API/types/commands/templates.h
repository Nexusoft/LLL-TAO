/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once

#include <Util/include/json.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /** @struct Templates
     *
     *  Structure to hold the template commands we will use for other command-sets.
     *
     **/
    struct Templates
    {

        /** Get
         *
         *  Get's a register by address or name.
         *
         *  @param[in] jParams The input parameters to the command.
         *  @param[in] fHelp Flag to determine if help was requested for command.
         *
         *  @return the json representation of given object.
         *
         **/
        static encoding::json Get(const encoding::json& jParams, const bool fHelp);
    };
}
