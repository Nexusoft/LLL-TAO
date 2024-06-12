/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2023

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
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
            APPEND        = (1 << 1), //tell LLD to append on writes making writing faster
            READONLY      = (1 << 2), //tell LLD that no writes are allowed on this instance
            CREATE        = (1 << 3), //create associated directories if they don't exist
            WRITE         = (1 << 4), //tell LLD to write to buffer before flushing to disk, makes LLD faster on writes
            FORCE         = (1 << 5), //tell LLD to force write to disk for every key using no buffering
            KEYSTORE      = (1 << 6), //tell LLD datachain to store the keys along with data
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
