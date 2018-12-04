/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_TAO_REGISTER_INCLUDE_ENUM_H
#define NEXUS_TAO_REGISTER_INCLUDE_ENUM_H

namespace TAO
{
    namespace Register
    {

        /** Operation Layer Byte Code. **/
        enum
        {
            OBJECT_RAW      = 0x00,
            OBJECT_ACCOUNT  = 0x01,
            OBJECT_TOKEN    = 0x02,
            OBJECT_ESCROW   = 0x03
        };
    }
}

#endif
