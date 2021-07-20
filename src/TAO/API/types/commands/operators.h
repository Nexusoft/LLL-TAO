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
    struct Operators
    {

        /** Array
         *
         *  Computes values into a single item array
         *
         *  @param[in] jParams The input parameters to the command.
         *  @param[in] jResult The list of objects to operate on.
         *
         *  @return the json result of the operations.
         *
         **/
        static encoding::json Array(const encoding::json& jParams, const encoding::json& jResult);


        /** Average
         *
         *  Computes average values into a single item array
         *
         *  @param[in] jParams The input parameters to the command.
         *  @param[in] jResult The list of objects to operate on.
         *
         *  @return the json result of the operations.
         *
         **/
        static encoding::json Average(const encoding::json& jParams, const encoding::json& jResult);


        /** Sum
         *
         *  Computes values list by addition.
         *
         *  @param[in] jParams The input parameters to the command.
         *  @param[in] jResult The list of objects to operate on.
         *
         *  @return the json result of the operations.
         *
         **/
        static encoding::json Sum(const encoding::json& jParams, const encoding::json& jResult);

    };
}
