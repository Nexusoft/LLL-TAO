/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2021

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To The Voice of The People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLP_INCLUDE_VERSION_H
#define NEXUS_LLP_INCLUDE_VERSION_H

#include <string>
#include <Util/include/debug.h>

namespace LLP
{

    /* The current Protocol Version. */
    const uint32_t PROTOCOL_MAJOR     = 3;
    const uint32_t PROTOCOL_MINOR     = 5;
    const uint32_t PROTOCOL_REVISION  = 0;
    const uint32_t PROTOCOL_BUILD     = 0;


    /* Used to determine the features available in the Nexus Network */
    const uint32_t PROTOCOL_VERSION =
                       1000000 * PROTOCOL_MAJOR
                     +   10000 * PROTOCOL_MINOR
                     +     100 * PROTOCOL_REVISION
                     +       1 * PROTOCOL_BUILD;


    /* Used to Lock-Out Nodes that are running a protocol version that is too old, */
    const uint32_t MIN_PROTO_VERSION = 20000;


    /* Used to define the baseline of Tritium Version. */
    const uint32_t MIN_TRITIUM_VERSION = 3000000;


    /* Used to define the baseline of -client mode Version. */
    const uint32_t MIN_TRITIUM_CLIENT_VERSION = 3040000;


    /* The name that will be shared with other nodes. */
    const std::string strProtocolName = "Tritium";


    /* Track the current minor version in whole inrements for checking on the network. */
    inline uint16_t MajorVersion(const uint32_t nProtocolVersion = PROTOCOL_VERSION)
    {
        return (nProtocolVersion / 1000000);
    }


    /* Track the current minor version in whole inrements for checking on the network. */
    inline int16_t MinorVersion(const uint32_t nProtocolVersion, const uint32_t nMajor)
    {
        /* Find our base major version number. */
        uint32_t nVersionMajor = (nMajor * 1000000);
        if(nVersionMajor > nProtocolVersion)
            return int16_t(-1); //we return -1 here if below current major protocol version

        return ((nProtocolVersion - nVersionMajor) / 10000);
    }


    //TODO: we want to add a version filtering system into TritiumNode that allows designated messages based on version.
    //this will allow us to expand the messaging protocol much easier, this method above is one step towards this aim.
    //this could be a static table, where each message enum has a version number its allowed on. This could also be added
    //to our flags inside packet header to reject messages that don't mask to the correct flag.
}

#endif
