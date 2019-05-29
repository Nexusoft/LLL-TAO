/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLC_INCLUDE_KEY_ERROR_H
#define NEXUS_LLC_INCLUDE_KEY_ERROR_H

#include <stdexcept>


namespace LLC
{
    /** Key Runtime Error Wrapper. **/
    class key_error : public std::runtime_error
    {
    public:
        explicit key_error(const std::string& str) : std::runtime_error(str) {}
    };
}

#endif
