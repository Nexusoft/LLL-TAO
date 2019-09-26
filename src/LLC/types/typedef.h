/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLC_TYPES_TYPEDEF_H
#define NEXUS_LLC_TYPES_TYPEDEF_H

#include <Util/include/allocators.h>

namespace LLC
{

    /** CPrivKey is a serialized private key, with all parameters included **/
    typedef std::vector<uint8_t, secure_allocator<uint8_t> > CPrivKey;


    /** CSecret is a serialization of just the secret parameter **/
    typedef std::vector<uint8_t, secure_allocator<uint8_t> > CSecret;

}

#endif
