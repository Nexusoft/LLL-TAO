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


        /* Keychain stream object. */
        std::fstream* pStream[10];

    public:

        BinaryHashMap() {}

        /** The Database Constructor. To determine file location and the Bytes per Record. **/
        BinaryHashMap(std::string strBaseLocationIn) : strBaseLocation(strBaseLocationIn)
        {
            Initialize();

            for(int i = 0; i < 10; i++)
                pStream[i] = new std::fstream(strprintf("%s_hashmap.%05u", strBaseLocation.c_str(), i).c_str(), std::ios::in | std::ios::out | std::ios::binary);
        }

        BinaryHashMap& operator=(BinaryHashMap map)
        {
            strBaseLocation    = map.strBaseLocation;

            for(int i = 0; i < 10; i++)
                pStream[i]     = map.pStream[i];

            return *this;
        }


        BinaryHashMap(const BinaryHashMap& map)
        {
            strBaseLocation    = map.strBaseLocation;

            for(int i = 0; i < 10; i++)
                pStream[i]     = map.pStream[i];
        }


        /** Clean up Memory Usage. **/
        ~BinaryHashMap() { }


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
        }


        /** Add / Update A Record in the Database **/
        bool Put(SectorKey cKey) const
        {
            LOCK(KEY_MUTEX);

            /* Get the bucket for the key */
            uint32_t nBucket = GetBucket(cKey.vKey);

            return true;
        }

        /** Simple Erase for now, not efficient in Data Usage of HD but quick to get erase function working. **/
        bool Erase(const std::vector<uint8_t> vKey)
        {
            LOCK(KEY_MUTEX);

            /* Check for the Key. */
            uint32_t nBucket = GetBucket(vKey);

            return true;
        }

        /** Get a Record from the Database with Given Key. **/
        bool Get(const std::vector<uint8_t> vKey, SectorKey& cKey)
        {
            LOCK(KEY_MUTEX);

            /* Get the assigned bucket for the hashmap. */
            uint32_t nBucket = GetBucket(vKey);

            return false;
        }
    };
}

#endif
