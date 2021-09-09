/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once

#include <cstdint>

namespace LLP
{

    class ServerConfig
    {
    public:

        /** Default constructor **/
        ServerConfig();


        /** Port the server should listen on **/
        uint16_t nPort;


        /** Port the server should listen on for SSL connections **/
        uint16_t nSSLPort;


        /** Whether this server should listen for new connections **/
        bool fListen;


        /** Indicates whether this server should accept connections from remote hosts, or only from localhost **/
        bool fRemote;


        /** Indicates the server should attempt to use SSL if possible **/
        bool fSSL;


        /** Indicates that SSL is required for connections and should not fall back to non-SSL **/
        bool fSSLRequired;


        /** Max number of outgoing connections this server can make. **/
        uint32_t nMaxIncoming;


        /** Maximum number connections in total that this server can handle.  Must be greater than nMaxIncoming **/
        uint32_t nMaxConnections;
        

        /** Max number of data threads this server should use **/
        uint16_t nMaxThreads;
        

        /** The timeout to set on new socket connections **/
        uint32_t nTimeout;


        /** Indicates if meter statistics should be logged **/
        bool fMeter;


        /** Enables DDOS protection for the server **/
        bool fDDOS;


        /** The maximum number of connection attempts allowed per IP address per second, before the address is banned.  This is
            calculated as a moving average over the DDOS timespan **/
        uint32_t nDDOSCScore;
        

        /** The maximum number of connection attempts allowed per IP address per second, before the address is banned.  This is
            calculated as a moving average over the DDOS timespan **/
        uint32_t nDDOSRScore;
        

        /** The timespan used to calculate the moving average DDOS scores **/
        uint32_t nDDOSTimespan;
        

        /** Indicates whether the server should use an address manager to establish new outgoing connections **/
        bool fManager;


        /** The time interval (in milliseconds) that the manager thread should use between new outgoing connection attempts **/
        uint32_t nManagerInterval;          

    };

}
