/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLP_INCLUDE_LEGACY_ADDRESS_H
#define NEXUS_LLP_INCLUDE_LEGACY_ADDRESS_H

#include <LLP/include/base_address.h>
#include <Util/templates/serialize.h>
#include <cstdint>

namespace LLP
{
    /* Services flags */
    enum
    {
        NODE_NETWORK = (1 << 0),
    };


    /** LegacyAddress
     *
     *  An Address with information about it as peer (Legacy)
     *
     **/
    class LegacyAddress : public BaseAddress
    {
    public:

        /* Default constructor */
        LegacyAddress();

        /* Copy constructors */
        LegacyAddress(const LegacyAddress &other);
        LegacyAddress(const BaseAddress &ipIn, uint64_t nServicesIn = NODE_NETWORK);

        /* Default destructor */
        virtual ~LegacyAddress();

        /* Copy assignment operator */
        LegacyAddress &operator=(const LegacyAddress &other);

        /* Serialization */
        IMPLEMENT_SERIALIZE
        (
            LegacyAddress* pthis = const_cast<LegacyAddress*>(this);
            BaseAddress* pip = (BaseAddress*)pthis;

            if (fRead)
            {
                pthis->nServices = NODE_NETWORK;
                pthis->nTime = 100000000;
                pthis->nLastTry = 0;
            }

            if (nSerType & SER_DISK)
                READWRITE(nSerVersion);

            if ((nSerType & SER_DISK) || (!(nSerType & SER_GETHASH)))
                READWRITE(nTime);

            READWRITE(nServices);
            READWRITE(*pip);
        )


        /** Print
         *
         *  Prints information about this address.
         *
         **/
        virtual void Print();

    protected:

        int64_t nLastTry;   // memory only
        uint64_t nServices;
        uint32_t nTime;     // disk and network only

    };
}

#endif
