/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_UTIL_TEMPLATES_CONCURRENT_HASHMAP_H
#define NEXUS_UTIL_TEMPLATES_CONCURRENT_HASHMAP_H

#include <unordered_map>
#include <map>
#include <shared_mutex>
#include <mutex>
#include <vector>
#include <functional>
#include <optional>

namespace util
{
    /** ConcurrentHashMap
     *
     *  Thread-safe concurrent hash map that supports parallel access for scalable
     *  miner session management.
     *
     *  Key Features:
     *  - Lock-free reads with shared_mutex for high read throughput
     *  - Fine-grained locking for efficient concurrent writes
     *  - Supports atomic operations like insert_or_update
     *  - Iteration support with snapshot semantics
     *
     *  Uses std::shared_mutex for reader-writer locking:
     *  - Multiple readers can access concurrently
     *  - Writers get exclusive access
     *
     **/
    template <typename KeyType, typename ValueType, typename Hash = std::hash<KeyType>>
    class ConcurrentHashMap
    {
    private:
        /** Internal storage **/
        std::unordered_map<KeyType, ValueType, Hash> mapData;

        /** Reader-writer lock for concurrent access **/
        mutable std::shared_mutex MUTEX;

    public:
        /** Default Constructor **/
        ConcurrentHashMap() = default;

        /** Destructor **/
        ~ConcurrentHashMap() = default;

        /** Insert
         *
         *  Insert a key-value pair. Returns false if key already exists.
         *
         *  @param[in] key The key to insert
         *  @param[in] value The value to associate
         *
         *  @return true if inserted, false if key exists
         *
         **/
        bool Insert(const KeyType& key, const ValueType& value)
        {
            std::unique_lock<std::shared_mutex> lock(MUTEX);
            auto result = mapData.insert({key, value});
            return result.second;
        }

        /** InsertOrUpdate
         *
         *  Insert a key-value pair or update if key exists.
         *  Returns true if inserted, false if updated.
         *
         *  @param[in] key The key to insert/update
         *  @param[in] value The value to set
         *
         *  @return true if inserted, false if updated
         *
         **/
        bool InsertOrUpdate(const KeyType& key, const ValueType& value)
        {
            std::unique_lock<std::shared_mutex> lock(MUTEX);
            auto it = mapData.find(key);
            if(it == mapData.end())
            {
                mapData.insert({key, value});
                return true;
            }
            it->second = value;
            return false;
        }

        /** Update
         *
         *  Update an existing key's value. Returns false if key doesn't exist.
         *
         *  @param[in] key The key to update
         *  @param[in] value The new value
         *
         *  @return true if updated, false if key not found
         *
         **/
        bool Update(const KeyType& key, const ValueType& value)
        {
            std::unique_lock<std::shared_mutex> lock(MUTEX);
            auto it = mapData.find(key);
            if(it == mapData.end())
                return false;
            it->second = value;
            return true;
        }

        /** Get
         *
         *  Get a value by key. Returns std::nullopt if not found.
         *
         *  @param[in] key The key to look up
         *
         *  @return Optional containing value if found
         *
         **/
        std::optional<ValueType> Get(const KeyType& key) const
        {
            std::shared_lock<std::shared_mutex> lock(MUTEX);
            auto it = mapData.find(key);
            if(it == mapData.end())
                return std::nullopt;
            return it->second;
        }

        /** Contains
         *
         *  Check if a key exists in the map.
         *
         *  @param[in] key The key to check
         *
         *  @return true if key exists
         *
         **/
        bool Contains(const KeyType& key) const
        {
            std::shared_lock<std::shared_mutex> lock(MUTEX);
            return mapData.find(key) != mapData.end();
        }

        /** Erase
         *
         *  Remove a key-value pair. Returns true if key was found and removed.
         *
         *  @param[in] key The key to remove
         *
         *  @return true if removed, false if key not found
         *
         **/
        bool Erase(const KeyType& key)
        {
            std::unique_lock<std::shared_mutex> lock(MUTEX);
            return mapData.erase(key) > 0;
        }

