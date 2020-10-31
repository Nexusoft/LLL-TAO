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
#include <mutex>

#include <LLD/hash/xxhash.h>
#include <LLD/templates/key.h>
#include <LLD/config/base.h>

namespace LLD::Config
{
    /** Structure to contain the configuration variables for the sector database object. **/
    class Sector : public Base
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


        /** No empty constructor. **/
        Sector() = delete;


        /** Required Constructor. **/
        Sector(const Base& base)
        : Base                    (base)
        , MAX_SECTOR_FILE_STREAMS (8)
        , MAX_SECTOR_CACHE_SIZE   (1024 * 1024)       //1 MB of cache default
        , MAX_SECTOR_FILE_SIZE    (1024 * 1024 * 512) //512 MB max per sector file
        , MAX_SECTOR_BUFFER_SIZE  (1024 * 1024 * 4)   //4 MB max disk buffer
        , SECTOR_LOCKS            (1024)
        , FILESYSTEM_LOCKS        (MAX_SECTOR_FILE_STREAMS)
        {
        }


        /** Copy Constructor. **/
        Sector(const Sector& map)
        : Base                    (map)
        , MAX_SECTOR_FILE_STREAMS (map.MAX_SECTOR_FILE_STREAMS)
        , MAX_SECTOR_CACHE_SIZE   (map.MAX_SECTOR_CACHE_SIZE)
        , MAX_SECTOR_FILE_SIZE    (map.MAX_SECTOR_FILE_SIZE)
        , MAX_SECTOR_BUFFER_SIZE  (map.MAX_SECTOR_BUFFER_SIZE)
        , SECTOR_LOCKS            (map.SECTOR_LOCKS.size())
        , FILESYSTEM_LOCKS        (map.FILESYSTEM_LOCKS.size())
        {
        }


        /** Move Constructor. **/
        Sector(Sector&& map)
        : Base                    (std::move(map))
        , MAX_SECTOR_FILE_STREAMS (std::move(map.MAX_SECTOR_FILE_STREAMS))
        , MAX_SECTOR_CACHE_SIZE   (std::move(map.MAX_SECTOR_CACHE_SIZE))
        , MAX_SECTOR_FILE_SIZE    (std::move(map.MAX_SECTOR_FILE_SIZE))
        , MAX_SECTOR_BUFFER_SIZE  (std::move(map.MAX_SECTOR_BUFFER_SIZE))
        , SECTOR_LOCKS            (map.SECTOR_LOCKS.size())
        , FILESYSTEM_LOCKS        (map.FILESYSTEM_LOCKS.size())
        {
        }


        /** Copy Assignment **/
        Sector& operator=(const Sector& map)
        {
            /* Database configuration. */
            DIRECTORY               = map.DIRECTORY;
            NAME                    = map.NAME;
            FLAGS                   = map.FLAGS;

            /* Sector configuration.  */
            MAX_SECTOR_FILE_STREAMS = map.MAX_SECTOR_FILE_STREAMS;
            MAX_SECTOR_CACHE_SIZE   = map.MAX_SECTOR_CACHE_SIZE;
            MAX_SECTOR_FILE_SIZE    = map.MAX_SECTOR_FILE_SIZE;
            MAX_SECTOR_BUFFER_SIZE  = map.MAX_SECTOR_BUFFER_SIZE;

            return *this;
        }


        /** Move Assignment **/
        Sector& operator=(Sector&& map)
        {
            /* Database configuration. */
            DIRECTORY               = std::move(map.DIRECTORY);
            NAME                    = std::move(map.NAME);
            FLAGS                   = std::move(map.FLAGS);

            /* Sector configuration.  */
            MAX_SECTOR_FILE_STREAMS = std::move(map.MAX_SECTOR_FILE_STREAMS);
            MAX_SECTOR_CACHE_SIZE   = std::move(map.MAX_SECTOR_CACHE_SIZE);
            MAX_SECTOR_FILE_SIZE    = std::move(map.MAX_SECTOR_FILE_SIZE);
            MAX_SECTOR_BUFFER_SIZE  = std::move(map.MAX_SECTOR_BUFFER_SIZE);

            return *this;
        }


        /** Destructor. **/
        ~Sector()
        {
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
            return SECTOR_LOCKS[nLock];
        }


        /** File
         *
         *  Grabs a lock from the set of sector locks by file handle.
         *
         *  @param[in] nFile The binary data of key to lock for.
         *
         *  @return a reference of the lock object.
         *
         **/
        std::mutex& FILE(const uint32_t nFile) const
        {
            /* Calculate the lock that will be obtained by the given key. */
            uint64_t nLock = XXH3_64bits((uint8_t*)&nFile, 4) % FILESYSTEM_LOCKS.size();
            return FILESYSTEM_LOCKS[nLock];
        }

    private:


        /** The sector level locking hashmap. **/
        mutable std::vector<std::mutex> SECTOR_LOCKS;


        /** The keychain level locking hashmap. **/
        mutable std::vector<std::mutex> FILESYSTEM_LOCKS;
    };
}

#endif
