/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2021

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People
____________________________________________________________________________________________*/

#pragma once

#include <LLC/include/random.h>
#include <LLC/types/pqssl.h>

#include <LLD/include/global.h>

#include <LLP/templates/message-connection.h>

#include <TAO/API/types/authentication.h>

#include <TAO/Ledger/types/pinunlock.h>


#include <Util/types/lock_unique_ptr.h>

namespace LLP
{

    class RelayNode : public MessageConnection
    {

        /** Create a context to track SSL related data. **/
        LLC::PQSSL_CTX* pqSSL;


        /** Crypto Object Register of Peer for Verification. **/
        TAO::Register::Crypto oCrypto;


        /** Internal map to track RTR's that are servicing each user-id. **/
        static util::atomic::lock_unique_ptr<std::map<uint256_t, std::set<LLP::BaseAddress>>> mapExternalRoutes;


    public:

        /** Requests are core functions to ask for response. **/
        struct REQUEST
        {
            enum : MessagePacket::message_t
            {
                /* Object Types. */
                RESERVED1     = 0x00,

                HANDSHAKE     = 0x01, //establish a new handshake
                SESSION       = 0x02, //establish a new session
                FIND          = 0x03, //find a given node by genesis-id
                LIST          = 0x04, //list active datatypes
                PING          = 0x05,
                COMMAND       = 0x06,
                AVAILABLE     = 0x07,

                RESERVED2     = 0x08,
            };

            /** VALID
             *
             *  Inline function to check if message request is valid request.
             *
             *  @param[in] nMsg The message value to check if valid.
             *
             *  @return true if the request was in range.
             */
            static inline bool VALID(const MessagePacket::message_t nMsg)
            {
                return (nMsg > RESERVED1 && nMsg < RESERVED2);
            }
        };

        /** A response is a given message response to requests. **/
        struct RESPONSE
        {
            enum : MessagePacket::message_t
            {
                HANDSHAKE       = 0x11, //respond with response data for handshake to exchange keys
                PONG            = 0x12, //pong messages give us latency and keep connection alive
                COMMAND         = 0x13, //command response passes command information back to remote host
                AVAILABLE       = 0x14, //check if a given user-id is available for relays
                INVALID         = 0x15, //enum to tell if a given handshake was valid or not
            };
        };

        /** Relay messages are messages meant to be passed on to another available node. **/
        struct RELAY
        {
            enum : MessagePacket::message_t
            {
                AVAILABLE = 0x20, //this message is responsible for keeping the list of active nodes synced
                FORWARD   = 0x21, //this message is responsible for forwarding between nodes
            };
        };


        /** Name
         *
         *  Returns a string for the name of this type of Node.
         *
         **/
        static std::string Name() { return "Relay"; }


        /** Constructor **/
        RelayNode();


        /** Constructor **/
        RelayNode(Socket SOCKET_IN, DDOS_Filter* DDOS_IN, bool fDDOSIn = false);


        /** Constructor **/
        RelayNode(DDOS_Filter* DDOS_IN, bool fDDOSIn = false);


        /* Virtual destructor. */
        virtual ~RelayNode();


        /** Event
         *
         *  Virtual Functions to Determine Behavior of Message LLP.
         *
         *  @param[in] EVENT The byte header of the event type.
         *  @param[in[ LENGTH The size of bytes read on packet read events.
         *
         */
        void Event(uint8_t EVENT, uint32_t LENGTH = 0);


        /** ProcessPacket
         *
         *  Main message handler once a packet is recieved.
         *
         *  @return True is no errors, false otherwise.
         *
         **/
        bool ProcessPacket();


        /** NewMessage
         *
         *  Creates a new message with a commands and data.
         *
         *  @param[in] nMsg The message type.
         *  @param[in] ssData A datastream object with data to write.
         *
         *  @return Returns a filled out tritium packet.
         *
         **/
        MessagePacket NewMessage(const uint16_t nMsg, const DataStream& ssData)
        {
            /* Build our packet to send now. */
            MessagePacket RESPONSE(nMsg);

            /* Encrypt packet if we have valid context. */
            if(pqSSL && pqSSL->Completed())
            {
                /* Encrypt our packet payload now. */
                std::vector<uint8_t> vCipherText;
                pqSSL->Encrypt(ssData.Bytes(), vCipherText);

                /* Set our packet payload. */
                RESPONSE.DATA = vCipherText;
            }

            /* Otherwise no encryption set data like normal. */
            else
                RESPONSE.DATA = ssData.Bytes();

            return RESPONSE;
        }


