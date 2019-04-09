/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/keychain/filemap.h>
#include <LLD/templates/key.h>
#include <LLD/include/enum.h>
#include <LLD/include/version.h>

#include <Util/templates/datastream.h>
#include <Util/include/filesystem.h>
#include <Util/include/debug.h>
#include <Util/include/hex.h>

namespace LLD
{

    /** The Database Constructor. To determine file location and the Bytes per Record. **/
    BinaryFileMap::BinaryFileMap(std::string strBaseLocationIn, uint8_t nFlagsIn)
    : strBaseLocation(strBaseLocationIn)
    , nCurrentFileSize(0)
    , nCurrentFile(0)
    , nFlags(nFlagsIn)
    {
        Initialize();
    }


    /** Default Destructor **/
    BinaryFileMap::~BinaryFileMap()
    {
    }


    /** Copy assignment operator **/
    BinaryFileMap& BinaryFileMap::operator=(BinaryFileMap map)
    {
        strBaseLocation    = map.strBaseLocation;
        fMemoryCaching     = map.fMemoryCaching;
        nCacheSize         = map.nCacheSize;
        nCurrentFile       = map.nCurrentFile;
        nCurrentFileSize   = map.nCurrentFileSize;

        Initialize();

        return *this;
    }


    /** Copy Constructor **/
    BinaryFileMap::BinaryFileMap(const BinaryFileMap& map)
    {
        strBaseLocation    = map.strBaseLocation;
        fMemoryCaching     = map.fMemoryCaching;
        nCacheSize         = map.nCacheSize;
        nCurrentFileSize   = map.nCurrentFileSize;
        nCurrentFile       = map.nCurrentFile;


        Initialize();
    }


    /*  Return the Keys to the Records Held in the Database. */
    std::vector< std::vector<uint8_t> > BinaryFileMap::GetKeys() const
    {
        std::vector< std::vector<uint8_t> > vKeys;
        auto nIterator = mapKeys.begin();

        for(; nIterator != mapKeys.end(); ++nIterator)
            vKeys.push_back(nIterator->first);

        return vKeys;
    }

    /*  Determines if the database has the Key. */
    bool BinaryFileMap::HasKey(const std::vector<uint8_t>& vKey) const
    {
        return mapKeys.count(vKey);
    }


    /*  Read the Database Keys and File Positions. */
    void BinaryFileMap::Initialize()
    {
        LOCK(KEY_MUTEX);

        /* Create directories if they don't exist yet. */
        if(!filesystem::exists(strBaseLocation) && filesystem::create_directories(strBaseLocation))
            debug::log(0, FUNCTION, "Generated Path ", strBaseLocation);

        /* Stats variable for collective keychain size. */
        uint32_t nKeychainSize = 0;
        uint32_t nTotalKeys = 0;


        /* Iterate through the files detected. */
        while(true)
        {
            std::string strFilename = debug::safe_printstr(strBaseLocation, "_filemap.", std::setfill('0'), std::setw(5), nCurrentFile);

            /* Get the Filename at given File Position. */
            std::fstream fIncoming(strFilename.c_str(), std::ios::in | std::ios::binary);
            if(!fIncoming)
            {
                if(nCurrentFile > 0)
                    nCurrentFile --;
                else
                {
                    std::ofstream ssFile(strFilename.c_str(), std::ios::out | std::ios::binary);
                    ssFile.close();
                }

                break;
            }

            /* Get the Binary Size. */
            fIncoming.ignore(std::numeric_limits<std::streamsize>::max());
            nCurrentFileSize = static_cast<int32_t>(fIncoming.gcount());
            nKeychainSize += nCurrentFileSize;

            /* Read the keychain file. */
            fIncoming.seekg (0, std::ios::beg);
            std::vector<uint8_t> vKeychain(nCurrentFileSize, 0);
            fIncoming.read((char*) &vKeychain[0], vKeychain.size());
            fIncoming.close();


            /* Iterator for Key Sectors. */
            uint32_t nIterator = 0;
            while(nIterator < nCurrentFileSize)
            {

                /* Get Binary Data */
                std::vector<uint8_t> vData(vKeychain.begin() + nIterator, vKeychain.begin() + nIterator + 13);


                /* Read the State and Size of Sector Header. */
                SectorKey cKey;
                DataStream ssKey(vData, SER_LLD, DATABASE_VERSION);
                ssKey >> cKey;


                /* Skip Empty Sectors */
                if(cKey.Ready())
                {

                    /* Read the Key Data. */
                    std::vector<uint8_t> vKey(vKeychain.begin() + nIterator + 13, vKeychain.begin() + nIterator + 13 + cKey.nLength);

                    /* Set the Key Data. */
                    mapKeys[vKey] = std::make_pair(nCurrentFile, nIterator);

                    /* Debug Output of Sector Key Information. */
                    debug::log(5, FUNCTION, "State: ", cKey.nState, " Length: ", cKey.nLength, " File: ", mapKeys[vKey].first, " Location: ", mapKeys[vKey].second, " Key: ", HexStr(vKey.begin(), vKey.end()));

                    ++nTotalKeys;
                }
                else
                {

                    /* Debug Output of Sector Key Information. */
                    debug::log(5, FUNCTION, "Skipping Sector State: ", cKey.nState, " Length: ", cKey.nLength);
                }

                /* Increment the Iterator. */
                nIterator += cKey.Size();
            }

            /* Iterate the current file. */
            ++nCurrentFile;

            /* Clear the keychain data. */
            vKeychain.clear();
        }

        debug::log(0, FUNCTION, "Initialized with ", nTotalKeys, " Keys",
          " | Total Size ", nKeychainSize,
          " | Total Files ", nCurrentFile + 1,
          " | Current Size ", nCurrentFileSize);
    }


