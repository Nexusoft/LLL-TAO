/*__________________________________________________________________________________________
           
            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2017] ++
            
            (c) Copyright The Nexus Developers 2014 - 2017
            
            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.
            
            "fides in stellis, virtus in numeris" - Faith in the Stars, Power in Numbers
____________________________________________________________________________________________*/

#ifndef NEXUS_LLD_TEMPLATES_MEMCACHEPOOL_H
#define NEXUS_LLD_TEMPLATES_MEMCACHEPOOL_H

#include "../../Util/templates/serialize.h"
#include "../../Util/include/mutex.h"

namespace LLD
{	
    
    /* The number of buckets available in Cache Pool. */
    const unsigned int MAX_CACHE_POOL_BUCKETS = 256 * 256;
    
    
    enum
    {
        PENDING_WRITE = 0,
        PENDING_ERASE = 1,
        
        MEMORY_ONLY   = 10, //Default State
        PENDING_TX    = 11,
        
        COMPLETED     = 255
    };
    

    /* Holding Object for Memory Maps. */
    struct CachedData
    {
        unsigned char  State;
        uint64         Timestamp;
        std::vector<unsigned char> Data;
    };
    
    
    /** Holding Pool:
    * 
    * This class is responsible for holding data that is partially processed.
    * It is also uselef for data that needs to be relayed from cache once recieved.
    * 
    * It must adhere to the processing expiration time.
    * 
    * A. It can pass data on the relay layers if required.
    * B. It can process data locked as orphans
    * 
    */
    class MemCachePool
    {
        
    protected:
        
        /* Destructor flag. */
        bool fDestruct;
        
        
        /* The Maximum Size of the Cache. */
        unsigned int MAX_CACHE_SIZE;
        
        
        /* The current size of the pool. */
        unsigned int nCurrentSize;
        
        
        /* Mutex for thread concurrencdy. */
        Mutex_t MUTEX;
        
        
        /* Map of the current holding data. */
        std::map<std::vector<unsigned char>, CachedData > mapObjects[MAX_CACHE_POOL_BUCKETS];
        
        
        /* Disk Buffer Object to flush objects to disk. */
        std::vector< std::pair<std::vector<unsigned char>, std::vector<unsigned char>> > vDiskBuffer;
        
        
        /* Transaction Disk Buffer Object. */
        std::vector< std::pair<std::vector<unsigned char>, std::vector<unsigned char>> > vTransactionBuffer;
        
        
        /* Thread of cache cleaner. */
        Thread_t CACHE_THREAD;
        
        
    public:
        
