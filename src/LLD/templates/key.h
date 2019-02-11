/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_LLD_TEMPLATES_KEY_H
#define NEXUS_LLD_TEMPLATES_KEY_H

#include <fstream>

#include <LLD/include/version.h>
#include <LLD/include/enum.h>

#include <LLC/hash/SK.h>
#include <LLC/hash/macro.h>
#include <LLC/types/bignum.h>

#include <Util/include/args.h>
#include <Util/include/config.h>
#include <Util/include/mutex.h>
#include <Util/include/hex.h>
#include <Util/templates/serialize.h>


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

        /** The Key Header:
            Byte 0: nState
            Byte 1 - 3: nLength (The Size of the Sector)
            Byte 3 - 5: nSector (The Sector Number [0 - x])
        **/
        uint8_t   		   	nState;
        uint16_t 			    nLength;

        /** These three hold the location of
            Sector in the Sector Database of
            Given Sector Key. **/
        uint16_t 			   nSectorFile;
        uint32_t   		   nSectorSize;
        uint32_t   			 nSectorStart;

        /* The binary data of the Sector key. */
        std::vector<uint8_t> vKey;


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
        SectorKey()
        : nState(0)
        , nLength(0)
        , nSectorFile(0)
        , nSectorSize(0)
        , nSectorStart(0) { }


        /** Constructor **/
        SectorKey(uint8_t nStateIn, std::vector<uint8_t> vKeyIn,
                  uint16_t nSectorFileIn, uint32_t nSectorStartIn, uint32_t nSectorSizeIn)
        : nState(nStateIn)
        , nLength(vKeyIn.size())
        , nSectorFile(nSectorFileIn)
        , nSectorSize(nSectorSizeIn)
        , nSectorStart(nSectorStartIn)
        , vKey(vKeyIn)
        {
        }


        /** Default Destructor **/
        ~SectorKey()
        {

        }


        /** Copy Assignment Operator **/
        SectorKey& operator=(const SectorKey& key)
        {
            nState          = key.nState;
            nLength         = key.nLength;
            nSectorFile     = key.nSectorFile;
            nSectorSize     = key.nSectorSize;
            nSectorStart    = key.nSectorStart;
            vKey            = key.vKey;

            return *this;
        }


        /** Default Copy Constructor **/
        SectorKey(const SectorKey& key)
        {
            nState          = key.nState;
            nLength         = key.nLength;
            nSectorFile     = key.nSectorFile;
            nSectorSize     = key.nSectorSize;
            nSectorStart    = key.nSectorStart;
            vKey            = key.vKey;
        }


        /** SetKey
         *
         *  Set the key for this object.
         *
         *  @param[in] vKeyIn The key to set.
         *
         **/
        void SetKey(const std::vector<uint8_t>& vKeyIn)
        {
            vKey = vKeyIn;
            nLength = vKey.size();
        }


        /** Begin
         *
         *  Iterator to the beginning of the raw key.
         *
         **/
        inline uint32_t Begin() const
        {
            return 13;
        }


        /** Size
         *
         *  Return the size of the key sector on disk.
         *
         **/
        inline uint32_t Size() const { return (13 + nLength); }


        /** Print
         *
         *  Dump Key to Debug Console.
         *
         **/
        void Print() const
        {
            debug::log(0,
                "SectorKey(nState=", (uint32_t)nState,
                ", nLength=", nLength,
                ", nSectorFile=", nSectorFile,
                ", nSectorSize=", nSectorSize,
                ", nSectorStart=", nSectorStart,")");
        }


        /** Empty
         *
         *  Determines if the key is in an empty state.
         *
         **/
        inline bool Empty() const
        {
            return (nState == STATE::EMPTY);
        }


        /** Ready
         *
         *  Determines if the key is in a ready state.
         *
         **/
        inline bool Ready() const
        {
            return (nState == STATE::READY);
        }


        /** IsTxn
         *
         *  Determines if the key is in a transaction state.
         *
         **/
        inline bool IsTxn() const
        {
            return (nState == STATE::TRANSACTION);
        }

    };
}

#endif
