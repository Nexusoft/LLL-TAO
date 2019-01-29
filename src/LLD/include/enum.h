/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_LLD_INCLUDE_ENUM_H
#define NEXUS_LLD_INCLUDE_ENUM_H

namespace LLD
{

    /** FLAGS
     *
     *  Database flags for keychains and sector.
     *
     **/
    struct FLAGS
    {
        enum
        {
            APPEND        = (1 << 1),
            READONLY      = (1 << 2),
            CREATE        = (1 << 3),
            WRITE         = (1 << 4),
            FORCE         = (1 << 5)
        };
    };


    /** STATE
     *
     *  Database states in the keychains.
     *
     **/
    struct STATE
    {
        enum
        {
            EMPTY 			= 0,
            READY 			= 1,
            TRANSACTION     = 2
        };
    };

}

#endif
