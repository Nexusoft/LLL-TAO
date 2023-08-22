/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2023

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once

#include <cstdint>

namespace LLP
{

    class Config
    {
    public:

        /** Whether this server should listen for new connections **/
        bool ENABLE_LISTEN;


        /** Indicates if meter statistics should be logged **/
        bool ENABLE_METERS;


        /** Enables DDOS protection for the server **/
        bool ENABLE_DDOS;


        /** Indicates whether the server should use an address manager to establish new outgoing connections **/
        bool ENABLE_MANAGER;


        /** Indicates the server should attempt to use SSL if possible **/
        bool ENABLE_SSL;


        /** Indicates whether this server should accept connections from remote hosts, or only from localhost **/
        bool ENABLE_REMOTE;


        /** Indicates that SSL is required for connections and should not fall back to non-SSL **/
        bool REQUIRE_SSL;


        /** Port the server should listen on **/
        uint16_t PORT_BASE;


        /** Port the server should listen on for SSL connections **/
        uint16_t PORT_SSL;


        /** Max number of incoming connections this server can make. **/
        uint32_t MAX_INCOMING;


        /** Maximum number connections in total that this server can handle.  Must be greater than nMaxIncoming **/
        uint32_t MAX_CONNECTIONS;


        /** Max number of data threads this server should use **/
        uint16_t MAX_THREADS;


        /** The maximum number of connection attempts allowed per IP address per second, before the address is banned. **/
        uint32_t DDOS_CSCORE;


        /** The maximum number of connection attempts allowed per IP address per second, before the address is banned. **/
        uint32_t DDOS_RSCORE;


        /** The timespan used to calculate the moving average DDOS scores **/
        uint32_t DDOS_TIMESPAN;


        /** The time interval (in milliseconds) that the manager thread should use between new outgoing connection attempts **/
        uint32_t MANAGER_SLEEP;


        /** The timeout to set on new socket connections **/
        uint32_t SOCKET_TIMEOUT;


        /** Empty Constructor. **/
        Config() = delete;


        /** Default constructor **/
        Config(const uint16_t nPort)
        : ENABLE_LISTEN   (true)
        , ENABLE_METERS   (false)
        , ENABLE_DDOS     (false)
        , ENABLE_MANAGER  (true)
        , ENABLE_SSL      (false)
        , ENABLE_REMOTE   (false)
        , REQUIRE_SSL     (false)
        , PORT_BASE       (nPort)
        , PORT_SSL        (0)    //note: this should be an invalid port to bind to
        , MAX_INCOMING    (317)
        , MAX_CONNECTIONS (333)
        , MAX_THREADS     (4)    //default: 4 threads
        , DDOS_CSCORE     (1)    //default: 1 connection per second
        , DDOS_RSCORE     (1000) //default: 1000 requests per second
        , DDOS_TIMESPAN   (60)   //default: 60 second moving average window
        , MANAGER_SLEEP   (1000) //default: 1000 ms sleeps between connection attempts
        , SOCKET_TIMEOUT  (30)   //default: 30 seconds for socket timeout
        {
        }


        /** Copy Constructor. **/
        Config(const Config& config)
        : ENABLE_LISTEN   (config.ENABLE_LISTEN)
        , ENABLE_METERS   (config.ENABLE_METERS)
        , ENABLE_DDOS     (config.ENABLE_DDOS)
        , ENABLE_MANAGER  (config.ENABLE_MANAGER)
        , ENABLE_SSL      (config.ENABLE_SSL)
        , ENABLE_REMOTE   (config.ENABLE_REMOTE)
        , REQUIRE_SSL     (config.REQUIRE_SSL)
        , PORT_BASE       (config.PORT_BASE)
        , PORT_SSL        (config.PORT_SSL)
        , MAX_INCOMING    (config.MAX_INCOMING)
        , MAX_CONNECTIONS (config.MAX_CONNECTIONS)
        , MAX_THREADS     (config.MAX_THREADS)
        , DDOS_CSCORE     (config.DDOS_CSCORE)
        , DDOS_RSCORE     (config.DDOS_RSCORE)
        , DDOS_TIMESPAN   (config.DDOS_TIMESPAN)
        , MANAGER_SLEEP   (config.MANAGER_SLEEP)
        , SOCKET_TIMEOUT  (config.SOCKET_TIMEOUT)
        {
            /* Refresh our configuration values. */
            auto_config();
        }


