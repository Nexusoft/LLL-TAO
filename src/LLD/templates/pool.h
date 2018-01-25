/*__________________________________________________________________________________________
 
			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2017] ++
			
			(c) Copyright The Nexus Developers 2014 - 2017
			
			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.
			
			"fides in stellis, virtus in numeris" - Faith in the Stars, Power in Numbers
  
____________________________________________________________________________________________*/

#ifndef NEXUS_LLP_TEMPLATES_POOL_H
#define NEXUS_LLP_TEMPLATES_POOL_H

#include "../../Core/include/unifiedtime.h"
#include "../../Util/include/mutex.h"

namespace LLD
{	

	/* Holding Object for Memory Maps. */
	struct CachedData
	{
		uint64        Timestamp;
		std::vector<unsigned char> vObjectData;
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
	class CachePool
	{
		
	protected:
        
        
        /* The Maximum Size of the Cache. */
        unsigned int MAX_CACHE_SIZE;
        
        
        /* The Maximum Cache Buckets. */
        unsigned int MAX_CACHE_BUCKETS;
        
        
        /* The current size of the pool. */
        unsigned int nCurrentSize;
        
		
		/* Mutex for thread concurrencdy. */
		Mutex_t MUTEX;
		
        
        /* Map of the current holding data. */
		std::map<std::vector<unsigned char>, CachedData > mapObjects[];
        
	public:
		