        /** PushMessage
         *
         *  Adds a relay packet to the queue to write to the socket.
         *
         *  @param[in] nMsg The message type.
         *
         **/
        void PushMessage(const uint16_t nMsg, const std::vector<uint8_t>& vData)
        {
            /* Build our packet to send now. */
            MessagePacket RESPONSE(nMsg);

            /* Encrypt packet if we have valid context. */
            if(pqSSL && pqSSL->Completed())
            {
                /* Encrypt our packet payload now. */
                std::vector<uint8_t> vCipherText;
                pqSSL->Encrypt(vData, vCipherText);

                /* Set our packet payload. */
                RESPONSE.DATA = vCipherText;
            }

            /* Otherwise no encryption set data like normal. */
            else
                RESPONSE.DATA = vData;

            /* Write the packet to our pipe. */
            WritePacket(RESPONSE);

            /* We want to track verbose to save some copies into log buffers. */
            if(config::nVerbose >= 4)
                debug::log(4, NODE, "sent message ", std::hex, nMsg, " of ", std::dec, vData.size(), " bytes");
        }


        /** PushMessage
         *
         *  Adds a relay packet to the queue to write to the socket.
         *
         *  @param[in] nMsg The message type.
         *
         **/
        void PushMessage(const uint16_t nMsg, const DataStream& ssData)
        {
            /* Write the packet to our pipe. */
            WritePacket(NewMessage(nMsg, ssData));

            /* We want to track verbose to save some copies into log buffers. */
            if(config::nVerbose >= 4)
                debug::log(4, NODE, "sent message ", std::hex, nMsg, " of ", std::dec, ssData.size(), " bytes");
        }


        /** PushMessage
         *
         *  Adds a tritium packet to the queue to write to the socket.
         *
         **/
        template<typename... Args>
        void PushMessage(const uint16_t nMsg, Args&&... args)
        {
            DataStream ssData(SER_NETWORK, PROTOCOL_VERSION);
            ((ssData << args), ...);

            WritePacket(NewMessage(nMsg, ssData));

            /* We want to track verbose to save some copies into log buffers. */
            if(config::nVerbose >= 4)
                debug::log(4, NODE, "sent message ", std::hex, nMsg, " of ", std::dec, ssData.size(), " bytes");
        }


        /** VerifyMessage
         *
         *  Verifies a signed message with any arbitrary data for use in authenticated messages
         *
         *  @param[out] hashGenesis The user-id that generated the message.
         *  @param[in] ssPacket The packet's binary data to verify.
         *  @param[in] args Variadic template parameter pack.
         *
         **/
        template<typename... Args>
        bool VerifyMessage(uint256_t &hashGenesis, const DataStream& ssPacket, Args&&... args)
        {
            /* Deserialize our user-id. */
            ssPacket >> hashGenesis;

            /* Check our broadcast time for replay protection. */
            uint64_t nTimestamp = 0;
            ssPacket >> nTimestamp;

            /* Check that handshake wasn't stale. */
            if(nTimestamp + 30 < runtime::unifiedtimestamp() || nTimestamp > runtime::unifiedtimestamp())
                return debug::error(FUNCTION, "message is stale by ", runtime::unifiedtimestamp() - (nTimestamp + 30), " seconds");

            /* Get our public key. */
            std::vector<uint8_t> vCryptoPub;
            ssPacket >> vCryptoPub;

            /* Get our current signature. */
            std::vector<uint8_t> vCryptoSig;
            ssPacket >> vCryptoSig;

            /* Assemble a datastream to hash. */
            DataStream ssSignature(SER_NETWORK, PROTOCOL_VERSION);
            ssSignature << hashGenesis << nTimestamp;

            /* Now apply the rest from the parameter pack. */
            ((ssSignature << args), ...);

            /* Get a hash of our message to sign. */
            const uint256_t hashMessage =
                LLC::SK256(ssSignature.Bytes());

            /* Generate register address for crypto register deterministically */
            const TAO::Register::Address addrCrypto =
                TAO::Register::Address(std::string("crypto"), hashGenesis, TAO::Register::Address::CRYPTO);

            /* Read the existing crypto object register. */
            if(!LLD::Register->ReadObject(addrCrypto, oCrypto, TAO::Ledger::FLAGS::LOOKUP))
                return debug::error(FUNCTION, "Failed to read crypto object register");

            /* Verify our signature is a valid authentication of crypto object register. */
            if(!oCrypto.VerifySignature("network", hashMessage.GetBytes(), vCryptoPub, vCryptoSig))
                return debug::error(FUNCTION, "invalid signature for handshake authentication");

            return true;
        }