    /*  Add / Update A Record in the Database */
    bool BinaryFileMap::Put(const SectorKey &cKey) const
    {
        LOCK(KEY_MUTEX);

        /* Write Header if First Update. */
        if(!mapKeys.count(cKey.vKey))
        {
            /* Check the Binary File Size. */
            if(nCurrentFileSize > FILEMAP_MAX_FILE_SIZE)
            {
                debug::log(4, FUNCTION, "Current File too Large, allocating new File ", nCurrentFile + 1);

                ++nCurrentFile;
                nCurrentFileSize = 0;

                std::ofstream ssFile(debug::safe_printstr(strBaseLocation, "_filemap.", std::setfill('0'), std::setw(5), nCurrentFile), std::ios::out | std::ios::binary);
                ssFile.close();
            }

            mapKeys[cKey.vKey] = std::make_pair(nCurrentFile, nCurrentFileSize);

            /* Increment current File Size. */
            nCurrentFileSize += cKey.Size();
        }


        /* Establish the Outgoing Stream. */
        std::fstream ssFile(debug::safe_printstr(strBaseLocation, "_filemap.", std::setfill('0'), std::setw(5), mapKeys[cKey.vKey].first), std::ios::in | std::ios::out | std::ios::binary);


        /* Seek File Pointer */
        ssFile.seekp(mapKeys[cKey.vKey].second, std::ios::beg);


        /* Handle the Sector Key Serialization. */
        DataStream ssKey(SER_LLD, DATABASE_VERSION);
        ssKey.reserve(cKey.Size());
        ssKey << cKey;


        /* Write to Disk. */
        std::vector<uint8_t> vData(ssKey.begin(), ssKey.end());
        vData.insert(vData.end(), cKey.vKey.begin(), cKey.vKey.end());
        ssFile.write((char*) &vData[0], vData.size());
        ssFile.flush();


        /* Debug Output of Sector Key Information. */
        debug::log(4, FUNCTION, "State: ", cKey.nState == STATE::READY ? "Valid" : "Invalid",
            " | Length: ", cKey.nLength, " | Location: ", mapKeys[cKey.vKey].second,
            " | File: ", mapKeys[cKey.vKey].first, " | Sector File: ", cKey.nSectorFile,
            " | Sector Size: ", cKey.nSectorSize, " | Sector Start: ", cKey.nSectorStart,
            " | Key: ", HexStr(cKey.vKey.begin(), cKey.vKey.end()),
            " | Current File: ", nCurrentFile, " | Current File Size: ", nCurrentFileSize);


        return true;
    }


