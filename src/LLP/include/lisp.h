/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once

#include <string>
#include <vector>
#include <map>

namespace LLP
{
    /** RLOC
     *
     *  Encapsulates a LISP RLOC (Routable LOCation)
     *
     **/
    class RLOC
    {
    public:

        /* The network interface name as described by the host OS */
        std::string strInterface;

        /* The translated IP address.  If this node is behind a NAT then this will contain
        *  the public IP address of the node's location */
        std::string strTranslatedRLOC;

        /* The hostname of this RLOC */
        std::string strRLOCName;
    };

    /** RLOC
     *
     *  Encapsulates a LISP EID (Endpoint IDentifier)
     *
     **/
    class EID
    {
    public:
        /* The IPV4 or IPV6 LISP assigned address */
        std::string strAddress;

        /* LISP instance ID showing which Nexus Network the node is on*/
        std::string strInstanceID;

        /* Lis of RLOC's mapped to this EID*/
        std::vector<RLOC> vRLOCs;
    };

    /** GetEIDs
    *
    *  Invokes the lispers.net API to obtain the EIDs and RLOCs used by this node
    *
    **/
    std::map<std::string, EID> GetEIDs();

    /** LispersAPIRequest
    *
    *  Makes a request to the lispers.net API for the desired endpoint
    *
    *  @param[in] strEndPoint The API endpoint to query
    *
    *  @return The response string from the lispers.net API.
    *
    **/
    std::string LispersAPIRequest(std::string strEndPoint);



}