		/** Base Constructor.
         * 
         * MAX_CACHE_SIZE default value is 32 MB
         * MAX_CACHE_BUCKETS default value is 65,539 (2 bytes)
         * 
         */
		CachePool() : mapObjects(), MAX_CACHE_SIZE(32 * 1024 * 1024), MAX_CACHE_BUCKETS(256 * 256), nCurrentSize(0) (0) {}
		
		
		/** Cache Size Constructor
		 * 
		 * @param[in] nCacheSizeIn The maximum size of this Cache Pool
		 * 
		 */
		CachePool(unsigned int nCacheSizeIn, unsigned int nMaxBuckets = 256 * 256) : mapObjects(), MAX_CACHE_SIZE(nCacheSizeIn), MAX_CACHE_BUCKETS(nMaxBuckets), nCurrentSize(0) {}
		
		
		/** Check for Data by Index.
		 * 
		 * @param[in] Index Template arguement to check by supplied index
		 * 
		 * @return Boolean expressino whether pool contains data by index
		 * 
		 */
		bool Has(std::vector<unsigned char> vKey) const { return mapObjects.count(Index); }
		
		
		/** Get the Data by Index
		 * 
		 * @param[in] Index Template argument to get by supplied index
		 * @param[out] Object Reference variable to return object if found
		 * 
		 * @return True if object was found, false if none found by index.
		 * 
		 */
		bool Get(IndexType Index, ObjectType& Object)
		{
			LOCK(MUTEX);
			
			if(!Has(Index))
				return false;
			
			Object = mapObjects[Index].Object;
			
			return true;
		}
		
		
		/** Get the Data by State.
		 * 
		 * @param[in] State The state that is being searched for 
		 * @param[out] vObjects The list of objects being sent out
		 * @param[in]  nLimit The limit to the number of objects to get (0 = unlimited)
		 * 
		 * @return Returns true if any states matched, false if none matched
		 * 
		 */
		bool Get(unsigned char State, std::vector<ObjectType>& vObjects, unsigned int nLimit = 0)
		{
			LOCK(MUTEX);
			
			for(auto const& i : mapObjects)
			{
				if(nLimit != 0 && vObjects.size() >= nLimit)
					return true;
				
				if(i.second.State == State)
					vObjects.push_back(i.second.Object);
			}
				
			return (vObjects.size() > 0);
		}
		
		
		/** Get the Indexes in Pool
		 * 
		 * @param[out] vIndexes A list of Indexes in the pool
		 * @param[in] nLimit The limit to the number of indexes to get (0 = unlimited)
		 * 
		 * @return Returns true if there are indexes, false if none found.
		 * 
		 */
		bool GetIndexes(std::vector<IndexType>& vIndexes, unsigned int nLimit = 0)
		{
			LOCK(MUTEX);
			
			for(auto i : mapObjects)
			{
				if(nLimit != 0 && vIndexes.size() >= nLimit)
					return true;
					
				vIndexes.push_back(i.first);
			}
			
			return (vIndexes.size() > 0);
		}
		
		
		/** Get the Indexs in Pool by State
		 * 
		 * @param[in] State The state char to filter results by
		 * @param[out] vIndexes The return vector with the results
		 * @param[in] nLimit The limit to the nuymber of indexes to get (0 = unlimited)
		 * 
		 * @return Returns true if indexes fit criteria, false if none found
		 * 
		 */
		bool GetIndexes(const unsigned char State, std::vector<IndexType>& vIndexes, unsigned int nLimit = 0)
		{
			LOCK(MUTEX);
			
			for(auto i : mapObjects)
			{
				if(nLimit != 0 && vIndexes.size() >= nLimit)
					return true;
				
				if(i.second.State == State)
					vIndexes.push_back(i.first);
			}
				
			return (vIndexes.size() > 0);
		}
		
		
		/** Update data in the Pool
		 * 
		 * Default state is UNVERIFIED
		 * 
		 * @param[in] Index Template argument to add selected index
		 * @param[in] Object Template argument for the object to be added.
		 * 
		 */
		bool Update(IndexType Index, ObjectType Object, unsigned char State = UNVERIFIED, uint64 nTimestamp = Timestamp(true))
		{
			LOCK(MUTEX);
			
			if(!Has(Index))
				return false;
			
			mapObjects[Index].Object    = Object;
			mapObjects[Index].State     = State;
			mapObjects[Index].Timestamp = nTimestamp;
			
			return true;
		}
		
		
		/** Add data to the pool
		 * 
		 * Default state is UNVERIFIED
		 * 
		 * @param[in] Index Template argument to add selected index
		 * @param[in] Object Template argument for the object to be added.
		 * 
		 */
		bool Add(IndexType Index, ObjectType Object, unsigned char State = UNVERIFIED, uint64 nTimestamp = Timestamp(true))
		{
			LOCK(MUTEX);
			
			if(Has(Index))
				return false;
			
			CHoldingObject<ObjectType> HoldingObject(nTimestamp, State, Object);
			mapObjects[Index] = HoldingObject;
			
			return true;
		}
		
		
		/** Set the State of specific Object
		 * 
		 * @param[in] Index Template argument to add selected index
		 * @param[in] State The selected state to add (Default is UNVERIFIED)
		 * 
		 */
		void AddState(IndexType Index, unsigned char State = UNVERIFIED)
		{ 
			LOCK(MUTEX);
			
			CHoldingObject<ObjectType> HoldingObject;
			HoldingObject.State = State;
			HoldingObject.Timestamp = Core::UnifiedTimestamp();
			
			mapObjects[Index] = HoldingObject;
		}

		
		/** Set the State of specific Object
		 * 
		 * @param[in] Index Template argument to add selected index
		 * @param[in] State The selected state to add (Default is UNVERIFIED)
		 * 
		 */
		void SetState(IndexType Index, unsigned char State = UNVERIFIED)
		{ 
			LOCK(MUTEX);
			
			if(!Has(Index))
				return;
			
			mapObjects[Index].State = State;
			mapObjects[Index].Timestamp = Core::UnifiedTimestamp();
		}
		
		
		/** Set Timestamp of Object
		 * 
		 * @param[in] Index Template argument to select Object
		 * @param[in] Timestamp The new timestamp for object
		 * 
		 */
		void SetTimestamp(IndexType Index, unsigned int Timestamp = Core::UnifiedTimestamp())
		{
			LOCK(MUTEX);
			
			if(!Has(Index))
				return;
			
			mapObjects[Index].Timestamp = Timestamp;
		}
		
		
		/** Get the State of Specific Object
		 * 
		 * @param[in] Index Template argument to add selected index
		 * 
		 * @return Object state (NOTFOUND returned if does not exist)
		 *
		 */
		unsigned char State(IndexType Index) const
		{
			if(!Has(Index))
				return NOTFOUND;
			
			return mapObjects.at(Index).State;
		}
		
		
		/** Force Remove Object by Index
		 * 
		 * @param[in] Index Template argument to determine location
		 * 
		 * @return True on successful removal, false if it fails
		 * 
		 */
		bool Remove(IndexType Index)
		{
			LOCK(MUTEX);
			
			if(!Has(Index))
				return false;
			
			mapObjects.erase(Index);
			
			return true;
		}
		
		
		/** Check if an object is Expired
		 * 
		 * @param[in] Index Template argument to determine location
		 * 
		 * @return True if object has expired, false if it is active
		 * 
		 */
		bool Expired(IndexType Index, unsigned int nTimestamp) const
		{
			if(!Has(Index))
				return true;
			
			if(mapObjects.at(Index).Timestamp + nTimestamp < Core::UnifiedTimestamp())
				return true;
			
			return false;
		}
		
		
		/** Check the age of an object since its last state change. 
		 * 
		 * @param[in] Index Template argument to determine location
		 * 
		 * @return The age in seconds of the object being quieried.
		 */
		unsigned int Age(IndexType Index) const
		{
			if(!Has(Index))
				return 0;
			
			return Core::UnifiedTimestamp() - mapObjects.at(Index).Timestamp;
		}
		
		
		/** Clean up data that is expired to keep memory use low
		 * 
		 * @return The number of elements that were removed in cleaning process.
		 * 
		 */
		int Clean()
		{
			std::vector<IndexType> vClean;
			
			{ LOCK(MUTEX);
				for(auto i : mapObjects)
					if(Expired(i.first, nExpirationTime))
						vClean.push_back(i.first);
			}
			
			for(auto i : vClean)
				Remove(i);
			
			return vClean.size();
		}
		
		/** Count or the number of elements in the memory pool
		 * 
		 * @return returns the total size of the pool.
		 * 
		 */
		int Count() const
		{
			return mapObjects.size();
		}
		
		
		/** Count or the number of elements in the pool
		 * 
		 * @param[in] State The state to filter the results with
		 * 
		 * @return Returns the total number of elements in the pool based on the state
		 * 
		 */
		int Count(unsigned char State)
		{
			LOCK(MUTEX);
			
			int nCount = 0;
			for(auto i : mapObjects)
				if(i.second.State == State)
					nCount++;
				
			return nCount;
		}
	};
}

#endif
