/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLP_INCLUDE_INV_H
#define NEXUS_LLP_INCLUDE_INV_H

#include <Util/templates/serialize.h>
#include <cstdint>

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
     *  Inventory message data.
     *
     *  Check the Inventory to See if a given hash is found.
     *
     **/
    class CInv
    {
    public:

        /** Default Constructor **/
        CInv();


        /** Constructor **/
        CInv(const uint1024_t& hashIn, const int32_t typeIn);


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


        /** Print
         *
         *  Prints this inventory data to the console window.
         *
         **/
        void Print() const;


        /** GetHash
         *
         *  Returns the hash associated with this inventory.
         *
         **/
        uint1024_t GetHash() const;


        /** SetHash
         *
         *  Sets the hash for this inventory.
         *
         *  @param[in] hashIn The 1024-bit hash to set.
         *
         **/
        void SetHash(const uint1024_t &hashIn);


        /** GetType
         *
         *  Returns the type of this inventory.
         *
         **/
        int32_t GetType() const;


        /** SetType
         *
         *  Sets the type for this inventory.
         *
         *  @param[in] typeIn The type to set.
         *
         **/
         void SetType(const int32_t typeIn);

    private:
        uint1024_t hash;
        int32_t type;
    };
}

#endif
