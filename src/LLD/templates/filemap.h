/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_LLD_TEMPLATES_FILEMAP_H
#define NEXUS_LLD_TEMPLATES_FILEMAP_H

#include <LLD/templates/key.h>
#include <Util/include/filesystem.h>

namespace LLD
{

    /** Total buckets for memory accessing. */
    const uint32_t FILEMAP_TOTAL_BUCKETS = 256 * 256;

    /* Maximum size a file can be in the keychain. */
    const uint32_t FILEMAP_MAX_FILE_SIZE = 1024 * 1024 * 1024; //1 GB per File


    /** Binary File Map Database Class.
     *
        Stores and Contains the Sector Keys to Access the Sector Database.
    **/
    class BinaryFileMap
    {
    protected:
        /** Mutex for Thread Synchronization. **/
        mutable Mutex_t KEY_MUTEX;


        /** The String to hold the Disk Location of Database File.
            Each Database File Acts as a New Table as in Conventional Design.
            Key can be any Type, which is how the Database Records are Accessed. **/
        std::string strBaseLocation;

        /** Caching Flag
            TODO: Expand the Caching System. **/
        bool fMemoryCaching = false;

        /** Caching Size.
            TODO: Make this a variable actually enforced. **/
        uint32_t nCacheSize = 0;


        /* Use these for iterating file locations. */
        mutable uint16_t nCurrentFile;
        mutable uint32_t nCurrentFileSize;

        /* Hashmap Custom Hash Using SK. */
        struct SK_Hashmap
        {
            std::size_t operator()(const std::vector<uint8_t>& k) const {
                return LLC::SK32(k);
            }
        };

    public:
        /** Map to Contain the Binary Positions of Each Key.
            Used to Quickly Read the Database File at Given Position
            To Obtain the Record from its Database Key. This is Read
            Into Memory on Database Initialization. **/
        mutable typename std::map< std::vector<uint8_t>, std::pair<uint16_t, uint32_t> > mapKeys[FILEMAP_TOTAL_BUCKETS];


        /** Caching Memory Map. Keep within caching limits. Pop on and off like a stack
            using seperate caching class if over limits. **/
        mutable typename std::map< std::vector<uint8_t>, SectorKey > mapKeysCache[FILEMAP_TOTAL_BUCKETS];


        /** The Database Constructor. To determine file location and the Bytes per Record. **/
        BinaryFileMap(std::string strBaseLocationIn) : strBaseLocation(strBaseLocationIn), nCurrentFile(0), nCurrentFileSize(0)
        {
            Initialize();
        }


        /** Clean up Memory Usage. **/
        ~BinaryFileMap() {}


        BinaryFileMap& operator=(BinaryFileMap map)
        {
            //KEY_MUTEX = map.KEY_MUTEX;

            strBaseLocation    = map.strBaseLocation;
            fMemoryCaching     = map.fMemoryCaching;
            nCacheSize         = map.nCacheSize;
            nCurrentFile       = map.nCurrentFile;
            nCurrentFileSize   = map.nCurrentFileSize;

            Initialize();

            return *this;
        }


        BinaryFileMap(const BinaryFileMap& map)
        {
            //KEY_MUTEX = map.KEY_MUTEX;

            strBaseLocation    = map.strBaseLocation;
            fMemoryCaching     = map.fMemoryCaching;
            nCacheSize         = map.nCacheSize;
            nCurrentFile       = map.nCurrentFile;
            nCurrentFileSize   = map.nCurrentFileSize;

            Initialize();
        }


        /** Return the Keys to the Records Held in the Database. **/
        std::vector< std::vector<uint8_t> > GetKeys()
        {
            std::vector< std::vector<uint8_t> > vKeys;
            for(int i = 0; i < FILEMAP_TOTAL_BUCKETS; i++)
                for(typename std::map< std::vector<uint8_t>, std::pair<uint16_t, uint32_t> >::iterator nIterator = mapKeys[i].begin(); nIterator != mapKeys[i].end(); nIterator++ )
                    vKeys.push_back(nIterator->first);

            return vKeys;
        }


        /** Return Whether a Key Exists in the Database. **/
        bool HasKey(const std::vector<uint8_t>& vKey) { return mapKeys[GetBucket(vKey)].count(vKey); }


        /** Handle the Assigning of a Map Bucket. **/
        uint32_t GetBucket(const std::vector<uint8_t>& vKey) const
        {
            uint64_t nBucket = 0;
            for(int i = 0; i < vKey.size() && i < 8; i++)
                nBucket += vKey[i] << (8 * i);

            return nBucket % FILEMAP_TOTAL_BUCKETS;
        }


