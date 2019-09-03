/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/include/random.h>

#include <LLD/include/global.h>

#include <LLP/types/tritium.h>
#include <LLP/include/global.h>
#include <LLP/templates/events.h>
#include <LLP/include/manager.h>

#include <TAO/Register/include/names.h>
#include <TAO/Register/types/object.h>

#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/types/mempool.h>

#include <Legacy/wallet/wallet.h>

#include <Util/include/runtime.h>
#include <Util/include/args.h>
#include <Util/include/debug.h>


#include <climits>
#include <memory>
#include <iomanip>

namespace LLP
{

    /** Default Constructor **/
    TritiumNode::TritiumNode()
    : BaseConnection<TritiumPacket>()
    , fAuthorized(false)
    , nLastPing(0)
    , nLastSamples(0)
    , mapLatencyTracker()
    , hashGenesis(0)
    , nTrust(0)
    {
    }


    /** Constructor **/
    TritiumNode::TritiumNode(Socket SOCKET_IN, DDOS_Filter* DDOS_IN, bool isDDOS)
    : BaseConnection<TritiumPacket>(SOCKET_IN, DDOS_IN, isDDOS)
    , fAuthorized(false)
    , nLastPing(0)
    , nLastSamples(0)
    , mapLatencyTracker()
    , hashGenesis(0)
    , nTrust(0)
    {
    }


    /** Constructor **/
    TritiumNode::TritiumNode(DDOS_Filter* DDOS_IN, bool isDDOS)
    : BaseConnection<TritiumPacket>(DDOS_IN, isDDOS)
    , fAuthorized(false)
    , nLastPing(0)
    , nLastSamples(0)
    , mapLatencyTracker()
    , hashGenesis(0)
    , nTrust(0)
    {
    }


    /** Default Destructor **/
    TritiumNode::~TritiumNode()
    {
    }


    /** Virtual Functions to Determine Behavior of Message LLP. **/
    void TritiumNode::Event(uint8_t EVENT, uint32_t LENGTH)
    {
        switch(EVENT)
        {
            case EVENT_CONNECT:
            {
                debug::log(1, NODE, fOUTGOING ? "Outgoing" : "Incoming",
                       " Connected at timestamp ",   runtime::unifiedtimestamp());

                break;
            }

            case EVENT_HEADER:
            {
                break;
            }

            case EVENT_PACKET:
            {
                /* Check a packet's validity once it is finished being read. */
                if(fDDOS)
                {

                    /* Give higher score for Bad Packets. */
                    if(INCOMING.Complete() && !INCOMING.IsValid())
                    {

                        debug::log(3, NODE "Dropped Packet (Complete: ", INCOMING.Complete() ? "Y" : "N",
                            " - Valid:)",  INCOMING.IsValid() ? "Y" : "N");

                        if(DDOS)
                            DDOS->rSCORE += 15;
                    }

                }

                if(INCOMING.Complete())
                {
                    debug::log(4, NODE "Received Packet (", INCOMING.LENGTH, ", ", INCOMING.GetBytes().size(), ")");

                    if(config::GetArg("-verbose", 0) >= 5)
                        PrintHex(INCOMING.GetBytes());
                }

                break;
            }

            case EVENT_GENERIC:
            {

                break;
            }

            case EVENT_DISCONNECT:
            {
                /* Debut output. */
                std::string strReason;
                switch(LENGTH)
                {
                    case DISCONNECT_TIMEOUT:
                        strReason = "Timeout";
                        break;

                    case DISCONNECT_ERRORS:
                        strReason = "Errors";
                        break;

                    case DISCONNECT_POLL_ERROR:
                        strReason = "Poll Error";
                        break;

                    case DISCONNECT_POLL_EMPTY:
                        strReason = "Unavailable";
                        break;

                    case DISCONNECT_DDOS:
                        strReason = "DDOS";
                        break;

                    case DISCONNECT_FORCE:
                        strReason = "Force";
                        break;

                    default:
                        strReason = "Unknown";
                        break;
                }

                /* Debug output for node disconnect. */
                debug::log(1, NODE, fOUTGOING ? "Outgoing" : "Incoming",
                    " Disconnected (", strReason, ") at timestamp ", runtime::unifiedtimestamp());

                if(TRITIUM_SERVER && TRITIUM_SERVER->pAddressManager)
                    TRITIUM_SERVER->pAddressManager->AddAddress(GetAddress(), ConnectState::DROPPED);

                break;
            }
        }
    }


