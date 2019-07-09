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

    /** Event Enumeration. Used to determine each Event from LLP Server. **/
    enum
    {
        EVENT_HEADER         = 0,
        EVENT_PACKET         = 1,
        EVENT_CONNECT        = 2,
        EVENT_DISCONNECT     = 3,
        EVENT_GENERIC        = 4,
        EVENT_FAILED         = 5,

        EVENT_COMMAND        = 6, /* For Message Pushing to Server Processors */

        /* Disonnect reason flags */
        DISCONNECT_TIMEOUT    = 7,
        DISCONNECT_ERRORS     = 8,
        DISCONNECT_POLL_ERROR = 9,
        DISCONNECT_POLL_EMPTY = 10,
        DISCONNECT_DDOS       = 11,
        DISCONNECT_FORCE      = 12,
        DISCONNECT_PEER       = 13
    };

}

#endif
