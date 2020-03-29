/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLP_TEMPLATES_EVENTS_H
#define NEXUS_LLP_TEMPLATES_EVENTS_H

namespace LLP
{

    /* Events for LLP packet processing. */
    namespace EVENTS
    {
        enum
        {
            HEADER         = 0,
            PACKET         = 1,
            CONNECT        = 2,
            DISCONNECT     = 3,
            GENERIC        = 4,
            FAILED         = 5,
            PROCESSED      = 6,
        };
    }


    /* Disonnect reason flags */
    namespace DISCONNECT
    {
        enum
        {
            TIMEOUT       = 7,
            ERRORS        = 8,
            POLL_ERROR    = 9,
            POLL_EMPTY    = 10,
            DDOS          = 11,
            FORCE         = 12,
            PEER          = 13,
            BUFFER        = 14,
            TIMEOUT_WRITE = 15,
        };
    }




}

#endif
