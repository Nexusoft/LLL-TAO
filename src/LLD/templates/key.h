/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLD_TEMPLATES_KEY_H
#define NEXUS_LLD_TEMPLATES_KEY_H

#include <Util/templates/serialize.h>

#include <cstdint>
#include <vector>


namespace LLD
{

    /** SectorKey
     *
     *  Key Class to Hold the Location of Sectors it is referencing.
     *  This Indexes the Sector Database.
     *
     **/
    class SectorKey
    {
    public:

        /** The current state of the sector (eg. WRITE, EMTPY, READ). **/
        uint8_t   		   	nState;


        /** The length of the sector key on disk. **/
        uint16_t 		    nLength;


        /** The file number that data is stored in. **/
        uint16_t 		   nSectorFile;


        /** The size of the sector data. **/
        uint32_t   		   nSectorSize;


        /** The binary position of the sector. **/
        uint32_t   		   nSectorStart;


        /** The binary data of the Sector key. **/
        std::vector<uint8_t> vKey;


        /** MEMORY ONLY: The timestamp for timestamped hash tables. **/
        mutable uint64_t nTimestamp;


        /* Serialization Macro. */
        IMPLEMENT_SERIALIZE
        (
            READWRITE(nState);
            READWRITE(nLength);
            READWRITE(nSectorFile);
            READWRITE(nSectorSize);
            READWRITE(nSectorStart);
        )


        /** Default Constructor. **/
        SectorKey();


        /** Copy Constructor **/
        SectorKey(const SectorKey& key);


        /** Move Constructor **/
        SectorKey(SectorKey&& key) noexcept;


        /** Copy Assignment Operator **/
        SectorKey& operator=(const SectorKey& key);


        /** Move Assignment Operator **/
        SectorKey& operator=(SectorKey&& key) noexcept;


        /** Default Destructor **/
        ~SectorKey();


        /** Constructor **/
        SectorKey(const uint8_t nStateIn, const std::vector<uint8_t>& vKeyIn,
                  const uint16_t nSectorFileIn, const uint32_t nSectorStartIn, const uint32_t nSectorSizeIn);


        /** SetKey
         *
         *  Set the key for this object.
         *
         *  @param[in] vKeyIn The key to set.
         *
         **/
        void SetKey(const std::vector<uint8_t>& vKeyIn);


        /** Begin
         *
         *  Iterator to the beginning of the raw key.
         *
         **/
        uint32_t Begin() const;


        /** Size
         *
         *  Return the size of the key sector on disk.
         *
         **/
        uint32_t Size() const;


        /** Print
         *
         *  Dump Key to Debug Console.
         *
         **/
        void Print() const;


        /** Empty
         *
         *  Determines if the key is in an empty state.
         *
         **/
        bool Empty() const;


        /** Ready
         *
         *  Determines if the key is in a ready state.
         *
         **/
        bool Ready() const;


        /** IsTxn
         *
         *  Determines if the key is in a transaction state.
         *
         **/
        bool IsTxn() const;

    };
}

#endif
