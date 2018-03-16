/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2017] ++
            
            (c) Copyright The Nexus Developers 2014 - 2017
            
            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.
            
            "fides in stellis, virtus in numeris" - Faith in the Stars, Power in Numbers

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
            CInv(int typeIn, const uint1024& hashIn);
            CInv(const std::string& strType, const uint1024& hashIn);

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
            uint1024 hash;
    };
}

#endif
