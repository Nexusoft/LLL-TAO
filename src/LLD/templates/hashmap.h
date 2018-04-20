/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2017] ++
            
            (c) Copyright The Nexus Developers 2014 - 2017
            
            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.
            
            "fides in stellis, virtus in numeris" - Faith in the Stars, Power in Numbers

____________________________________________________________________________________________*/

#ifndef NEXUS_LLD_TEMPLATES_HASHMAP_H
#define NEXUS_LLD_TEMPLATES_HASHMAP_H

#include <fstream>

#include "key.h"

namespace LLD
{
    
    class HashKey : public SectorKey
    {
    public:
        
        unsigned int nIterator;
        unsigned int nBucket;
    
        HashKey(SectorKey cKey, unsigned int nIteratorIn, unsigned int nBucketIn) : SectorKey(cKey), nIterator(nIteratorIn), nBucket(nBucketIn) {}
    };
    

    /** The Maximum buckets allowed in the hashmap. */
    const unsigned int HASHMAP_TOTAL_BUCKETS = 256;
    
    
    /** The Maximum cache size for allocated keys. **/
    const unsigned int HASHMAP_MAX_CACHE_SZIE = 1024 * 1024; //1 MB
    
    
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
        unsigned int nCacheSize = 0;
        
        /** Cache Pool for Found Records. **/
        MemCachePool* cachePool;
        
    public:	
        
        /** The Database Constructor. To determine file location and the Bytes per Record. **/
        BinaryHashMap(std::string strBaseLocationIn, std::string strDatabaseNameIn) : strBaseLocation(strBaseLocationIn), cachePool(new MemCachePool(HASHMAP_MAX_CACHE_SZIE))
        {
            Initialize();
        }
        
        
        /** Clean up Memory Usage. **/
        ~BinaryHashMap() { delete cachePool; }
        
        
        /** Return the Keys to the Records Held in the Database. **/
        std::vector< std::vector<unsigned char> > GetKeys()
        {
            std::vector< std::vector<unsigned char> > vKeys;
            //TODO: FINISH
            return vKeys;
        }
        
        
        /** Return Whether a Key Exists in the Database. **/
        bool HasKey(const std::vector<unsigned char>& vKey)
        { 
            
            /* Get the assigned bucket for the hashmap. */
            unsigned int nBucket = GetBucket(vKey);
            
            /* Establish the Stream File for Keychain Bucket. */
            std::fstream ssIncoming(strprintf("%s_hashmap.%05u", strBaseLocation.c_str(), nBucket).c_str(), std::ios::in | std::ios::out | std::ios::binary);
            
            
            /* Read the Bucket File. */
            ssIncoming.ignore(std::numeric_limits<std::streamsize>::max());
            ssIncoming.seekg (0, std::ios::beg);
            std::vector<unsigned char> vBucket(fStream.gcount(), 0);
            ssIncoming.read((char*) &vBucket[0], vBucket.size());
            ssIncoming.close();
                        
                        
            /* Iterator for Key Sectors. */
            unsigned int nIterator = 0;
            while(nIterator < vBucket.size())
            {
                            
                /* Get Binary Data */
                std::vector<unsigned char> vData(vBucket.begin() + nIterator, vBucket.begin() + nIterator + 15);
                            
                            
                /* Read the State and Size of Sector Header. */
                SectorKey cKey;
                CDataStream ssKey(vData, SER_LLD, DATABASE_VERSION);
                ssKey >> cKey;
                            
                if(cKey.Ready())
                {
                            
                    /* Read the Key Data. */
                    std::vector<unsigned char> vKeyIn(vBucket.begin() + nIterator + 15, vBucket.begin() + nIterator + 15 + cKey.nLength);
                                
                    /* Found the Binary Position. */
                    if(vKeyIn == vKey)
                    {
                        return true;
                    }       
                }
                    
                nIterator += cKey.Size();
            }
            
            return false;
        }
        
        
        /** Handle the Assigning of a Map Bucket. **/
        unsigned int GetBucket(const std::vector<unsigned char>& vKey) const
        {
            uint64 nBucket = 0;
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
            

            
            
            /* Debug Output of Sector Key Information. */
            if(GetArg("-verbose", 0) >= 4)
                printf(FUNCTION "State: %s | Length: %u | Location: %u | File: %u | Sector File: %u | Sector Size: %u | Sector Start: %u\n Key: %s\nCurrent File: %u | Current File Size: %u\n", __PRETTY_FUNCTION__, cKey.nState == READY ? "Valid" : "Invalid", cKey.nLength, mapKeys[nBucket][cKey.vKey].second, mapKeys[nBucket][cKey.vKey].first, cKey.nSectorFile, cKey.nSectorSize, cKey.nSectorStart, HexStr(cKey.vKey.begin(), cKey.vKey.end()).c_str(), nCurrentFile, nCurrentFileSize);
            
            
            return true;
        }
        
        /** Simple Erase for now, not efficient in Data Usage of HD but quick to get erase function working. **/
        bool Erase(const std::vector<unsigned char> vKey)
        {
            LOCK(KEY_MUTEX);
            
            /* Check for the Key. */
            unsigned int nBucket = GetBucket(vKey);
            
            /* Establish the Outgoing Stream. */
            std::fstream fStream(strprintf("%s_hashmap.%05u", strBaseLocation.c_str(), nBucket).c_str(), std::ios::in | std::ios::out | std::ios::binary);
            
            
            /* Set to put at the right file and sector position. */
            fStream.seekp(mapKeys[nBucket][vKey].second, std::ios::beg);
            
            
            /* Establish the Sector State as Empty. */
            std::vector<unsigned char> vData(1, EMPTY);
            fStream.write((char*) &vData[0], vData.size());
                
            
            /* Remove the Sector Key from the Memory Map. */
            mapKeys[nBucket].erase(vKey);
            
            
            return true;
        }
        
        /** Get a Record from the Database with Given Key. **/
        bool Get(const std::vector<unsigned char> vKey, SectorKey& cKey)
        {
            LOCK(KEY_MUTEX);
            
            
            /* Debug Output of Sector Key Information. */
            if(GetArg("-verbose", 0) >= 4)
                printf(FUNCTION "State: %s | Length: %u | Location: %u | File: %u | Sector File: %u | Sector Size: %u | Sector Start: %u\n Key: %s\n", __PRETTY_FUNCTION__, cKey.nState == READY ? "Valid" : "Invalid", cKey.nLength, mapKeys[nBucket][vKey].second, mapKeys[nBucket][vKey].first, cKey.nSectorFile, cKey.nSectorSize, cKey.nSectorStart, HexStr(cKey.vKey.begin(), cKey.vKey.end()).c_str());

            
            return false;
        }
    };
}
    
#endif