        /** Move Constructor. **/
        Config(Config&& config)
        : ENABLE_LISTEN   (std::move(config.ENABLE_LISTEN))
        , ENABLE_METERS   (std::move(config.ENABLE_METERS))
        , ENABLE_DDOS     (std::move(config.ENABLE_DDOS))
        , ENABLE_MANAGER  (std::move(config.ENABLE_MANAGER))
        , ENABLE_SSL      (std::move(config.ENABLE_SSL))
        , ENABLE_REMOTE   (std::move(config.ENABLE_REMOTE))
        , REQUIRE_SSL     (std::move(config.REQUIRE_SSL))
        , PORT_BASE       (std::move(config.PORT_BASE))
        , PORT_SSL        (std::move(config.PORT_SSL))
        , MAX_INCOMING    (std::move(config.MAX_INCOMING))
        , MAX_CONNECTIONS (std::move(config.MAX_CONNECTIONS))
        , MAX_THREADS     (std::move(config.MAX_THREADS))
        , DDOS_CSCORE     (std::move(config.DDOS_CSCORE))
        , DDOS_RSCORE     (std::move(config.DDOS_RSCORE))
        , DDOS_TIMESPAN   (std::move(config.DDOS_TIMESPAN))
        , MANAGER_SLEEP   (std::move(config.MANAGER_SLEEP))
        , SOCKET_TIMEOUT  (std::move(config.SOCKET_TIMEOUT))
        {
            /* Refresh our configuration values. */
            auto_config();
        }


        /** Copy Assignment. **/
        Config& operator=(const Config& config)
        {
            ENABLE_LISTEN   = config.ENABLE_LISTEN;
            ENABLE_METERS   = config.ENABLE_METERS;
            ENABLE_DDOS     = config.ENABLE_DDOS;
            ENABLE_MANAGER  = config.ENABLE_MANAGER;
            ENABLE_SSL      = config.ENABLE_SSL;
            ENABLE_REMOTE   = config.ENABLE_REMOTE;
            REQUIRE_SSL     = config.REQUIRE_SSL;
            PORT_BASE       = config.PORT_BASE;
            PORT_SSL        = config.PORT_SSL;
            MAX_INCOMING    = config.MAX_INCOMING;
            MAX_CONNECTIONS = config.MAX_CONNECTIONS;
            MAX_THREADS     = config.MAX_THREADS;
            DDOS_CSCORE     = config.DDOS_CSCORE;
            DDOS_RSCORE     = config.DDOS_RSCORE;
            DDOS_TIMESPAN   = config.DDOS_TIMESPAN;
            MANAGER_SLEEP   = config.MANAGER_SLEEP;
            SOCKET_TIMEOUT  = config.SOCKET_TIMEOUT;

            /* Refresh our configuration values. */
            auto_config();

            return *this;
        }


        /** Move Assignment. **/
        Config& operator=(Config&& config)
        {
            ENABLE_LISTEN   = std::move(config.ENABLE_LISTEN);
            ENABLE_METERS   = std::move(config.ENABLE_METERS);
            ENABLE_DDOS     = std::move(config.ENABLE_DDOS);
            ENABLE_MANAGER  = std::move(config.ENABLE_MANAGER);
            ENABLE_SSL      = std::move(config.ENABLE_SSL);
            ENABLE_REMOTE   = std::move(config.ENABLE_REMOTE);
            REQUIRE_SSL     = std::move(config.REQUIRE_SSL);
            PORT_BASE       = std::move(config.PORT_BASE);
            PORT_SSL        = std::move(config.PORT_SSL);
            MAX_INCOMING    = std::move(config.MAX_INCOMING);
            MAX_CONNECTIONS = std::move(config.MAX_CONNECTIONS);
            MAX_THREADS     = std::move(config.MAX_THREADS);
            DDOS_CSCORE     = std::move(config.DDOS_CSCORE);
            DDOS_RSCORE     = std::move(config.DDOS_RSCORE);
            DDOS_TIMESPAN   = std::move(config.DDOS_TIMESPAN);
            MANAGER_SLEEP   = std::move(config.MANAGER_SLEEP);
            SOCKET_TIMEOUT  = std::move(config.SOCKET_TIMEOUT);

            /* Refresh our configuration values. */
            auto_config();

            return *this;
        }


        /** Destructor. **/
        ~Config()
        {
        }


    private:


        /** auto_config
         *
         *  Updates the automatic configuration values for the server object.
         *
         *  We can guarentee that when this config is either moved or copied, that it is in the
         *  constructor of the LLP, and thus in its final form before being consumed. This allows us
         *  to auto-configure some dynamic states that require previous static configurations.
         *
         **/
        void auto_config()
        {
            /* If SSL is require, so is the flag. */
            if(REQUIRE_SSL)
                ENABLE_SSL = true;


            //TODO: we want to add check_limits in here to verify configuration is not out of range.
        }

    };
}
