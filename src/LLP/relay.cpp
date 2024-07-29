/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2021

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People
____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <LLP/types/relay.h>
#include <LLP/types/tritium.h>

#include <LLP/templates/events.h>

namespace LLP
{
    /* Map to track external RTR's that are servicing each user-id. */
    util::atomic::lock_unique_ptr<std::map<uint256_t, std::set<LLP::BaseAddress>>> RelayNode::mapExternalRoutes =
        new std::map<uint256_t, std::set<LLP::BaseAddress>>();

    /* Map to track internal connections that are servicing each user-id. */
    util::atomic::lock_unique_ptr<std::map<uint256_t, std::shared_ptr<RelayNode>>> RelayNode::mapInternalRoutes =
        new std::map<uint256_t, std::shared_ptr<RelayNode>>();

    /** Constructor **/
    RelayNode::RelayNode()
    : MessageConnection ( )
    , pqSSL   (new LLC::PQSSL_CTX())
    , oCrypto ( )
    {
    }


    /** Constructor **/
    RelayNode::RelayNode(Socket SOCKET_IN, DDOS_Filter* DDOS_IN, bool fDDOSIn)
    : MessageConnection (SOCKET_IN, DDOS_IN, fDDOSIn)
    , pqSSL (new LLC::PQSSL_CTX())
    , oCrypto ( )
    {
    }


    /** Constructor **/
    RelayNode::RelayNode(DDOS_Filter* DDOS_IN, bool fDDOSIn)
    : MessageConnection (DDOS_IN, fDDOSIn)
    , pqSSL (new LLC::PQSSL_CTX())
    , oCrypto ( )
    {
    }


    /* Virtual destructor. */
    RelayNode::~RelayNode()
    {
        /* Check for SSL context. */
        if(pqSSL)
            delete pqSSL;
    }


    /* Virtual Functions to Determine Behavior of Message LLP. */
    void RelayNode::Event(uint8_t EVENT, uint32_t LENGTH)
    {
        /* Handle any DDOS Packet Filters. */
        if(EVENT == EVENTS::HEADER)
            return;

        /* Handle for a Packet Data Read. */
        if(EVENT == EVENTS::PACKET)
            return;

        /* On Generic Event, Broadcast new block if flagged. */
        if(EVENT == EVENTS::GENERIC)
            return;

        /* On Connect Event, Assign the Proper Daemon Handle. */
        if(EVENT == EVENTS::CONNECT)
        {
            /* We want to initiate our handshake on connect. */
            if(pqSSL && Outgoing())
            {
                /* Only send auth messages if the auth key has been cached */
                SecureString strPIN;
                if(TAO::API::Authentication::GetPIN(TAO::Ledger::PinUnlock::NETWORK, strPIN))
                {
                    /* Get an instance of our credentials. */
                    const auto& pCredentials =
                        TAO::API::Authentication::Credentials();

                    /* Setup our SSL context. */
                    if(!pqSSL->Initialize(pCredentials, strPIN))
                    {
                        /* Verbose output of debug data. */
                        debug::warning(FUNCTION, "failed to initialize our PQ-SSL Context");
                        return;
                    }

                    /* Now we should have a valid context for use in SSL handshake. */
                    const std::vector<uint8_t> vPayload =
                        pqSSL->InitiateHandshake(pCredentials, strPIN);

                    /* Push this message now. */
                    PushMessage(REQUEST::HANDSHAKE, vPayload);
                }
            }

            return;
        }

        /* On Disconnect Event, Reduce the Connection Count for Daemon */
        if(EVENT == EVENTS::DISCONNECT)
            return;
    }


