/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_LLP_INCLUDE_INV_H
#define NEXUS_LLP_INCLUDE_INV_H

#include <Util/templates/serialize.h>

namespace LLP
{

    /* Inventory Block Messages Switch. */
    enum
    {
        MSG_TX = 1,
        MSG_BLOCK,
    };


    /** CInv
     *
     *  Inv message data.
     *
     **/
    class CInv
    {
        public:

            /** Default Constructor **/
            CInv();

            /** Constructor **/
            CInv(int typeIn, const uint1024_t& hashIn);

            /** Constructor **/
            CInv(const std::string& strType, const uint1024_t& hashIn);


            /** Serialization **/
            IMPLEMENT_SERIALIZE
            (
                READWRITE(type);
                READWRITE(hash);
            )

            /** Relational operator less than **/
            friend bool operator<(const CInv& a, const CInv& b);

            /** IsKnownType
             *
             *  Determines if this inventory a known type.
             *
             **/
            bool IsKnownType() const;

            /** GetCommand
             *
             *  Returns a command from this inventory object.
             *
             **/
            const char* GetCommand() const;

            /** ToString
             *
             *  Returns data about this inventory as a string object.
             *
             **/
            std::string ToString() const;

            /** print
             *
             *  Prints this inventory data to the console window.
             *
             **/
            void print() const;

        // TODO: make private (improves encapsulation)
        public:
            int type;
            uint1024_t hash;
    };
}

#endif