        /** Base Constructor.
        * 
        * MAX_CACHE_SIZE default value is 32 MB
        * MAX_CACHE_BUCKETS default value is 65,539 (2 bytes)
        * 
        */
        MemCachePool() : fDestruct(false), MAX_CACHE_SIZE(1024 * 1024), nCurrentSize(0), CACHE_THREAD(boost::bind(&MemCachePool::CacheCleaner, this)) {}
        
        
        /** Cache Size Constructor
        * 
        * @param[in] nCacheSizeIn The maximum size of this Cache Pool
        * 
        */
        MemCachePool(unsigned int nCacheSizeIn) : fDestruct(false), MAX_CACHE_SIZE(nCacheSizeIn), nCurrentSize(0), CACHE_THREAD(boost::bind(&MemCachePool::CacheCleaner, this)) {}
        
        
        /* Class Destructor. */
        ~MemCachePool()
        {
            fDestruct = true;
            
            CACHE_THREAD.join();
        }
        
        
        /** Get the assigned bucket
        * 
        * @param[in] vKey The binary data of key 
        * 
        * @return The bucket number through serializaing first two bytes of key.
        */
        unsigned int GetBucket(std::vector<unsigned char> vKey) const 
        { 
            uint64 nBucket = 0;
            for(int i = 0; i < vKey.size() && i < 8; i++)
                nBucket += vKey[i] << (8 * i);
            
            return nBucket % MAX_CACHE_POOL_BUCKETS;
        }
        
        
        /** Check if data exists
        * 
        * @param[in] vKey The binary data of the key
        * 
        * @return True/False whether pool contains data by index
        * 
        */
        bool Has(std::vector<unsigned char> vKey, unsigned int nBucket = MAX_CACHE_POOL_BUCKETS + 1) const 
        { 
            if(nBucket == MAX_CACHE_POOL_BUCKETS + 1)
                nBucket = GetBucket(vKey);
            
            return (mapObjects[nBucket].find(vKey) != mapObjects[nBucket].end());
        }
        
        
        /** Get the data by index
        * 
        * @param[in] vKey The binary data of the key
        * @param[out] vData The binary data of the cached record
        * 
        * @return True if object was found, false if none found by index.
        * 
        */
        bool Get(std::vector<unsigned char> vKey, std::vector<unsigned char>& vData)
        {
            LOCK(MUTEX);
            
            /* Check if the Record Exists. */
            auto nBucket = GetBucket(vKey);
            if(!Has(vKey, nBucket))
                return false;
            
            /* Get the data record. */
            vData = mapObjects[nBucket][vKey].Data;
            
            /* Update the Object's time (to keep cache of most accessed elements). */
            mapObjects[nBucket][vKey].Timestamp = Timestamp(true);
            
            return true;
        }
        
        
        /** Get the Bulk Objects in the Pool
        * 
        * @param[out] vObjects A list of objects from the pool in binary form
        * @param[in] nLimit The limit to the number of indexes to get (0 = unlimited)
        * 
        * @return Returns true if there are indexes, false if none found.
        * 
        * TODO: Determine how data type will be carried forward for serializaing
        * 
        */
        bool Get(std::vector< std::pair<std::vector<unsigned char>, std::vector<unsigned char>> >& vObjects, unsigned char nState = MEMORY_ONLY, unsigned int nLimit = 0)
        {
            LOCK(MUTEX);
            
            for(int nBucket = 0; nBucket < MAX_CACHE_POOL_BUCKETS; nBucket++)
            {
                for(auto obj : mapObjects[nBucket])
                {
                    if(nLimit != 0 && vObjects.size() >= nLimit)
                        return true;
                    
                    if(nState != obj.second.State)
                        continue;
                        
                    vObjects.push_back(std::make_pair(obj.first, obj.second.Data));
                }
            }
            
            return (vObjects.size() > 0);
        }
        
        
        /** Get the indexes in Pool
        * 
        * @param[out] vIndexes A list of indexes from the pool in binary form
        * @param[in] nLimit The limit to the number of indexes to get (0 = unlimited)
        * 
        * @return Returns true if there are indexes, false if none found.
        * 
        * TODO: Determine how data type will be carried forward for serializaing
        * 
        */
        bool GetIndexes(std::vector< std::vector<unsigned char> >& vIndexes, unsigned char nState = MEMORY_ONLY, unsigned int nLimit = 0)
        {
            LOCK(MUTEX);
            
            for(int nBucket = 0; nBucket < MAX_CACHE_POOL_BUCKETS; nBucket++)
            {
                for(auto obj : mapObjects[nBucket])
                {
                    if(nLimit != 0 && vIndexes.size() >= nLimit)
                        return true;
                    
                    if(nState != obj.second.State)
                        continue;
                        
                    vIndexes.push_back(obj.first);
                }
            }
            
            return (vIndexes.size() > 0);
        }
        
        
        /** Add data in the Pool
        * 
        * @param[in] vKey The key in binary form
        * @param[in] vData The input data in binary form
        * @param[in] nState The state of the object being written
        * @param[in] nTimestamp The Time record was Put (ms)
        * 
        */
        void Put(std::vector<unsigned char> vKey, std::vector<unsigned char> vData, unsigned char nState = MEMORY_ONLY, uint64 nTimestamp = Timestamp(true))
        {
            LOCK(MUTEX);
            
            auto nBucket = GetBucket(vKey);
            if(!Has(vKey, nBucket))
                nCurrentSize += vData.size();
            
            /* Handle the Writing Buffer, out of transaction orders. */
            if(nState == PENDING_WRITE)
                vDiskBuffer.push_back(std::make_pair(vKey, vData));
            
            /* Handle a Pending Transaction, write to buffer */
            else if(nState == PENDING_TX)
                vTransactionBuffer.push_back(std::make_pair(vKey, vData));
                
            /* Overwrite existing objects. */
            CachedData cacheObject = { nState, nTimestamp, vData };
            mapObjects[nBucket][vKey] = cacheObject;
        }
        
        
        /** Get Disk Buffer.
        * 
        *  Returns the current disk buffer ready for writing.
        *
        */
        bool GetDiskBuffer(std::vector< std::pair<std::vector<unsigned char>, std::vector<unsigned char>> >& vBuffer)
        {
            LOCK(MUTEX);
            
            if(vDiskBuffer.size() == 0)
                return false;
            
            vBuffer = vDiskBuffer;
            vDiskBuffer.clear();
            
            return true;
        }
        
        
        /** Get Transaction Buffer.
        * 
        *  Returns the current disk buffer ready for writing.
        *
        */
        bool GetTransactionBuffer(std::vector< std::pair<std::vector<unsigned char>, std::vector<unsigned char>> >& vBuffer)
        {
            LOCK(MUTEX);
            
            if(vTransactionBuffer.size() == 0)
                return false;
            
            vBuffer = vTransactionBuffer;
            vTransactionBuffer.clear();
            
            return true;
        }
        
        
        /** Set the state of the data in cache
        * 
        * @param[in] vKey The key in binary form
        * @param[in] nState The new state of the object.
        * 
        */
        void SetState(std::vector<unsigned char> vKey, unsigned short nState)
        {
            auto nBucket = GetBucket(vKey);
            if(!Has(vKey, nBucket))
                return;
            
            mapObjects[nBucket][vKey].Timestamp = Timestamp(true);
            mapObjects[nBucket][vKey].State = nState;
        }
        
        
        /** Force Remove Object by Index
        * 
        * @param[in] vKey Binary Data of the Key
        * 
        * @return True on successful removal, false if it fails
        * 
        */
        bool Remove(std::vector<unsigned char> vKey)
        {
            LOCK(MUTEX);
            
            /* Check if the Record Exists. */
            auto nBucket = GetBucket(vKey);
            if(!Has(vKey, nBucket))
                return false;
            
            nCurrentSize -= mapObjects[nBucket][vKey].Data.size();
            
            //if(mapObjects[nBucket][vKey].State == PENDING_TX)
            //    mapObjects[nBucket][vKey].State = PENDING_ERASE;
            //else
            mapObjects[nBucket].erase(vKey);
            
            return true;
        }
        
        
        /** Force Remove Object by Index
        * 
        * @param[in] vKey Binary Data of the Key
        * @param[in] nState The State objects to remove
        * 
        * 
        * NOTE: This is high complexity, use sparingly
        * 
        */
        void Remove(unsigned char nState)
        {
            std::vector< std::vector<unsigned char> > vKeys;
            GetIndexes(vKeys, nState);
            
            for(auto Key : vKeys)
                Remove(Key);
        }
        
        
        static bool SortByTime(const std::pair< std::vector<unsigned char>, CachedData>& a, const std::pair< std::vector<unsigned char>, CachedData>& b)
        {
            return a.second.Timestamp < b.second.Timestamp;
        }
        
        
        /** Clean up older data from the Cache Pool to keep within Cache Limits. 
        * 
        *  This is a Worker Thread.
        */
        void CacheCleaner()
        {
            while(!fDestruct)
            {
                /* Trim off less used objects if reached cache limits. */
                if(nCurrentSize > MAX_CACHE_SIZE)
                {
                    std::vector< std::pair< std::vector<unsigned char>, CachedData > > vKeys;
                    for(int nBucket = 0; nBucket < MAX_CACHE_POOL_BUCKETS; nBucket++)
                    {
                        for(auto obj : mapObjects[nBucket])
                        {
                            /* Don't clear objects waiting for writes or transactions. */
                            if( (obj.second.State != PENDING_WRITE) && (obj.second.State != PENDING_ERASE) && (obj.second.State != PENDING_TX) )
                                vKeys.push_back(obj);
                        }
                    }
                    
                    /* Sort by earliest timestamp and remove until cache is balanced. */
                    std::sort(vKeys.begin(), vKeys.end(), SortByTime);
                    for(auto obj : vKeys)
                    {
                        if(nCurrentSize <= MAX_CACHE_SIZE)
                            break;
                        
                        Remove(obj.first);
                    }
                }
                
                Sleep(1);
            }
        }
    };
}

#endif