    /* Main message handler once a packet is recieved. */
    bool RelayNode::ProcessPacket()
    {
        /* Check if we need to decrypt our packet. */
        std::vector<uint8_t> vPlainText;
        if(pqSSL && pqSSL->Completed())
        {
            /* Try to decrypt our payload now. */
            if(!pqSSL->Decrypt(INCOMING.DATA, vPlainText))
                return debug::drop(NODE, "packet request decryption failed: ", debug::GetLastError());
        }

        /* Otherwise just copy our bytes to our buffer. */
        else
            vPlainText = INCOMING.DATA;

        /* Deserialize the packeet from incoming packet payload. */
        DataStream ssPacket(vPlainText, SER_NETWORK, PROTOCOL_VERSION);
        switch(INCOMING.MESSAGE)
        {
            /* This message requests a list of available nodes that service given genesis-id. */
            case REQUEST::AVAILABLE:
            {
                /* Deserialize our user-id. */
                uint256_t hashGenesis;
                ssPacket >> hashGenesis;

                /* Check if we have any available nodes. */
                std::set<LLP::BaseAddress> setAvailable;
                if(mapExternalRoutes->count(hashGenesis))
                    setAvailable = mapExternalRoutes->at(hashGenesis);

                /* Push our response of available nodes. */
                PushMessage(RESPONSE::AVAILABLE, setAvailable);

                break;
            }


            case REQUEST::COMMAND:
            {
                std::string strMessage;
                ssPacket >> strMessage;

                debug::log(0, NODE, strMessage);

                break;
            }


            /* This message is received by a node making an ougoing going. */
            case REQUEST::HANDSHAKE:
            {
                /* Make sure this is a incoming connection. */
                if(Outgoing())
                    return debug::drop(NODE, "REQUEST::HANDSHAKE is invalid for outgoing connections");

                /* Only send auth messages if the auth key has been cached */
                SecureString strPIN;
                if(TAO::API::Authentication::GetPIN(TAO::Ledger::PinUnlock::NETWORK, strPIN))
                {
                    /* Get an instance of our credentials. */
                    const auto& pCredentials =
                        TAO::API::Authentication::Credentials();

                    /* Setup our SSL context. */
                    if(!pqSSL->Initialize(pCredentials, strPIN))
                        return debug::drop(NODE, "failed to initialize our PQ-SSL Context: ", debug::GetLastError());

                    /* Now we can respond with a valid context for use in SSL handshake. */
                    const std::vector<uint8_t> vPayload =
                        pqSSL->RespondHandshake(pCredentials, strPIN, vPlainText);

                    /* Push this message now. */
                    PushMessage(RESPONSE::HANDSHAKE, vPayload);
                }

                break;
            }


            /* Request a new session with a connected node in internal routes. */
            case REQUEST::SESSION:
            {
                /* Make sure this is a incoming connection. */
                if(Outgoing())
                    return debug::drop(NODE, "REQUEST::SESSION is invalid for outgoing connections");

                /* Extract the genesis-id that we want to establish session for. */
                uint256_t hashGenesis;
                ssPacket >> hashGenesis;

                /* Check our map of internal routes. */
                if(!mapInternalRoutes->count(hashGenesis))
                {
                    /* Respond with error message. */
                    PushMessage(RESPONSE::INVALID, std::string("no internal routes"));
                    break;
                }

                /* Get our relay node from internal routes. */
                std::shared_ptr<RelayNode> pRelay =
                    mapInternalRoutes->at(hashGenesis);

                break;
            }


            /* This message is a response to any errors in requests. */
            case RESPONSE::INVALID:
            {
                /* Extract our error message. */
                std::string strMessage;
                ssPacket >> strMessage;

                debug::log(0, NODE, "RESPONSE::INVALID: received error message: ", strMessage);

                break;
            }


            /* This message is generated in response to REQUEST::HANDSHAKE and completes the encryption channel. */
            case RESPONSE::HANDSHAKE:
            {
                /* Check that this is an incoming connection. */
                if(Incoming())
                    return debug::drop(NODE, "RESPONSE::HANDSHAKE is invalid for incoming connections");

                /* Complete our handshake sequence now. */
                if(!pqSSL->CompleteHandshake(vPlainText))
                    return debug::drop(NODE, "RESPONSE::HANDSHAKE: failed to complete handshake: ", debug::GetLastError());

                /* Relay that we are available now with a signed message. */
                if(!SignMessage(RELAY::AVAILABLE, TritiumNode::addrThis.load()))
                    return debug::drop(NODE, "RESPONSE::HANDSHAKE: failed to relay signed address");

                break;
            }


            /* Message to forward a message from one node to another connected to this relay server. */
            case RELAY::FORWARD:
            {
                break;
            }


            /* Message to determine that a given user-id is being serviced by that node. */
            case RELAY::AVAILABLE:
            {
                /* Deserialize the node. */
                LLP::BaseAddress addrRouter;
                ssPacket >> addrRouter;

                /* Check that message was valid. */
                uint256_t hashGenesis;
                if(!VerifyMessage(hashGenesis, ssPacket, addrRouter))
                    return debug::drop(NODE, "RELAY::AVAILABLE: invalid message signature");

                /* Add this to our routing table. */
                if(!mapExternalRoutes->count(hashGenesis))
                    mapExternalRoutes->insert(std::make_pair(hashGenesis, std::set<LLP::BaseAddress>()));

                /* Insert as a record now. */
                mapExternalRoutes->at(hashGenesis).insert(addrRouter);

                /* Relay to all of our connected nodes. */
                //if(RELAY_SERVER)
                //    RELAY_SERVER->_Relay(RELAY::AVAILABLE, ssPacket);

                break;
            }
        }

        return true;
    }


    /* Checks if a node is subscribed to receive a notification. */
    const DataStream RelayNode::RelayFilter(const uint16_t nMsg, const DataStream& ssData) const
    {
        /* For an available node only push for outgoing connections. */
        if(nMsg == RELAY::AVAILABLE)
        {
            /* Only relay to outgoing nodes. */
            if(Outgoing())
                return ssData;
        }

        return DataStream(SER_NETWORK, MIN_PROTO_VERSION);
    }
}
