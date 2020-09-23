/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLD_CONFIG_SECTOR_H
#define NEXUS_LLD_CONFIG_SECTOR_H

#include <cstdint>

namespace LLD::Config
{
    /** Structure to contain the configuration variables for the sector database object. **/
    class Sector
    {
    public:

        /** The total number of open file streams. **/
        uint32_t MAX_SECTOR_FILE_STREAMS;


        /** The maximum cache size for sector database. **/
        uint64_t MAX_SECTOR_CACHE_SIZE;


        /** Maximum size a file can be in the datachain. **/
        uint64_t MAX_SECTOR_FILE_SIZE;


        /** Maximum size that the buffer can grow to before being flushed to disk. **/
        uint64_t MAX_SECTOR_BUFFER_SIZE;


        /** The string to hold the database location. **/
        std::string BASE_DIRECTORY;


        /** The string to hold the database name. **/
        std::string DB_NAME;


        /** The keychain level flags. **/
        uint8_t FLAGS;


        /** The sector level locking hashmap. **/
        mutable std::vector<std::mutex> SECTOR_LOCKS;


        /** The keychain level locking hashmap. **/
        mutable std::vector<std::mutex> KEYCHAIN_LOCKS;


        /** No default constructor. **/
        Sector() = delete;


        /** Copy Constructor. **/
        Sector(const Sector& map)
        : MAX_SECTOR_FILE_STREAMS (map.MAX_SECTOR_FILE_STREAMS)
        , MAX_SECTOR_CACHE_SIZE   (map.MAX_SECTOR_CACHE_SIZE)
        , MAX_SECTOR_FILE_SIZE    (map.MAX_SECTOR_FILE_SIZE)
        , MAX_SECTOR_BUFFER_SIZE  (map.MAX_SECTOR_BUFFER_SIZE)
        , BASE_DIRECTORY          (map.BASE_DIRECTORY)
        , DB_NAME                 (map.DB_NAME)
        , FLAGS                   (map.FLAGS)
        , SECTOR_LOCKS            (map.SECTOR_LOCKS.size())
        , KEYCHAIN_LOCKS          (map.KEYCHAIN_LOCKS.size())
        {
        }


        /** Move Constructor. **/
        Sector(Sector&& map)
        : MAX_SECTOR_FILE_STREAMS (std::move(map.MAX_SECTOR_FILE_STREAMS))
        , MAX_SECTOR_CACHE_SIZE   (std::move(map.MAX_SECTOR_CACHE_SIZE))
        , MAX_SECTOR_FILE_SIZE    (std::move(map.MAX_SECTOR_FILE_SIZE))
        , MAX_SECTOR_BUFFER_SIZE  (std::move(map.MAX_SECTOR_BUFFER_SIZE))
        , BASE_DIRECTORY          (std::move(map.BASE_DIRECTORY))
        , DB_NAME                 (std::move(map.DB_NAME))
        , FLAGS                   (std::move(map.FLAGS))
        , SECTOR_LOCKS            (std::move(map.SECTOR_LOCKS.size()))
        , KEYCHAIN_LOCKS          (map.KEYCHAIN_LOCKS.size())
        {
        }


        /** Copy Assignment **/
        Sector& operator=(const Sector& map)
        {
            MAX_SECTOR_FILE_STREAMS = map.MAX_SECTOR_FILE_STREAMS;
            MAX_SECTOR_CACHE_SIZE   = map.MAX_SECTOR_CACHE_SIZE;
            MAX_SECTOR_FILE_SIZE    = map.MAX_SECTOR_FILE_SIZE;
            MAX_SECTOR_BUFFER_SIZE  = map.MAX_SECTOR_BUFFER_SIZE;

            BASE_DIRECTORY          = map.BASE_DIRECTORY;
            DB_NAME                 = map.DB_NAME;
            FLAGS                   = map.FLAGS;

            return *this;
        }


        /** Move Assignment **/
        Sector& operator=(Sector&& map)
        {
            MAX_SECTOR_FILE_STREAMS = std::move(map.MAX_SECTOR_FILE_STREAMS);
            MAX_SECTOR_CACHE_SIZE   = std::move(map.MAX_SECTOR_CACHE_SIZE);
            MAX_SECTOR_FILE_SIZE    = std::move(map.MAX_SECTOR_FILE_SIZE);
            MAX_SECTOR_BUFFER_SIZE  = std::move(map.MAX_SECTOR_BUFFER_SIZE);

            BASE_DIRECTORY          = std::move(map.BASE_DIRECTORY);
            DB_NAME                 = std::move(map.DB_NAME);
            FLAGS                   = std::move(map.FLAGS);

            return *this;
        }


        /** Destructor. **/
        ~Sector()
        {
        }


        /** Required Constructor. **/
        Sector(const std::string& strName, const uint8_t nFlags)
        : MAX_SECTOR_FILE_STREAMS (8)
        , MAX_SECTOR_CACHE_SIZE   (1024 * 1024)       //1 MB of cache default
        , MAX_SECTOR_FILE_SIZE    (1024 * 1024 * 512) //512 MB max per sector file
        , MAX_SECTOR_BUFFER_SIZE  (1024 * 1024 * 4)   //4 MB max disk buffer
        , BASE_DIRECTORY          (config::GetDataDir() + strName + "/")
        , DB_NAME                 (strName)
        , FLAGS                   (nFlags)
        , SECTOR_LOCKS            (1024)
        , KEYCHAIN_LOCKS          (1024)
        {
        }


        /** KeychainLock
         *
         *  Grabs a lock from the set of keychain locks by key data.
         *
         *  @param[in] vKey The binary data of key to lock for.
         *
         *  @return a reference of the lock object.
         *
         **/
        std::mutex& KEYCHAIN(const std::vector<uint8_t>& vKey) const
        {
            /* Calculate the lock that will be obtained by the given key. */
            uint64_t nLock = XXH3_64bits((uint8_t*)&vKey[0], vKey.size()) % KEYCHAIN_LOCKS.size();
            return KEYCHAIN_LOCKS[nLock];
        }


        /** File
         *
         *  Grabs a lock from the set of keychain locks by file handle.
         *
         *  @param[in] nFile The binary data of key to lock for.
         *
         *  @return a reference of the lock object.
         *
         **/
        std::mutex& FILE(const uint32_t nFile) const
        {
            /* Calculate the lock that will be obtained by the given key. */
            uint64_t nLock = XXH3_64bits((uint8_t*)&nFile, 4) % KEYCHAIN_LOCKS.size();
            return KEYCHAIN_LOCKS[nLock];
        }


        /** SectorLock
         *
         *  Grabs a lock from the set of sector locks by sector position.
         *
         *  @param[in] key The sector location to lock for.
         *
         *  @return a reference of the lock object.
         *
         **/
        std::mutex& SECTOR(const SectorKey& key) const
        {
            /* Calculate the lock that will be obtained by the given key. */
            uint64_t nLock = ((key.nSectorFile + 1) * (key.nSectorStart + 1)) % SECTOR_LOCKS.size();
            return KEYCHAIN_LOCKS[nLock];
        }
    };
}

#endif
