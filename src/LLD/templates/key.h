/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2017] ++

            (c) Copyright The Nexus Developers 2014 - 2017

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "fides in stellis, virtus in numeris" - Faith in the Stars, Power in Numbers

____________________________________________________________________________________________*/

#ifndef NEXUS_LLD_TEMPLATES_KEY_H
#define NEXUS_LLD_TEMPLATES_KEY_H

#include <fstream>

#include "version.h"

#include "../../LLC/hash/SK.h"
#include "../../LLC/hash/macro.h"
#include "../../LLC/types/bignum.h"

#include "../../Util/include/args.h"
#include "../../Util/include/config.h"
#include "../../Util/include/mutex.h"
#include "../../Util/include/hex.h"
#include "../../Util/templates/serialize.h"


namespace LLD
{
    /** Enumeration for each State.
        Allows better thread concurrency
        Allow Reads on READY and READ.

        Only Flush to Database if not Cached. (TODO) **/
    enum
    {
        EMPTY 			= 0,
        READ  			= 1,
        WRITE 			= 2,
        READY 			= 3,
        TRANSACTION     = 4
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
        unsigned char   		   	nState;
        unsigned short 			   nLength;

        /** These three hold the location of
            Sector in the Sector Database of
            Given Sector Key. **/
        unsigned short 			   nSectorFile;
        unsigned short   		   nSectorSize;
        unsigned int   			   nSectorStart;

        /* The binary data of the Sector key. */
        std::vector<unsigned char> vKey;

        /** Checksum of Original Data to ensure no database corrupted sectors.
            TODO: Consider the Original Data from a checksum.
            When transactions implemented have transactions stored in a sector Database.
            Ensure Original keychain and new keychain are stored on disk.
            If there is failure here before it is commited to the main database it will only
            corrupt the database backups. This will ensure the core database never gets corrupted.
            On startup ensure that the checksums match to ensure that the database was not stopped
            in the middle of a write. **/
        unsigned int nChecksum;

        /* Serialization Macro. */
        IMPLEMENT_SERIALIZE
        (
            READWRITE(nState);
            READWRITE(nLength);
            READWRITE(nSectorFile);
            READWRITE(nSectorSize);
            READWRITE(nSectorStart);
            READWRITE(nChecksum);
        )

        /* Constructors. */
        SectorKey() : nState(0), nLength(0), nSectorFile(0), nSectorSize(0), nSectorStart(0) { }
        SectorKey(unsigned char nStateIn, std::vector<unsigned char> vKeyIn, unsigned short nSectorFileIn, unsigned int nSectorStartIn, unsigned short nSectorSizeIn) : nState(nStateIn), nSectorFile(nSectorFileIn), nSectorSize(nSectorSizeIn), nSectorStart(nSectorStartIn)
        {
            nLength = vKeyIn.size();
            vKey    = vKeyIn;
        }


        /* Iterator to the beginning of the raw key. */
        unsigned int Begin() { return 15; }


        /* Return the Size of the Key Sector on Disk. */
        unsigned int Size() { return (15 + nLength); }


        /* Dump Key to Debug Console. */
        void Print() { printf("SectorKey(nState=%u, nLength=%u, nSectorFile=%u, nSectorSize=%u, nSectorStart=%u, nChecksum=%" PRIu64 ")\n", nState, nLength, nSectorFile, nSectorSize, nSectorStart, nChecksum); }


        /* Check for Key Activity on Sector. */
        bool Empty() { return (nState == EMPTY); }
        bool Ready() { return (nState == READY); }
        bool IsTxn() { return (nState == TRANSACTION); }

    };
}

#endif