        /** Read the Database Keys and File Positions. **/
        void Initialize()
        {
            LOCK(KEY_MUTEX);

            /* Create directories if they don't exist yet. */
            if(create_directory(strBaseLocation))
                printf(FUNCTION "Generated Path %s\n", __PRETTY_FUNCTION__, strBaseLocation.c_str());

            /* Stats variable for collective keychain size. */
            uint32_t nKeychainSize = 0, nTotalKeys = 0;


            /* Iterate through the files detected. */
            while(true)
            {
                std::string strFilename = strprintf("%s_filemap.%05u", strBaseLocation.c_str(), nCurrentFile);
                printf(FUNCTION "Checking File %s\n", __PRETTY_FUNCTION__, strFilename.c_str());

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
                nCurrentFileSize = fIncoming.gcount();
                nKeychainSize += nCurrentFileSize;

                printf(FUNCTION "Keychain File %u Loading [%u bytes]...\n", __PRETTY_FUNCTION__, nCurrentFile, nCurrentFileSize);


                fIncoming.seekg (0, std::ios::beg);
                std::vector<uint8_t> vKeychain(nCurrentFileSize, 0);
                fIncoming.read((char*) &vKeychain[0], vKeychain.size());
                fIncoming.close();


                /* Iterator for Key Sectors. */
                uint32_t nIterator = 0;
                while(nIterator < nCurrentFileSize)
                {

                    /* Get Binary Data */
                    std::vector<uint8_t> vKey(vKeychain.begin() + nIterator, vKeychain.begin() + nIterator + 15);


                    /* Read the State and Size of Sector Header. */
                    SectorKey cKey;
                    CDataStream ssKey(vKey, SER_LLD, DATABASE_VERSION);
                    ssKey >> cKey;


                    /* Skip Empty Sectors for Now.
                        TODO: Handle any sector and keys gracfully here to ensure that the Sector is returned to a valid state from the transaction journal in case there was a failure reading and writing in the sector. This will most likely be held in the sector database code. */
                    if(cKey.Ready())
                    {

                        /* Read the Key Data. */
                        std::vector<uint8_t> vKey(vKeychain.begin() + nIterator + 15, vKeychain.begin() + nIterator + 15 + cKey.nLength);

                        /* Set the Key Data. */
                        uint32_t nBucket = GetBucket(vKey);
                        mapKeys[nBucket][vKey] = std::make_pair(nCurrentFile, nIterator);
                        //mapKeysCache[nBucket][vKey] = cKey;

                        /* Debug Output of Sector Key Information. */
                        if(GetArg("-verbose", 0) >= 5)
                            printf(FUNCTION "State: %u Length: %u File: %u Location: %u Key: %s\n", __PRETTY_FUNCTION__, cKey.nState, cKey.nLength, mapKeys[nBucket][vKey].first, mapKeys[nBucket][vKey].second, HexStr(vKey.begin(), vKey.end()).c_str());

                        nTotalKeys++;
                    }
                    else
                    {

                        /* Debug Output of Sector Key Information. */
                        if(GetArg("-verbose", 0) >= 5)
                            printf(FUNCTION "Skipping Sector State: %u Length: %u\n", __PRETTY_FUNCTION__, cKey.nState, cKey.nLength);
                    }

                    /* Increment the Iterator. */
                    nIterator += cKey.Size();
                }

                /* Iterate the current file. */
                nCurrentFile++;

                /* Clear the keychain data. */
                vKeychain.clear();
            }

            printf(FUNCTION "Initialized with %u Keys | Total Size %u | Total Files %u | Current Size %u\n", __PRETTY_FUNCTION__, nTotalKeys, nKeychainSize, nCurrentFile + 1, nCurrentFileSize);
        }

        /** Add / Update A Record in the Database **/
        bool Put(SectorKey cKey) const
        {
            LOCK(KEY_MUTEX);

            /* Write Header if First Update. */
            uint32_t nBucket = GetBucket(cKey.vKey);
            if(!mapKeys[nBucket].count(cKey.vKey))
            {
                /* Check the Binary File Size. */
                if(nCurrentFileSize > FILEMAP_MAX_FILE_SIZE)
                {
                    if(GetArg("-verbose", 0) >= 4)
                        printf(FUNCTION "Current File too Large, allocating new File %u\n", __PRETTY_FUNCTION__, nCurrentFileSize, nCurrentFile + 1);

                    nCurrentFile ++;
                    nCurrentFileSize = 0;

                    std::ofstream ssFile(strprintf("%s_filemap.%05u", strBaseLocation.c_str(), nCurrentFile).c_str(), std::ios::out | std::ios::binary);
                    ssFile.close();
                }

                mapKeys[nBucket][cKey.vKey] = std::make_pair(nCurrentFile, nCurrentFileSize);
            }


            /* Establish the Outgoing Stream. */
            std::fstream ssFile(strprintf("%s_filemap.%05u", strBaseLocation.c_str(), mapKeys[nBucket][cKey.vKey].first).c_str(), std::ios::in | std::ios::out | std::ios::binary);


            /* Seek File Pointer */
            ssFile.seekp(mapKeys[nBucket][cKey.vKey].second, std::ios::beg);


            /* Handle the Sector Key Serialization. */
            CDataStream ssKey(SER_LLD, DATABASE_VERSION);
            ssKey.reserve(cKey.Size());
            ssKey << cKey;


            /* Write to Disk. */
            std::vector<uint8_t> vData(ssKey.begin(), ssKey.end());
            vData.insert(vData.end(), cKey.vKey.begin(), cKey.vKey.end());
            ssFile.write((char*) &vData[0], vData.size());

            /* Increment current File Size. */
            nCurrentFileSize += cKey.Size();


            /* Debug Output of Sector Key Information. */
            if(GetArg("-verbose", 0) >= 4)
                printf(FUNCTION "State: %s | Length: %u | Location: %u | File: %u | Sector File: %u | Sector Size: %u | Sector Start: %u | Key: %s | Current File: %u | Current File Size: %u\n", __PRETTY_FUNCTION__, cKey.nState == READY ? "Valid" : "Invalid", cKey.nLength, mapKeys[nBucket][cKey.vKey].second, mapKeys[nBucket][cKey.vKey].first, cKey.nSectorFile, cKey.nSectorSize, cKey.nSectorStart, HexStr(cKey.vKey.begin(), cKey.vKey.end()).c_str(), nCurrentFile, nCurrentFileSize);


            return true;
        }