        /** Size
         *
         *  Get the number of elements in the map.
         *
         *  @return Number of elements
         *
         **/
        size_t Size() const
        {
            std::shared_lock<std::shared_mutex> lock(MUTEX);
            return mapData.size();
        }

        /** Empty
         *
         *  Check if the map is empty.
         *
         *  @return true if empty
         *
         **/
        bool Empty() const
        {
            std::shared_lock<std::shared_mutex> lock(MUTEX);
            return mapData.empty();
        }

        /** Clear
         *
         *  Remove all elements from the map.
         *
         **/
        void Clear()
        {
            std::unique_lock<std::shared_mutex> lock(MUTEX);
            mapData.clear();
        }

        /** GetAll
         *
         *  Get a snapshot of all values in the map.
         *  Provides consistent iteration over all elements.
         *
         *  @return Vector of all values
         *
         **/
        std::vector<ValueType> GetAll() const
        {
            std::shared_lock<std::shared_mutex> lock(MUTEX);
            std::vector<ValueType> values;
            values.reserve(mapData.size());
            for(const auto& pair : mapData)
                values.push_back(pair.second);
            return values;
        }

        /** GetAllPairs
         *
         *  Get a snapshot of all key-value pairs in the map.
         *
         *  @return Vector of key-value pairs
         *
         **/
        std::vector<std::pair<KeyType, ValueType>> GetAllPairs() const
        {
            std::shared_lock<std::shared_mutex> lock(MUTEX);
            std::vector<std::pair<KeyType, ValueType>> pairs;
            pairs.reserve(mapData.size());
            for(const auto& pair : mapData)
                pairs.push_back(pair);
            return pairs;
        }

        /** ForEach
         *
         *  Execute a function for each element. The function should not modify the map.
         *
         *  @param[in] fn Function to execute for each element
         *
         **/
        void ForEach(std::function<void(const KeyType&, const ValueType&)> fn) const
        {
            std::shared_lock<std::shared_mutex> lock(MUTEX);
            for(const auto& pair : mapData)
                fn(pair.first, pair.second);
        }

        /** UpdateIf
         *
         *  Update a value if a condition is met. Returns true if updated.
         *
         *  @param[in] key The key to update
         *  @param[in] predicate Condition to check (receives current value)
         *  @param[in] updater Function to create new value (receives current value)
         *
         *  @return true if updated, false if key not found or condition not met
         *
         **/
        bool UpdateIf(
            const KeyType& key,
            std::function<bool(const ValueType&)> predicate,
            std::function<ValueType(const ValueType&)> updater)
        {
            std::unique_lock<std::shared_mutex> lock(MUTEX);
            auto it = mapData.find(key);
            if(it == mapData.end())
                return false;
            if(!predicate(it->second))
                return false;
            it->second = updater(it->second);
            return true;
        }

        /** GetAndRemove
         *
         *  Atomically get and remove a value.
         *
         *  @param[in] key The key to get and remove
         *
         *  @return Optional containing value if found
         *
         **/
        std::optional<ValueType> GetAndRemove(const KeyType& key)
        {
            std::unique_lock<std::shared_mutex> lock(MUTEX);
            auto it = mapData.find(key);
            if(it == mapData.end())
                return std::nullopt;
            ValueType value = std::move(it->second);
            mapData.erase(it);
            return value;
        }

        /** Transform
         *
         *  Atomically transform a single entry's value in-place.
         *  The transformer receives the current value and returns the new value,
         *  eliminating TOCTOU races from separate Get/Modify/Update sequences.
         *
         *  @param[in] key The key to transform
         *  @param[in] transformer Function to produce new value from current value
         *
         *  @return true if key was found and transformed, false if key not found
         *
         **/
        bool Transform(const KeyType& key, std::function<ValueType(const ValueType&)> transformer)
        {
            std::unique_lock<std::shared_mutex> lock(MUTEX);
            auto it = mapData.find(key);
            if(it == mapData.end())
                return false;
            it->second = transformer(it->second);
            return true;
        }