    /** Main message handler once a packet is recieved. **/
    bool TritiumNode::ProcessPacket()
    {
        DataStream ssPacket(INCOMING.DATA, SER_NETWORK, PROTOCOL_VERSION);
        switch(INCOMING.MESSAGE)
        {

            /* Handle for auth command. */
            case ACTION::AUTH:
            {
                /* Hard requirement for genesis. */
                ssPacket >> hashGenesis;

                /* Debug logging. */
                debug::log(0, NODE, "new connection from ", hashGenesis.SubString());

                /* Get the signature information. */
                if(hashGenesis != 0)
                {
                    /* Verify the signature information. */
                    uint8_t nType = 0;
                    ssPacket >> nType;

                    /* Get the public key. */
                    std::vector<uint8_t> vchPubKey;
                    ssPacket >> vchPubKey;

                    /* Check the public key to expected authorization key. */
                    uint256_t hashAuth = LLC::SK256(vchPubKey);

                    /* Get the crypto register. */
                    TAO::Register::Object crypto;
                    if(!TAO::Register::GetNameRegister(hashGenesis, "crypto", crypto))
                        return debug::error(NODE, "authorization failed, missing crypto register");

                    /* Parse the object. */
                    if(!crypto.Parse())
                        return debug::error(NODE, "failed to parse crypto register");

                    /* Check the authorization hash. */
                    uint256_t hashCheck = crypto.get<uint256_t>("auth");
                    if(hashAuth != hashCheck)
                        return debug::error(NODE, "failed to authorize, invalid public key");

                    /* Get the signature. */
                    std::vector<uint8_t> vchSig;
                    ssPacket >> vchSig;

                    /* Switch based on signature type. */
                    switch(nType)
                    {
                        /* Support for the FALCON signature scheeme. */
                        case TAO::Ledger::SIGNATURE::FALCON:
                        {
                            /* Create the FL Key object. */
                            LLC::FLKey key;

                            /* Set the public key and verify. */
                            key.SetPubKey(vchPubKey);
                            if(!key.Verify(hashGenesis.GetBytes(), vchSig))
                                return debug::error(NODE, "invalid transaction signature");

                            break;
                        }

                        /* Support for the BRAINPOOL signature scheme. */
                        case TAO::Ledger::SIGNATURE::BRAINPOOL:
                        {
                            /* Create EC Key object. */
                            LLC::ECKey key = LLC::ECKey(LLC::BRAINPOOL_P512_T1, 64);

                            /* Set the public key and verify. */
                            key.SetPubKey(vchPubKey);
                            if(!key.Verify(hashGenesis.GetBytes(), vchSig))
                                return debug::error(NODE, "invalid transaction signature");

                            break;
                        }

                        default:
                            return debug::error(NODE, "invalid signature type");
                    }

                    /* Get the crypto register. */
                    TAO::Register::Object trust;
                    if(!TAO::Register::GetNameRegister(hashGenesis, "trust", trust))
                        return debug::error(NODE, "authorization failed, missing trust register");

                    /* Parse the object. */
                    if(!trust.Parse())
                        return debug::error(NODE, "failed to parse trust register");

                    /* Set the node's current trust score. */
                    nTrust = trust.get<uint64_t>("trust");

                    /* Set to authorized node if passed all cryptographic checks. */
                    fAuthorized = true;
                }

                break;
            }


            /* Handle for list command. */
            case ACTION::LIST:
            {
                break;
            }


            /* Handle for get command. */
            case ACTION::GET:
            {
                break;
            }


            /* Handle for notify command. */
            case ACTION::NOTIFY:
            {
                break;
            }


            /* Handle for ping command. */
            case ACTION::PING:
            {
                break;
            }
        }

        return true;
    }


    /*  Non-Blocking Packet reader to build a packet from TCP Connection.
     *  This keeps thread from spending too much time for each Connection. */
    void TritiumNode::ReadPacket()
    {
        if(!INCOMING.Complete())
        {
            /** Handle Reading Packet Length Header. **/
            if(INCOMING.IsNull() && Available() >= 10)
            {
                std::vector<uint8_t> BYTES(10, 0);
                if(Read(BYTES, 10) == 10)
                {
                    DataStream ssHeader(BYTES, SER_NETWORK, MIN_PROTO_VERSION);
                    ssHeader >> INCOMING;

                    Event(EVENT_HEADER);
                }
            }

            /** Handle Reading Packet Data. **/
            uint32_t nAvailable = Available();
            if(nAvailable > 0 && !INCOMING.IsNull() && INCOMING.DATA.size() < INCOMING.LENGTH)
            {
                /* Create the packet data object. */
                std::vector<uint8_t> DATA(std::min(nAvailable, (uint32_t)(INCOMING.LENGTH - INCOMING.DATA.size())), 0);

                /* Read up to 512 bytes of data. */
                if(Read(DATA, DATA.size()) == DATA.size())
                {
                    INCOMING.DATA.insert(INCOMING.DATA.end(), DATA.begin(), DATA.end());
                    Event(EVENT_PACKET, static_cast<uint32_t>(DATA.size()));
                }
            }
        }
    }

    /* Determine if a node is authorized and therfore trusted. */
    bool TritiumNode::Authorized() const
    {
        return hashGenesis != 0 && fAuthorized;
    }
}