        /** SignMessage
         *
         *  Generates a signed message with any arbitrary data for use in authenticated messages
         *
         *  @param[in] nMsg The message type.
         *  @param[in] args Variadic template parameter pack.
         *
         **/
        template<typename... Args>
        bool SignMessage(const uint16_t nMsg, Args&&... args)
        {
            /* Only send auth messages if the auth key has been cached */
            SecureString strPIN;
            if(!TAO::API::Authentication::GetPIN(TAO::Ledger::PinUnlock::NETWORK, strPIN))
                return debug::error(FUNCTION, "no available session to sign message");

            /* Get an instance of our credentials. */
            const auto& pCredentials =
                TAO::API::Authentication::Credentials();

            /* Get our genesis from credentials. */
            const uint256_t hashGenesis =
                pCredentials->Genesis();

            /* Grab our current timestamp from unified clock. */
            const uint64_t nTimestamp =
                runtime::unifiedtimestamp();

            /* Assemble a datastream to hash. */
            DataStream ssSignature(SER_NETWORK, PROTOCOL_VERSION);
            ssSignature << hashGenesis << nTimestamp;

            /* Now apply the rest from the parameter pack. */
            ((ssSignature << args), ...);

            /* Get a hash of our message to sign. */
            const uint256_t hashMessage =
                LLC::SK256(ssSignature.Bytes());

            /* Generate register address for crypto register deterministically */
            const TAO::Register::Address addrCrypto =
                TAO::Register::Address(std::string("crypto"), hashGenesis, TAO::Register::Address::CRYPTO);

            /* Read the existing crypto object register. */
            if(!LLD::Register->ReadObject(addrCrypto, oCrypto, TAO::Ledger::FLAGS::LOOKUP))
                return debug::error(FUNCTION, "failed to read crypto object register");

            /* Build our signature and public key. */
            std::vector<uint8_t> vCryptoPub;
            std::vector<uint8_t> vCryptoSig;

            /* Verify our signature is a valid authentication of crypto object register. */
            if(!oCrypto.GenerateSignature("network", pCredentials, strPIN, hashMessage.GetBytes(), vCryptoPub, vCryptoSig))
                return debug::error(FUNCTION, "failed to generate signature for signed message");

            /* Build our message packet. */
            DataStream ssPacket(SER_NETWORK, PROTOCOL_VERSION);
            ((ssPacket << args), ...);

            /* Add our message data now. */
            ssPacket << hashGenesis << nTimestamp << vCryptoPub << vCryptoSig;

            /* Write the packet to our pipe now. */
            WritePacket(NewMessage(nMsg, ssPacket));

            /* We want to track verbose to save some copies into log buffers. */
            if(config::nVerbose >= 4)
                debug::log(4, NODE, "sent message ", std::hex, nMsg, " of ", std::dec, ssPacket.size(), " bytes");

            return true;
        }


        /** RelayFilter
         *
         *  Checks our main relay filters to send off to main relay nodes.
         *
         *  @return a data stream with relevant relay information
         *
         **/
        const DataStream RelayFilter(const uint16_t nMsg, const DataStream& ssData) const;
    };
}