        /** TransformAll
         *
         *  Atomically transform all entries under a single write lock.
         *  Eliminates snapshot-overwrite races (e.g., NotifyNewRound)
         *  by applying the transformer to the current value of each entry.
         *
         *  @param[in] transformer Function to produce new value from current value
         *
         *  @return Number of entries transformed
         *
         **/
        uint32_t TransformAll(std::function<ValueType(const ValueType&)> transformer)
        {
            std::unique_lock<std::shared_mutex> lock(MUTEX);
            uint32_t nUpdated = 0;
            for(auto& pair : mapData)
            {
                pair.second = transformer(pair.second);
                ++nUpdated;
            }
            return nUpdated;
        }
    };


    /** ConcurrentOrderedMap
     *
     *  Thread-safe ordered map for cases where key ordering is important.
     *  Uses std::map internally for ordered iteration.
     *
     **/
    template <typename KeyType, typename ValueType, typename Compare = std::less<KeyType>>
    class ConcurrentOrderedMap
    {
    private:
        /** Internal storage **/
        std::map<KeyType, ValueType, Compare> mapData;

        /** Reader-writer lock for concurrent access **/
        mutable std::shared_mutex MUTEX;

    public:
        /** Default Constructor **/
        ConcurrentOrderedMap() = default;

        /** Destructor **/
        ~ConcurrentOrderedMap() = default;

        /** Insert
         *
         *  Insert a key-value pair. Returns false if key already exists.
         *
         **/
        bool Insert(const KeyType& key, const ValueType& value)
        {
            std::unique_lock<std::shared_mutex> lock(MUTEX);
            auto result = mapData.insert({key, value});
            return result.second;
        }

        /** InsertOrUpdate
         *
         *  Insert a key-value pair or update if key exists.
         *
         **/
        bool InsertOrUpdate(const KeyType& key, const ValueType& value)
        {
            std::unique_lock<std::shared_mutex> lock(MUTEX);
            auto it = mapData.find(key);
            if(it == mapData.end())
            {
                mapData.insert({key, value});
                return true;
            }
            it->second = value;
            return false;
        }

        /** Get
         *
         *  Get a value by key.
         *
         **/
        std::optional<ValueType> Get(const KeyType& key) const
        {
            std::shared_lock<std::shared_mutex> lock(MUTEX);
            auto it = mapData.find(key);
            if(it == mapData.end())
                return std::nullopt;
            return it->second;
        }

        /** Contains
         *
         *  Check if a key exists.
         *
         **/
        bool Contains(const KeyType& key) const
        {
            std::shared_lock<std::shared_mutex> lock(MUTEX);
            return mapData.find(key) != mapData.end();
        }

        /** Erase
         *
         *  Remove a key-value pair.
         *
         **/
        bool Erase(const KeyType& key)
        {
            std::unique_lock<std::shared_mutex> lock(MUTEX);
            return mapData.erase(key) > 0;
        }

        /** Size
         *
         *  Get the number of elements.
         *
         **/
        size_t Size() const
        {
            std::shared_lock<std::shared_mutex> lock(MUTEX);
            return mapData.size();
        }

        /** Clear
         *
         *  Remove all elements.
         *
         **/
        void Clear()
        {
            std::unique_lock<std::shared_mutex> lock(MUTEX);
            mapData.clear();
        }

        /** GetAll
         *
         *  Get a snapshot of all values.
         *
         **/
        std::vector<ValueType> GetAll() const
        {
            std::shared_lock<std::shared_mutex> lock(MUTEX);
            std::vector<ValueType> values;
            values.reserve(mapData.size());
            for(const auto& pair : mapData)
                values.push_back(pair.second);
            return values;
        }
    };

} // namespace util

#endif
