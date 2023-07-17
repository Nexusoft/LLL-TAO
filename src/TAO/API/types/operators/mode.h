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
    /** @namespace Operators
     *
     *  Namespace to hold all available operators for use in commands routes.
     *
     **/
    namespace Operators
    {

        /** Mode
         *
         *  Get's the most occurring value in the sequence. Supports bi-modal, tri-modal, and multi-modal.
         *
         *  @param[in] jParams The input parameters to the command.
         *  @param[in] jResult The list of objects to operate on.
         *
         *  @return the json result of the operations.
         *
         **/
        encoding::json Mode(const encoding::json& jParams, const encoding::json& jResult);

    }
}
