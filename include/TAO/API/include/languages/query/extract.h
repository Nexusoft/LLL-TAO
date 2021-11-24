/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2021

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once

#include <string>

/* Global TAO namespace. */
namespace TAO::API
{

    /** ExtractString
     *
     *  Extract a string value from `quoted` query syntax
     *
     *  @param[in] strExtract The string to extract parameter from.
     *
     *  @return The JSON statement generated with query.
     *
     **/
    __attribute__((const)) std::string ExtractString(const std::string& strExtract);
}