        /** Simple Erase for now, not efficient in Data Usage of HD but quick to get erase function working. **/
        bool Erase(const std::vector<uint8_t> vKey)
        {
            LOCK(KEY_MUTEX);

            /* Check for the Key. */
            uint32_t nBucket = GetBucket(vKey);
            if(!mapKeys[nBucket].count(vKey))
                return error(FUNCTION "Key doesn't Exist", __PRETTY_FUNCTION__);


            /* Establish the Outgoing Stream. */
            std::string strFilename = strprintf("%s_filemap.%05u", strBaseLocation.c_str(), mapKeys[nBucket][vKey].first);
            std::fstream ssFile(strFilename.c_str(), std::ios::in | std::ios::out | std::ios::binary);


            /* Set to put at the right file and sector position. */
            ssFile.seekp(mapKeys[nBucket][vKey].second, std::ios::beg);


            /* Establish the Sector State as Empty. */
            std::vector<uint8_t> vData(1, EMPTY);
            ssFile.write((char*) &vData[0], vData.size());


            /* Remove the Sector Key from the Memory Map. */
            mapKeys[nBucket].erase(vKey);


            return true;
        }

        /** Get a Record from the Database with Given Key. **/
        bool Get(const std::vector<uint8_t> vKey, SectorKey& cKey)
        {
            LOCK(KEY_MUTEX);


            /* Check the Memory Cache. */
            uint32_t nBucket = GetBucket(vKey);
            if(mapKeysCache[nBucket].count(vKey)) {
                cKey = mapKeysCache[nBucket][vKey];

                return true;
            }


            /* Read a Record from Binary Data. */
            if(mapKeys[nBucket].count(vKey))
            {

                /* Open the Stream Object. */
                std::string strFilename = strprintf("%s_filemap.%05u", strBaseLocation.c_str(), mapKeys[nBucket][vKey].first);
                std::ifstream ssFile(strFilename.c_str(), std::ios::in | std::ios::binary);


                /* Seek to the Sector Position on Disk. */
                ssFile.seekg(mapKeys[nBucket][vKey].second, std::ios::beg);


                /* Read the State and Size of Sector Header. */
                std::vector<uint8_t> vData(15, 0);
                ssFile.read((char*) &vData[0], 15);


                /* De-serialize the Header. */
                CDataStream ssHeader(vData, SER_LLD, DATABASE_VERSION);
                ssHeader >> cKey;


                /* Debug Output of Sector Key Information. */
                if(GetArg("-verbose", 0) >= 4)
                    printf(FUNCTION "State: %s | Length: %u | Location: %u | File: %u | Sector File: %u | Sector Size: %u | Sector Start: %u | Key: %s\n", __PRETTY_FUNCTION__, cKey.nState == READY ? "Valid" : "Invalid", cKey.nLength, mapKeys[nBucket][vKey].second, mapKeys[nBucket][vKey].first, cKey.nSectorFile, cKey.nSectorSize, cKey.nSectorStart, HexStr(vKey.begin(), vKey.end()).c_str());


                /* Skip Empty Sectors for Now. (TODO: Expand to Reads / Writes) */
                if(cKey.Ready() || cKey.IsTxn()) {

                    /* Read the Key Data. */
                    std::vector<uint8_t> vKeyIn(cKey.nLength, 0);
                    ssFile.read((char*) &vKeyIn[0], vKeyIn.size());

                    /* Check the Keys Match Properly. */
                    if(vKeyIn != vKey)
                        return error(FUNCTION "Key Mistmatch: DB:: %s MEM %s\n", __PRETTY_FUNCTION__, HexStr(vKeyIn.begin(), vKeyIn.end()).c_str(), HexStr(vKey.begin(), vKey.end()).c_str());

                    /* Assign Key to Sector. */
                    cKey.vKey = vKeyIn;

                    return true;
                }

            }

            return false;
        }
    };
}

#endif
