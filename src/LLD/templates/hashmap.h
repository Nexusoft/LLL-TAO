/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_LLD_TEMPLATES_HASHMAP_H
#define NEXUS_LLD_TEMPLATES_HASHMAP_H

#include <LLD/templates/key.h>

namespace LLD
{

    /** The Maximum buckets allowed in the hashmap. */
    const uint32_t HASHMAP_TOTAL_BUCKETS = 256 * 256;


    /** The Maximum cache size for allocated keys. **/
    const uint32_t HASHMAP_MAX_CACHE_SZIE = 1024 * 1024; //1 MB


    /** Base Key Database Class.
        Stores and Contains the Sector Keys to Access the
        Sector Database.
    **/
    class BinaryHashMap
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


        /** Cache of records searched with find. **/
        mutable std::map<std::vector<uint8_t>, uint32_t> mapBinaryIterators;

    public:

        /** The Database Constructor. To determine file location and the Bytes per Record. **/
        BinaryHashMap(std::string strBaseLocationIn) : strBaseLocation(strBaseLocationIn)
        {
            Initialize();
        }


        /** Clean up Memory Usage. **/
        ~BinaryHashMap() { }


        /** Return the Keys to the Records Held in the Database. **/
        std::vector< std::vector<uint8_t> > GetKeys()
        {
            std::vector< std::vector<uint8_t> > vKeys;
            //TODO: FINISH
            return vKeys;
        }


        /** Return Whether a Key Exists in the Database. **/
        bool HasKey(const std::vector<uint8_t>& vKey)
        {
            if(mapBinaryIterators.count(vKey))
                return true;

            /* Get the assigned bucket for the hashmap. */
            uint32_t nBucket = GetBucket(vKey);

            /* Establish the Stream File for Keychain Bucket. */
            std::fstream ssFile(strprintf("%s_hashmap.%05u", strBaseLocation.c_str(), nBucket).c_str(), std::ios::in | std::ios::binary);
            if(!ssFile)
                return false;


            /* Read the Bucket File. */
            ssFile.ignore(std::numeric_limits<std::streamsize>::max());
            ssFile.seekg (0, std::ios::beg);
            std::vector<uint8_t> vBucket(ssFile.gcount(), 0);
            ssFile.read((char*) &vBucket[0], vBucket.size());
            ssFile.close();


            /* Iterator for Key Sectors. */
            uint32_t nIterator = 0;
            while(nIterator < vBucket.size())
            {

                /* Get Binary Data */
                std::vector<uint8_t> vData(vBucket.begin() + nIterator, vBucket.begin() + nIterator + 15);

                /* Read the State and Size of Sector Header. */
                SectorKey cKey;
                CDataStream ssKey(vData, SER_LLD, DATABASE_VERSION);
                ssKey >> cKey;

                if(cKey.Ready())
                {
                    /* Read the Key Data. */
                    std::vector<uint8_t> vKeyIn(vBucket.begin() + nIterator + 15, vBucket.begin() + nIterator + 15 + cKey.nLength);

                    /* Found the Binary Position. */
                    if(vKeyIn == vKey)
                    {
                        mapBinaryIterators[vKey] = nIterator;

                        return true;
                    }
                }

                nIterator += cKey.Size();
            }

            return false;
        }


        /** Handle the Assigning of a Map Bucket. **/
        uint32_t GetBucket(const std::vector<uint8_t>& vKey) const
        {
            uint64_t nBucket = 0;
            for(int i = 0; i < vKey.size() && i < 8; i++)
                nBucket += vKey[i] << (8 * i);

            return nBucket % HASHMAP_TOTAL_BUCKETS;
        }


        /** Read the Database Keys and File Positions. **/
        void Initialize()
        {
            LOCK(KEY_MUTEX);

            /* Create directories if they don't exist yet. */
            if(boost::filesystem::create_directories(strBaseLocation))
                printf(FUNCTION "Generated Path %s\n", __PRETTY_FUNCTION__, strBaseLocation.c_str());
        }


        /** Add / Update A Record in the Database **/
        bool Put(SectorKey cKey) const
        {
            LOCK(KEY_MUTEX);

            /* Get the bucket for the key */
            uint32_t nBucket = GetBucket(cKey.vKey);

            /* Establish the Stream File for Keychain Bucket. */
            std::ofstream ssFile(strprintf("%s_hashmap.%05u", strBaseLocation.c_str(), nBucket).c_str(), std::ios::out | std::ios::binary);

            if(mapBinaryIterators.count(cKey.vKey))
                ssFile.seekp(mapBinaryIterators[cKey.vKey], std::ios::beg);
            else
                ssFile.seekp(0, std::ios::end);

            //for debugging only
            uint32_t nIterator = ssFile.tellp();


            /* Handle the Sector Key Serialization. */
            CDataStream ssKey(SER_LLD, DATABASE_VERSION);
            ssKey.reserve(cKey.Size());
            ssKey << cKey;


            /* Write to Disk. */
            std::vector<uint8_t> vData(ssKey.begin(), ssKey.end());
            vData.insert(vData.end(), cKey.vKey.begin(), cKey.vKey.end());
            ssFile.write((char*) &vData[0], vData.size());
            ssFile.close();

            /* Debug Output of Sector Key Information. */
            if(GetArg("-verbose", 0) >= 4)
                printf(FUNCTION "State: %s | Length: %u | Position: %u | Bucket: %u | Sector File: %u | Sector Size: %u | Sector Start: %u | Key: %s\n", __PRETTY_FUNCTION__, cKey.nState == READY ? "Valid" : "Invalid", cKey.nLength, nIterator, nBucket, cKey.nSectorFile, cKey.nSectorSize, cKey.nSectorStart, HexStr(cKey.vKey.begin(), cKey.vKey.end()).c_str());


            return true;
        }