    /*  Simple Erase for now, not efficient in Data Usage of HD but quick
     *  to get erase function working. */
    bool BinaryFileMap::Erase(const std::vector<uint8_t> &vKey)
    {
        LOCK(KEY_MUTEX);

        /* Check for the Key. */
        if(!mapKeys.count(vKey))
            return debug::error(FUNCTION, "Key doesn't Exist");


        /* Establish the Outgoing Stream. */
        std::fstream ssFile(debug::safe_printstr(strBaseLocation, "_filemap.", std::setfill('0'), std::setw(5), mapKeys[vKey].first), std::ios::in | std::ios::out | std::ios::binary);


        /* Set to put at the right file and sector position. */
        ssFile.seekp(mapKeys[vKey].second, std::ios::beg);


        /* Establish the Sector State as Empty. */
        std::vector<uint8_t> vData(STATE::EMPTY);
        ssFile.write((char*) &vData[0], vData.size());
        ssFile.flush();


        /* Remove the Sector Key from the Memory Map. */
        mapKeys.erase(vKey);


        return true;
    }


    /*  Simple Erase for now, not efficient in Data Usage of HD but quick
     *  to get erase function working. */
    bool BinaryFileMap::Restore(const std::vector<uint8_t> &vKey)
    {
        LOCK(KEY_MUTEX);

        /* Check for the Key. */
        if(!mapKeys.count(vKey))
            return debug::error(FUNCTION, "Key doesn't Exist");


        /* Establish the Outgoing Stream. */
        std::string strFilename = debug::safe_printstr(strBaseLocation, "_filemap.", std::setfill('0'), std::setw(5), mapKeys[vKey].first);
        std::fstream ssFile(strFilename.c_str(), std::ios::in | std::ios::out | std::ios::binary);


        /* Set to put at the right file and sector position. */
        ssFile.seekp(mapKeys[vKey].second, std::ios::beg);


        /* Establish the Sector State as Empty. */
        std::vector<uint8_t> vData(STATE::READY);
        ssFile.write((char*) &vData[0], vData.size());
        ssFile.flush();


        /* Remove the Sector Key from the Memory Map. */
        mapKeys.erase(vKey);


        return true;
    }


    /*  Get a Record from the Database with Given Key. */
    bool BinaryFileMap::Get(const std::vector<uint8_t>& vKey, SectorKey& cKey)
    {
        LOCK(KEY_MUTEX);

        /* Read a Record from Binary Data. */
        if(mapKeys.count(vKey))
        {
            /* Open the Stream Object. */
            std::string strFilename = debug::safe_printstr(strBaseLocation, "_filemap.", std::setfill('0'), std::setw(5), mapKeys[vKey].first);
            std::ifstream ssFile(strFilename.c_str(), std::ios::in | std::ios::binary);


            /* Seek to the Sector Position on Disk. */
            ssFile.seekg(mapKeys[vKey].second, std::ios::beg);


            /* Read the State and Size of Sector Header. */
            std::vector<uint8_t> vData(13, 0);
            ssFile.read((char*) &vData[0], 13);


            /* De-serialize the Header. */
            DataStream ssHeader(vData, SER_LLD, DATABASE_VERSION);
            ssHeader >> cKey;


            /* Debug Output of Sector Key Information. */
            debug::log(4, FUNCTION, "State: ", cKey.nState == STATE::READY ? "Valid" : "Invalid",
                " | Length: ", cKey.nLength, " | Location: ", mapKeys[vKey].second,
                " | File: ",  mapKeys[vKey].first, " | Sector File: ", cKey.nSectorFile,
                " | Sector Size: ", cKey.nSectorSize, " | Sector Start: ", cKey.nSectorStart,
                " | Key: ",  HexStr(vKey.begin(), vKey.end()));


            /* Skip Empty Sectors for Now. (TODO: Expand to Reads / Writes) */
            if(cKey.Ready() || cKey.IsTxn())
            {

                /* Read the Key Data. */
                std::vector<uint8_t> vKeyIn(cKey.nLength, 0);
                ssFile.read((char*) &vKeyIn[0], vKeyIn.size());

                /* Check the Keys Match Properly. */
                if(vKeyIn != vKey)
                    return debug::error(FUNCTION, "Key Mistmatch: DB:: ", HexStr(vKeyIn.begin(), vKeyIn.end()), " MEM ", HexStr(vKey.begin(), vKey.end()));

                /* Assign Key to Sector. */
                cKey.vKey = vKeyIn;

                return true;
            }
        }

        return false;
    }


}
