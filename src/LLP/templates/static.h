/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2023

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#include <LLP/templates/base_connection.h>
#include <LLP/packets/message.h>
#include <LLP/packets/packet.h>
#include <LLP/packets/http.h>

namespace LLP
{

    /* Forward Declared Static Templates. */
    template<> std::atomic<uint64_t> BaseConnection<MessagePacket>::REQUESTS;
    template<> std::atomic<uint64_t> BaseConnection<MessagePacket>::PACKETS;
    template<> std::atomic<uint64_t> BaseConnection<Packet>::REQUESTS;
    template<> std::atomic<uint64_t> BaseConnection<Packet>::PACKETS;
    template<> std::atomic<uint64_t> BaseConnection<HTTPPacket>::REQUESTS;
    template<> std::atomic<uint64_t> BaseConnection<HTTPPacket>::PACKETS;

}
