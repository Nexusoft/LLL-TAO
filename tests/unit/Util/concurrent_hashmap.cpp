/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <unit/catch2/catch.hpp>

#include <Util/templates/concurrent_hashmap.h>

#include <thread>
#include <vector>
#include <atomic>

using namespace util;


TEST_CASE("ConcurrentHashMap Basic Operations", "[concurrent_hashmap]")
{
    SECTION("Default constructor creates empty map")
    {
        ConcurrentHashMap<std::string, int> map;
        
        REQUIRE(map.Empty() == true);
        REQUIRE(map.Size() == 0);
    }
    
    SECTION("Insert adds new key-value pair")
    {
        ConcurrentHashMap<std::string, int> map;
        
        bool result = map.Insert("key1", 100);
        
        REQUIRE(result == true);
        REQUIRE(map.Size() == 1);
        REQUIRE(map.Contains("key1") == true);
    }
    
    SECTION("Insert fails for duplicate key")
    {
        ConcurrentHashMap<std::string, int> map;
        
        map.Insert("key1", 100);
        bool result = map.Insert("key1", 200);
        
        REQUIRE(result == false);
        REQUIRE(map.Get("key1").value() == 100);  // Original value preserved
    }
    
    SECTION("InsertOrUpdate inserts new key")
    {
        ConcurrentHashMap<std::string, int> map;
        
        bool wasInsert = map.InsertOrUpdate("key1", 100);
        
        REQUIRE(wasInsert == true);
        REQUIRE(map.Get("key1").value() == 100);
    }
    
    SECTION("InsertOrUpdate updates existing key")
    {
        ConcurrentHashMap<std::string, int> map;
        
        map.Insert("key1", 100);
        bool wasInsert = map.InsertOrUpdate("key1", 200);
        
        REQUIRE(wasInsert == false);
        REQUIRE(map.Get("key1").value() == 200);
    }
    
    SECTION("Get returns nullopt for missing key")
    {
        ConcurrentHashMap<std::string, int> map;
        
        auto result = map.Get("missing");
        
        REQUIRE(!result.has_value());
    }
    
    SECTION("Erase removes existing key")
    {
        ConcurrentHashMap<std::string, int> map;
        
        map.Insert("key1", 100);
        bool result = map.Erase("key1");
        
        REQUIRE(result == true);
        REQUIRE(map.Contains("key1") == false);
    }
    
    SECTION("Erase returns false for missing key")
    {
        ConcurrentHashMap<std::string, int> map;
        
        bool result = map.Erase("missing");
        
        REQUIRE(result == false);
    }
    
    SECTION("Clear removes all elements")
    {
        ConcurrentHashMap<std::string, int> map;
        
        map.Insert("key1", 1);
        map.Insert("key2", 2);
        map.Insert("key3", 3);
        
        map.Clear();
        
        REQUIRE(map.Empty() == true);
        REQUIRE(map.Size() == 0);
    }
    
    SECTION("GetAll returns all values")
    {
        ConcurrentHashMap<std::string, int> map;
        
        map.Insert("key1", 1);
        map.Insert("key2", 2);
        map.Insert("key3", 3);
        
        auto values = map.GetAll();
        
        REQUIRE(values.size() == 3);
    }
    
    SECTION("GetAndRemove atomically gets and removes")
    {
        ConcurrentHashMap<std::string, int> map;
        
        map.Insert("key1", 100);
        
        auto result = map.GetAndRemove("key1");
        
        REQUIRE(result.has_value());
        REQUIRE(result.value() == 100);
        REQUIRE(map.Contains("key1") == false);
    }
}


TEST_CASE("ConcurrentHashMap Thread Safety", "[concurrent_hashmap]")
{
    SECTION("Concurrent inserts work correctly")
    {
        ConcurrentHashMap<int, int> map;
        std::atomic<int> successCount(0);
        const int NUM_THREADS = 10;
        const int INSERTS_PER_THREAD = 100;
        
        std::vector<std::thread> threads;
        for(int t = 0; t < NUM_THREADS; ++t)
        {
            threads.emplace_back([&map, &successCount, t, INSERTS_PER_THREAD]() {
                for(int i = 0; i < INSERTS_PER_THREAD; ++i)
                {
                    int key = t * INSERTS_PER_THREAD + i;
                    if(map.Insert(key, key))
                        successCount++;
                }
            });
        }
        
        for(auto& thread : threads)
            thread.join();
        
        REQUIRE(successCount == NUM_THREADS * INSERTS_PER_THREAD);
        REQUIRE(map.Size() == static_cast<size_t>(NUM_THREADS * INSERTS_PER_THREAD));
    }
    
    SECTION("Concurrent reads and writes work correctly")
    {
        ConcurrentHashMap<int, int> map;
        std::atomic<bool> done(false);
        
        /* Pre-populate with some data */
        for(int i = 0; i < 100; ++i)
            map.Insert(i, i);
        
        /* Reader thread */
        std::thread reader([&map, &done]() {
            while(!done)
            {
                for(int i = 0; i < 100; ++i)
                {
                    auto val = map.Get(i);
                    /* Value might be updated by writer */
                }
            }
        });
        
        /* Writer thread */
        std::thread writer([&map, &done]() {
            for(int iter = 0; iter < 1000; ++iter)
            {
                for(int i = 0; i < 100; ++i)
                {
                    map.InsertOrUpdate(i, iter);
                }
            }
            done = true;
        });
        
        writer.join();
        reader.join();
        
        /* Should complete without deadlock or crash */
        REQUIRE(map.Size() == 100);
    }
}


TEST_CASE("ConcurrentOrderedMap Basic Operations", "[concurrent_hashmap]")
{
    SECTION("Default constructor creates empty map")
    {
        ConcurrentOrderedMap<std::string, int> map;
        
        REQUIRE(map.Size() == 0);
    }
    
    SECTION("Insert adds new key-value pair")
    {
        ConcurrentOrderedMap<std::string, int> map;
        
        bool result = map.Insert("key1", 100);
        
        REQUIRE(result == true);
        REQUIRE(map.Size() == 1);
    }
    
    SECTION("Get retrieves value")
    {
        ConcurrentOrderedMap<std::string, int> map;
        
        map.Insert("key1", 100);
        
        auto result = map.Get("key1");
        
        REQUIRE(result.has_value());
        REQUIRE(result.value() == 100);
    }
}
