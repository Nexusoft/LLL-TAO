/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_LLP_INCLUDE_INV_H
#define NEXUS_LLP_INCLUDE_INV_H

#include"../../Util/templates/serialize.h"

namespace LLP
{

    /* Inventory Block Messages Switch. */
    enum
    {
        MSG_TX = 1,
        MSG_BLOCK,
    };


    /** inv message data */
    class CInv
    {
        public:
            CInv();
            CInv(int typeIn, const uint1024_t& hashIn);
            CInv(const std::string& strType, const uint1024_t& hashIn);

            IMPLEMENT_SERIALIZE
            (
                READWRITE(type);
                READWRITE(hash);
            )

            friend bool operator<(const CInv& a, const CInv& b);

            bool IsKnownType() const;
            const char* GetCommand() const;
            std::string ToString() const;
            void print() const;

        // TODO: make private (improves encapsulation)
        public:
            int type;
            uint1024_t hash;
    };
}

#endif
