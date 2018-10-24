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

#include <algorithm>


//TODO: Abstract base class for all keychains
namespace LLD
{


    /** Binary Hash Map
     *
     *  This class is responsible for managing the keys to the sector database.
     *
     *  It contains a Binary Hash Map with a minimum complexity of O(1)
     *  It uses a linked file list based on index to iterate trhough files and binary Positions
     *  when there is a collision that is found
     *
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


        /** The Maximum buckets allowed in the hashmap. */
        uint32_t HASHMAP_TOTAL_BUCKETS;


        /** The Maximum cache size for allocated keys. **/
        uint32_t HASHMAP_MAX_CACHE_SZIE;


        /** The MAximum key size for static key sectors. **/
        uint16_t HASHMAP_MAX_KEY_SIZE;


        /** The total space that a key consumes. */
        uint16_t HASHMAP_KEY_ALLOCATION;


        /* Keychain stream object. */
        std::fstream* pstream[10];

    public:

        BinaryHashMap() : HASHMAP_TOTAL_BUCKETS(256 * 256 * 10), HASHMAP_MAX_CACHE_SZIE(10 * 1024), HASHMAP_MAX_KEY_SIZE(128), HASHMAP_KEY_ALLOCATION(HASHMAP_MAX_KEY_SIZE + 15) {}

        /** The Database Constructor. To determine file location and the Bytes per Record. **/
        BinaryHashMap(std::string strBaseLocationIn) : strBaseLocation(strBaseLocationIn), HASHMAP_TOTAL_BUCKETS(256 * 256), HASHMAP_MAX_CACHE_SZIE(10 * 1024), HASHMAP_MAX_KEY_SIZE(128), HASHMAP_KEY_ALLOCATION(HASHMAP_MAX_KEY_SIZE + 15)
        {
            Initialize();
        }

        BinaryHashMap& operator=(BinaryHashMap map)
        {
            strBaseLocation    = map.strBaseLocation;

            for(int i = 0; i < 10; i++)
                pstream[i]     = map.pstream[i];

            return *this;
        }


        BinaryHashMap(const BinaryHashMap& map)
        {
            strBaseLocation    = map.strBaseLocation;

            for(int i = 0; i < 10; i++)
                pstream[i]     = map.pstream[i];
        }


        /** Clean up Memory Usage. **/
        ~BinaryHashMap()
        {
        }


        /** Handle the Assigning of a Map Bucket. **/
        uint32_t GetBucket(const std::vector<uint8_t> vKey) const
        {
            uint64_t nBucket = 0;
            for(int i = 0; i < vKey.size() && i < 8; i++)
                nBucket += vKey[i] << (8 * i);

            return nBucket % HASHMAP_TOTAL_BUCKETS;
        }


        /** Read the Database Keys and File Positions. **/
        void Initialize()
        {
            /* Create directories if they don't exist yet. */
            if(boost::filesystem::create_directories(strBaseLocation))
                printf(FUNCTION "Generated Path %s\n", __PRETTY_FUNCTION__, strBaseLocation.c_str());


            /* Setup the file objects. */
            for(int i = 0; i < 10; i++)
            {
                const char* file = strprintf("%s_hashmap.%05u", strBaseLocation.c_str(), i).c_str();
                if(!boost::filesystem::exists(file))
                {
                    uint32_t nMaxSize = HASHMAP_TOTAL_BUCKETS * HASHMAP_KEY_ALLOCATION;

                    std::vector<uint8_t> vSpace(nMaxSize, 0);
                    pstream[i] = new std::fstream(file, std::ios::in | std::ios::out | std::ios::binary | std::ios::trunc);
                    pstream[i]->write((char*)&vSpace[0], vSpace.size());
                    pstream[i]->close();

                    printf(FUNCTION "Generated Disk Hash Map %i of %u bytes\n", __PRETTY_FUNCTION__, i, vSpace.size());
                }

                pstream[i] = new std::fstream(file, std::ios::in | std::ios::out | std::ios::binary); //open the file objects. Have a file cache handler

                //pStream[i]->rdbuf()->pubsetbuf(iobuf, sizeof iobuf);
            }
        }


        /** Get a Record from the Database with Given Key. **/
        bool Get(const std::vector<uint8_t> vKey, SectorKey& cKey)
        {
            LOCK(KEY_MUTEX);

            /* Get the assigned bucket for the hashmap. */
            uint32_t nBucket = GetBucket(vKey);

            /* Get the file binary position. */
            uint32_t nFilePos = nBucket * HASHMAP_KEY_ALLOCATION;

            /* Get the binary data. */
            for(int i = 0; i < 10; i++)
            {
                pstream[i]->seekg (nFilePos, std::ios::beg);
                std::vector<uint8_t> vKeychain(vKey.size(), 0);
                pstream[i]->read((char*) &vKeychain[0], vKeychain.size());

                /* Check that the keys match. */
                if(vKey == vKeychain)
                {
                    /* Read the key object from disk. */
                    pstream[i]->seekg (nFilePos + HASHMAP_MAX_KEY_SIZE, std::ios::beg);
                    std::vector<uint8_t> vData(15, 0);
                    pstream[i]->read((char*) &vData[0], vData.size());

                    /* Read the State and Size of Sector Header. */
                    SectorKey cKey;
                    CDataStream ssKey(vData, SER_LLD, DATABASE_VERSION);
                    ssKey >> cKey;

                    return true;
                }
            }

            return false;
        }


        /** Add / Update A Record in the Database **/
        bool Put(SectorKey cKey) const
        {
            LOCK(KEY_MUTEX);

            /* Get the assigned bucket for the hashmap. */
            uint32_t nBucket = GetBucket(cKey.vKey);

            /* Get the file binary position. */
            uint32_t nFilePos = nBucket * HASHMAP_KEY_ALLOCATION;

            /* Get the binary data. */
            std::vector<uint8_t> vBlank(cKey.vKey.size(), 0);
            for(int i = 0; i < 10; i++)
            {
                pstream[i]->seekg (nFilePos, std::ios::beg);
                std::vector<uint8_t> vKeychain(cKey.vKey.size(), 0);
                pstream[i]->read((char*) &vKeychain[0], vKeychain.size());

                /* Check that the keys match. */
                if(vBlank == vKeychain || cKey.vKey == vKeychain)
                {
                    /* Read the key object from disk. */
                    std::vector<uint8_t> vData(cKey.vKey);

                    /* Read the State and Size of Sector Header. */
                    SectorKey cKey;
                    CDataStream ssKey(SER_LLD, DATABASE_VERSION);
                    ssKey << cKey;

                    /* Insert data into the back of the vector. */
                    vData.insert(vData.end(), ssKey.begin(), ssKey.end());
                    pstream[i]->seekp (nFilePos, std::ios::beg);
                    pstream[i]->write((char*)&vData[0], vData.size());

                    return true;
                }
            }

            return false;
        }

        /** Simple Erase for now, not efficient in Data Usage of HD but quick to get erase function working. **/
        bool Erase(const std::vector<uint8_t> vKey)
        {
            LOCK(KEY_MUTEX);

            /* Check for the Key. */
            uint32_t nBucket = GetBucket(vKey);

            return true;
        }
    };
}

#endif
