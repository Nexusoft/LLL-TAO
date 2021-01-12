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

#include <shared_mutex>

namespace LLD::Config
{
    /** Structure to contain the configuration variables for the sector database object. **/
    class Static : public Base
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
        Static() = delete;


        /** Required Constructor. **/
        Static(const Base& base)
        : Base                    (base)
        , MAX_SECTOR_FILE_STREAMS (8)
        , MAX_SECTOR_CACHE_SIZE   (1024 * 1024)       //1 MB of cache default
        , MAX_SECTOR_FILE_SIZE    (1024 * 1024 * 512) //512 MB max per sector file
        , MAX_SECTOR_BUFFER_SIZE  (1024 * 1024 * 4)   //4 MB max disk buffer
        , FILESYSTEM_LOCKS        ( )
        {
        }


        /** Copy Constructor. **/
        Static(const Static& map)
        : Base                    (map)
        , MAX_SECTOR_FILE_STREAMS (map.MAX_SECTOR_FILE_STREAMS)
        , MAX_SECTOR_CACHE_SIZE   (map.MAX_SECTOR_CACHE_SIZE)
        , MAX_SECTOR_FILE_SIZE    (map.MAX_SECTOR_FILE_SIZE)
        , MAX_SECTOR_BUFFER_SIZE  (map.MAX_SECTOR_BUFFER_SIZE)
        , FILESYSTEM_LOCKS        ( )
        {
            /* Refresh our configuration values. */
            auto_config();
        }


        /** Move Constructor. **/
        Static(Static&& map)
        : Base                    (std::move(map))
        , MAX_SECTOR_FILE_STREAMS (std::move(map.MAX_SECTOR_FILE_STREAMS))
        , MAX_SECTOR_CACHE_SIZE   (std::move(map.MAX_SECTOR_CACHE_SIZE))
        , MAX_SECTOR_FILE_SIZE    (std::move(map.MAX_SECTOR_FILE_SIZE))
        , MAX_SECTOR_BUFFER_SIZE  (std::move(map.MAX_SECTOR_BUFFER_SIZE))
        , FILESYSTEM_LOCKS        ( )
        {
            /* Refresh our configuration values. */
            auto_config();
        }


        /** Copy Assignment **/
        Static& operator=(const Static& map)
        {
            /* Database configuration. */
            DIRECTORY               = map.DIRECTORY;
            NAME                    = map.NAME;
            FLAGS                   = map.FLAGS;

            /* Static configuration.  */
            MAX_SECTOR_FILE_STREAMS = map.MAX_SECTOR_FILE_STREAMS;
            MAX_SECTOR_CACHE_SIZE   = map.MAX_SECTOR_CACHE_SIZE;
            MAX_SECTOR_FILE_SIZE    = map.MAX_SECTOR_FILE_SIZE;
            MAX_SECTOR_BUFFER_SIZE  = map.MAX_SECTOR_BUFFER_SIZE;

            /* Refresh our configuration values. */
            auto_config();

            return *this;
        }


        /** Move Assignment **/
        Static& operator=(Static&& map)
        {
            /* Database configuration. */
            DIRECTORY               = std::move(map.DIRECTORY);
            NAME                    = std::move(map.NAME);
            FLAGS                   = std::move(map.FLAGS);

            /* Static configuration.  */
            MAX_SECTOR_FILE_STREAMS = std::move(map.MAX_SECTOR_FILE_STREAMS);
            MAX_SECTOR_CACHE_SIZE   = std::move(map.MAX_SECTOR_CACHE_SIZE);
            MAX_SECTOR_FILE_SIZE    = std::move(map.MAX_SECTOR_FILE_SIZE);
            MAX_SECTOR_BUFFER_SIZE  = std::move(map.MAX_SECTOR_BUFFER_SIZE);

            /* Refresh our configuration values. */
            auto_config();

            return *this;
        }


        /** Destructor. **/
        ~Static()
        {
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
            uint64_t nLock = XXH3_64bits_withSeed((uint8_t*)&nFile, 4, 0) % FILESYSTEM_LOCKS.size();
            return FILESYSTEM_LOCKS[nLock];
        }

    private:

        /** auto_config
         *
         *  Updates the automatic configuration values for the static database object.
         *
         *  We can guarentee that when this config is either moved or copied, that it is in the
         *  constructor of the LLD, and thus in its final form before being consumed. This allows us
         *  to auto-configure some dynamic states that require previous static configurations.
         *
         **/
        void auto_config()
        {
            /* Check our maximum limits. */
            check_limits<uint32_t>(PARAMS(MAX_SECTOR_CACHE_SIZE)); //we are limiting our cache to 4.3 GB for now
            check_limits<uint32_t>(PARAMS(MAX_SECTOR_FILE_SIZE));  //files shouldn't be larger than 4.3 GB
            check_limits<uint32_t>(PARAMS(MAX_SECTOR_BUFFER_SIZE)); //we don't need more than 4.3 GB of write buffer

            /* Check our maximum ranges. */
            check_ranges<uint32_t>(PARAMS(MAX_SECTOR_FILE_STREAMS), 9999); //TODO: we should check ulimit here

            /* We want to ensure our mutex lists are initialized here. */
            FILESYSTEM_LOCKS        = std::vector<std::mutex>(MAX_SECTOR_FILE_STREAMS);
        }


        /** The keychain level locking hashmap. **/
        mutable std::vector<std::mutex> FILESYSTEM_LOCKS;

    };
}

#endif
