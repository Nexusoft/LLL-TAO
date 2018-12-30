/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_LLD_TEMPLATES_KEY_H
#define NEXUS_LLD_TEMPLATES_KEY_H

#include <fstream>

#include <LLD/include/version.h>

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
    /** Enumeration for each State.
        Allows better thread concurrency
        Allow Reads on READY and READ.

        Only Flush to Database if not Cached. (TODO) **/
    enum STATE
    {
        EMPTY 			= 0,
        READY 			= 1,
        TRANSACTION     = 2
    };


    /** Key Class to Hold the Location of Sectors it is referencing.
        This Indexes the Sector Database. **/
    class SectorKey
    {
    public:

        /** The Key Header:
            Byte 0: nState
            Byte 1 - 3: nLength (The Size of the Sector)
            Byte 3 - 5: nSector (The Sector Number [0 - x])
        **/
        uint8_t   		   	    nState;
        uint16_t 			    nLength;

        /** These three hold the location of
            Sector in the Sector Database of
            Given Sector Key. **/
        uint16_t 			   nSectorFile;
        uint16_t   		       nSectorSize;
        uint32_t   			   nSectorStart;

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

        /* Constructors. */
        SectorKey()
        : nState(0)
        , nLength(0)
        , nSectorFile(0)
        , nSectorSize(0)
        , nSectorStart(0) { }

        SectorKey(uint8_t nStateIn, std::vector<uint8_t> vKeyIn,
                  uint16_t nSectorFileIn, uint32_t nSectorStartIn, uint16_t nSectorSizeIn)
        : nState(nStateIn)
        , nSectorFile(nSectorFileIn)
        , nSectorSize(nSectorSizeIn)
        , nSectorStart(nSectorStartIn)
        {
            nLength = vKeyIn.size();
            vKey    = vKeyIn;
        }

        ~SectorKey()
        {

        }

        SectorKey& operator=(SectorKey key)
        {
            nState          = key.nState;
            nLength         = key.nLength;
            nSectorFile     = key.nSectorFile;
            nSectorSize     = key.nSectorSize;
            nSectorStart    = key.nSectorStart;
            vKey            = key.vKey;
        }

        SectorKey(const SectorKey& key)
        {
            nState          = key.nState;
            nLength         = key.nLength;
            nSectorFile     = key.nSectorFile;
            nSectorSize     = key.nSectorSize;
            nSectorStart    = key.nSectorStart;
            vKey            = key.vKey;
        }

        /* Iterator to the beginning of the raw key. */
        uint32_t Begin() { return 11; }


        /* Return the Size of the Key Sector on Disk. */
        uint32_t Size() { return (11 + nLength); }


        /* Dump Key to Debug Console. */
        void Print()
        {
            debug::log(0, "SectorKey(nState=%u, nLength=%u, nSectorFile=%u, nSectorSize=%u, nSectorStart=%u)",
            nState, nLength, nSectorFile, nSectorSize, nSectorStart);
        }


        /* Check for Key Activity on Sector. */
        bool Empty() { return (nState == EMPTY); }
        bool Ready() { return (nState == READY); }
        bool IsTxn() { return (nState == TRANSACTION); }

    };
}

#endif
