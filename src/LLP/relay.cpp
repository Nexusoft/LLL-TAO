/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2021

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People
____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <LLP/types/relay.h>

#include <LLP/templates/events.h>

#include <TAO/API/types/authentication.h>

#include <TAO/Ledger/types/pinunlock.h>

namespace LLP
{

    /** Constructor **/
    RelayNode::RelayNode()
    : BaseConnection<MessagePacket> ( )
    , pqSSL   (new LLC::PQSSL_CTX())
    , oCrypto ( )
    , mapRoutingTable (new std::multimap<uint256_t, LLP::BaseAddress>())
    {
    }


    /** Constructor **/
    RelayNode::RelayNode(Socket SOCKET_IN, DDOS_Filter* DDOS_IN, bool fDDOSIn)
    : BaseConnection<MessagePacket> (SOCKET_IN, DDOS_IN, fDDOSIn)
    , pqSSL (new LLC::PQSSL_CTX())
    , oCrypto ( )
    , mapRoutingTable (new std::multimap<uint256_t, LLP::BaseAddress>())
    {
    }


    /** Constructor **/
    RelayNode::RelayNode(DDOS_Filter* DDOS_IN, bool fDDOSIn)
    : BaseConnection<MessagePacket> (DDOS_IN, fDDOSIn)
    , pqSSL (new LLC::PQSSL_CTX())
    , oCrypto ( )
    , mapRoutingTable (new std::multimap<uint256_t, LLP::BaseAddress>())
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
            if(pqSSL && !Incoming())
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
            /* This message is generated in response to an outgoing handshake. */
            case REQUEST::HANDSHAKE:
            {
                /* Check that this is not an outgoing connection. */
                if(!Incoming())
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

                    /* Get some of our connection related data now. */
                    ssPacket.SetPos(1);

                    /* Deserialize our handshake recipient. */
                    uint256_t hashGenesis;
                    ssPacket >> hashGenesis;

                    /* Generate register address for crypto register deterministically */
                    const TAO::Register::Address addrCrypto =
                        TAO::Register::Address(std::string("crypto"), hashGenesis, TAO::Register::Address::CRYPTO);

                    /* Read the existing crypto object register. */
                    if(!LLD::Register->ReadObject(addrCrypto, oCrypto, TAO::Ledger::FLAGS::LOOKUP))
                        return debug::drop(NODE, "Failed to read crypto object register");

                    /* Push this message now. */
                    PushMessage(RESPONSE::HANDSHAKE, vPayload);
                }

                break;
            }


            /* This message is generated in response to REQUEST::HANDSHAKE and completes the encryption channel. */
            case RESPONSE::HANDSHAKE:
            {
                /* Check that this is not an outgoing connection. */
                if(Incoming())
                    return debug::drop(NODE, "RESPONSE::HANDSHAKE is invalid for incoming connections");

                /* Complete our handshake sequence now. */
                if(!pqSSL->CompleteHandshake(vPlainText))
                    return debug::drop(NODE, "RESPONSE::HANDSHAKE: failed to complete handshake: ", debug::GetLastError());

                /* Get some of our connection related data now. */
                ssPacket.SetPos(1);

                /* Deserialize our handshake recipient. */
                uint256_t hashGenesis;
                ssPacket >> hashGenesis;

                /* Generate register address for crypto register deterministically */
                const TAO::Register::Address addrCrypto =
                    TAO::Register::Address(std::string("crypto"), hashGenesis, TAO::Register::Address::CRYPTO);

                /* Read the existing crypto object register. */
                if(!LLD::Register->ReadObject(addrCrypto, oCrypto, TAO::Ledger::FLAGS::LOOKUP))
                    return debug::drop(NODE, "Failed to read crypto object register");

                break;
            }


            /* Message to determine that a given user-id is being serviced by that node. */
            case RELAY::AVAILABLE:
            {
                /* Deserialize our user-id. */
                uint256_t hashGenesis;
                ssPacket >> hashGenesis;

                /* Check our broadcast time for replay protection. */
                uint64_t nTimestamp = 0;
                ssPacket >> nTimestamp;

                /* Check that handshake wasn't stale. */
                if(nTimestamp + 30 < runtime::unifiedtimestamp() || nTimestamp > runtime::unifiedtimestamp())
                {
                    /* Give us just a little warning message. */
                    debug::warning(NODE, "handshake is stale by ", runtime::unifiedtimestamp() - (nTimestamp + 30), " seconds");
                    break;
                }

                /* Deserialize the node. */
                LLP::BaseAddress addrRouter;
                ssPacket >> addrRouter;

                /* Assemble a datastream to hash. */
                DataStream ssSignature(SER_NETWORK, 1);
                ssSignature << hashGenesis << nTimestamp << addrRouter;

                /* Get a hash of our message to sign. */
                const uint256_t hashMessage =
                    LLC::SK256(ssSignature.Bytes());

                /* Generate register address for crypto register deterministically */
                const TAO::Register::Address addrCrypto =
                    TAO::Register::Address(std::string("crypto"), hashGenesis, TAO::Register::Address::CRYPTO);

                /* Read the existing crypto object register. */
                if(!LLD::Register->ReadObject(addrCrypto, oCrypto, TAO::Ledger::FLAGS::LOOKUP))
                    return debug::drop(NODE, "Failed to read crypto object register");

                /* Get our public key. */
                std::vector<uint8_t> vCryptoPub;
                ssPacket >> vCryptoPub;

                /* Get our current signature. */
                std::vector<uint8_t> vCryptoSig;
                ssPacket >> vCryptoSig;

                /* Verify our signature is a valid authentication of crypto object register. */
                if(!oCrypto.VerifySignature("network", hashMessage.GetBytes(), vCryptoPub, vCryptoSig))
                {
                    debug::warning(FUNCTION, "invalid signature for handshake authentication");
                    break;
                }

                /* Add this to our routing table. */
                mapRoutingTable->insert(std::make_pair(hashGenesis, addrRouter));

                break;
            }


            case REQUEST::COMMAND:
            {
                std::string strMessage;
                ssPacket >> strMessage;

                debug::log(0, NODE, strMessage);

                break;
            }
        }


        return true;
    }
}
