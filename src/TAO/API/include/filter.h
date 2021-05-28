/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once

#include <Util/include/json.h>

namespace TAO::Register { class Object; }

namespace TAO::API
{

    /** FilterObject
     *
     *  Determines if an object should be included in a list based on input parameters.
     *
     *  @param[in] jParams The input parameters for the command.
     *  @param[in] objCheck The object we are checking for.
     *
     *  @return true if the object should be included in the results.
     *
     **/
    bool FilterObject(const json::json& jParams, const TAO::Register::Object& objCheck);

}