        /** Simple Erase for now, not efficient in Data Usage of HD but quick to get erase function working. **/
        bool Erase(const std::vector<uint8_t> vKey)
        {
            LOCK(KEY_MUTEX);

            /* Check for the Key. */
            uint32_t nBucket = GetBucket(vKey);

            /* Establish the Outgoing Stream. */
            std::fstream ssFile(strprintf("%s_hashmap.%05u", strBaseLocation.c_str(), nBucket).c_str(), std::ios::in | std::ios::out | std::ios::binary);


            return true;
        }

        /** Get a Record from the Database with Given Key. **/
        bool Get(const std::vector<uint8_t> vKey, SectorKey& cKey)
        {
            LOCK(KEY_MUTEX);

            /* Get the assigned bucket for the hashmap. */
            uint32_t nBucket = GetBucket(vKey);

            /* Establish the Stream File for Keychain Bucket. */
            std::fstream ssFile(strprintf("%s_hashmap.%05u", strBaseLocation.c_str(), nBucket).c_str(), std::ios::in | std::ios::out | std::ios::binary);

            if(mapBinaryIterators.count(vKey))
            {
                ssFile.seekg(mapBinaryIterators[vKey], std::ios::beg);

                /* Read the State and Size of Sector Header. */
                std::vector<uint8_t> vData(15, 0);
                ssFile.read((char*) &vData[0], 15);


                /* De-serialize the Header. */
                CDataStream ssHeader(vData, SER_LLD, DATABASE_VERSION);
                ssHeader >> cKey;


                /* Skip Empty Sectors for Now. (TODO: Expand to Reads / Writes) */
                if(cKey.Ready())
                {

                    /* Read the Key Data. */
                    std::vector<uint8_t> vKeyIn(cKey.nLength, 0);
                    ssFile.read((char*) &vKeyIn[0], vKeyIn.size());

                    /* Check the Keys Match Properly. */
                    if(vKeyIn != vKey)
                        return error(FUNCTION "Key Mistmatch: DB:: %s MEM %s\n", __PRETTY_FUNCTION__, HexStr(vKeyIn.begin(), vKeyIn.end()).c_str(), HexStr(vKey.begin(), vKey.end()).c_str());

                    /* Assign Key to Sector. */
                    cKey.vKey = vKeyIn;

                    /* Debug Output of Sector Key Information. */
                    if(GetArg("-verbose", 0) >= 4)
                        printf(FUNCTION "State: %s | Length: %u | Position: %u | Bucket: %u | Sector File: %u | Sector Size: %u | Sector Start: %u | Key: %s\n", __PRETTY_FUNCTION__, cKey.nState == READY ? "Valid" : "Invalid", cKey.nLength, mapBinaryIterators[vKey], nBucket, cKey.nSectorFile, cKey.nSectorSize, cKey.nSectorStart, HexStr(cKey.vKey.begin(), cKey.vKey.end()).c_str());

                    return true;
                }
            }
            else
            {
                /* Read the Bucket File. */
                ssFile.ignore(std::numeric_limits<std::streamsize>::max());
                ssFile.seekg (0, std::ios::beg);
                std::vector<uint8_t> vBucket(ssFile.gcount(), 0);
                ssFile.read((char*) &vBucket[0], vBucket.size());


                /* Iterator for Key Sectors. */
                uint32_t nIterator = 0;
                while(nIterator < vBucket.size())
                {

                    /* Get Binary Data */
                    std::vector<uint8_t> vData(vBucket.begin() + nIterator, vBucket.begin() + nIterator + 15);


                    /* Read the State and Size of Sector Header. */
                    CDataStream ssKey(vData, SER_LLD, DATABASE_VERSION);
                    ssKey >> cKey;

                    if(cKey.Ready())
                    {

                        /* Read the Key Data. */
                        std::vector<uint8_t> vKeyIn(vBucket.begin() + nIterator + 15, vBucket.begin() + nIterator + 15 + cKey.nLength);

                        /* Found the Binary Position. */
                        if(vKeyIn == vKey)
                        {
                            cKey.vKey = vKeyIn;

                            /* Debug Output of Sector Key Information. */
                            if(GetArg("-verbose", 0) >= 4)
                                printf(FUNCTION "State: %s | Length: %u | Position: %u | Bucket: %u | Sector File: %u | Sector Size: %u | Sector Start: %u | Key: %s\n", __PRETTY_FUNCTION__, cKey.nState == READY ? "Valid" : "Invalid", cKey.nLength, nIterator, nBucket, cKey.nSectorFile, cKey.nSectorSize, cKey.nSectorStart, HexStr(cKey.vKey.begin(), cKey.vKey.end()).c_str());

                            return true;
                        }
                    }

                    nIterator += cKey.Size();
                }
            }

            return false;
        }
    };
}

#endif
