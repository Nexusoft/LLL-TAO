/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLD_CONFIG_HASHMAP_H
#define NEXUS_LLD_CONFIG_HASHMAP_H

#include <cstdint>
#include <mutex>

#include <LLD/hash/xxhash.h>
#include <LLD/config/base.h>

namespace LLD::Config
{
    /** Structure to contain the configuration variables for a BinaryHashMap Keychain. **/
    class Hashmap : public Base
    {
    public:

        /** The maximum number of hashmap files per shard. **/
        uint32_t MAX_HASHMAPS;


        /** The mininum number of buckets to probe per key. **/
        uint32_t MIN_LINEAR_PROBES;


        /** The maximum number of buckets to probe per key. **/
        uint32_t MAX_LINEAR_PROBES;


        /** Value used to break from potentially large looping if anything significant fails (never happened, but here as a breaker). **/
        uint32_t MAX_LINEAR_PROBE_CYCLES;


        /** Total number of files for each hashmap. **/
        uint16_t MAX_FILES_PER_HASHMAP;


        /** Maximum number of open file streams. **/
        uint16_t MAX_HASHMAP_FILE_STREAMS;


        /** Total number of files for each hashmap. **/
        uint16_t MAX_FILES_PER_INDEX;


        /** Maximum number of open file streams. **/
        uint16_t MAX_INDEX_FILE_STREAMS;


        /** The number of bits per k-hash, higher value is more accurate filter. **/
        uint32_t PRIMARY_BLOOM_ACCURACY;


        /** The number of k-hashes for the primary bloom filter. **/
        uint32_t PRIMARY_BLOOM_HASHES;


        /** The number of bits in the primary bloom filter. **/
        uint64_t PRIMARY_BLOOM_BITS;


        /** The number of k-hashes for the secondary bloom filter. **/
        uint32_t SECONDARY_BLOOM_HASHES;


        /** The number of bits per secondary bloom filter. **/
        uint32_t SECONDARY_BLOOM_BITS;


        /** The Maximum buckets allowed in the hashmap. **/
        uint64_t HASHMAP_TOTAL_BUCKETS;


        /** The total space that a key consumes. **/
        const uint16_t HASHMAP_KEY_ALLOCATION;


        /** Flag to determine if initialization should be fast or slow. **/
        bool QUICK_INIT;


        /** No empty constructor. **/
        Hashmap() = delete;


        /** Default constructor uses optimum values for bloom filters. **/
        Hashmap(const Base& base)
        : Base                     (base)
        , MAX_HASHMAPS             (256)
        , MIN_LINEAR_PROBES        (1)   //default: of 1 linear probes before moving to next hashmap file
        , MAX_LINEAR_PROBES        (144) //default: of 144 linear probes before moving to next hashmap file
        , MAX_LINEAR_PROBE_CYCLES  (0)   //default: automatically derived from our min/max probing ranges as a safety
        , MAX_FILES_PER_HASHMAP    (4)   //default: 4 files per hashmap
        , MAX_HASHMAP_FILE_STREAMS (MAX_HASHMAPS) //default: maximum hashmap files, otherwise you will degrade performance
        , MAX_FILES_PER_INDEX      (4)   //default: 4 files per index
        , MAX_INDEX_FILE_STREAMS   (MAX_FILES_PER_INDEX) //default: of 1:1 for best performance
        , PRIMARY_BLOOM_ACCURACY   (144) //default: this results in a multiplier of 1.44 bits per k-hash
        , PRIMARY_BLOOM_HASHES     (7)
        , PRIMARY_BLOOM_BITS       ((PRIMARY_BLOOM_ACCURACY / 100.0) * MAX_HASHMAPS * PRIMARY_BLOOM_HASHES)
        , SECONDARY_BLOOM_HASHES   (7)
        , SECONDARY_BLOOM_BITS     (13)
        , HASHMAP_TOTAL_BUCKETS    (77773)
        , HASHMAP_KEY_ALLOCATION   (16 + 13) //constant: 16 bytes for key checksum, 13 bytes for ckey class
        , QUICK_INIT               (true)    //default: this only really gives us total keys output and makes startup a lot slower
        , INDEX_LOCKS              (MAX_INDEX_FILE_STREAMS)
        , HASHMAP_LOCKS            (MAX_HASHMAP_FILE_STREAMS)
        {
        }


        /** Copy Constructor. **/
        Hashmap(const Hashmap& map)
        : Base                     (map)
        , MAX_HASHMAPS             (map.MAX_HASHMAPS)
        , MIN_LINEAR_PROBES        (map.MIN_LINEAR_PROBES)
        , MAX_LINEAR_PROBES        (map.MAX_LINEAR_PROBES)
        , MAX_LINEAR_PROBE_CYCLES  (map.MAX_LINEAR_PROBE_CYCLES)
        , MAX_FILES_PER_HASHMAP    (map.MAX_FILES_PER_HASHMAP)
        , MAX_HASHMAP_FILE_STREAMS (map.MAX_HASHMAP_FILE_STREAMS)
        , MAX_FILES_PER_INDEX      (map.MAX_FILES_PER_INDEX)
        , MAX_INDEX_FILE_STREAMS   (map.MAX_INDEX_FILE_STREAMS)
        , PRIMARY_BLOOM_ACCURACY   (map.PRIMARY_BLOOM_ACCURACY)
        , PRIMARY_BLOOM_HASHES     (map.PRIMARY_BLOOM_HASHES)
        , PRIMARY_BLOOM_BITS       (map.PRIMARY_BLOOM_BITS)
        , SECONDARY_BLOOM_HASHES   (map.SECONDARY_BLOOM_HASHES)
        , SECONDARY_BLOOM_BITS     (map.SECONDARY_BLOOM_BITS)
        , HASHMAP_TOTAL_BUCKETS    (map.HASHMAP_TOTAL_BUCKETS)
        , HASHMAP_KEY_ALLOCATION   (map.HASHMAP_KEY_ALLOCATION)
        , QUICK_INIT               (map.QUICK_INIT)
        , INDEX_LOCKS              ( )
        , HASHMAP_LOCKS            ( )
        {
            auto_config();
        }


        /** Move Constructor. **/
        Hashmap(Hashmap&& map)
        : Base                     (std::move(map))
        , MAX_HASHMAPS             (std::move(map.MAX_HASHMAPS))
        , MIN_LINEAR_PROBES        (std::move(map.MIN_LINEAR_PROBES))
        , MAX_LINEAR_PROBES        (std::move(map.MAX_LINEAR_PROBES))
        , MAX_LINEAR_PROBE_CYCLES  (std::move(map.MAX_LINEAR_PROBE_CYCLES))
        , MAX_FILES_PER_HASHMAP    (std::move(map.MAX_FILES_PER_HASHMAP))
        , MAX_HASHMAP_FILE_STREAMS (std::move(map.MAX_HASHMAP_FILE_STREAMS))
        , MAX_FILES_PER_INDEX      (std::move(map.MAX_FILES_PER_INDEX))
        , MAX_INDEX_FILE_STREAMS   (std::move(map.MAX_INDEX_FILE_STREAMS))
        , PRIMARY_BLOOM_ACCURACY   (std::move(map.PRIMARY_BLOOM_ACCURACY))
        , PRIMARY_BLOOM_HASHES     (std::move(map.PRIMARY_BLOOM_HASHES))
        , PRIMARY_BLOOM_BITS       (std::move(map.PRIMARY_BLOOM_BITS))
        , SECONDARY_BLOOM_HASHES   (std::move(map.SECONDARY_BLOOM_HASHES))
        , SECONDARY_BLOOM_BITS     (std::move(map.SECONDARY_BLOOM_BITS))
        , HASHMAP_TOTAL_BUCKETS    (std::move(map.HASHMAP_TOTAL_BUCKETS))
        , HASHMAP_KEY_ALLOCATION   (std::move(map.HASHMAP_KEY_ALLOCATION))
        , QUICK_INIT               (std::move(map.QUICK_INIT))
        , INDEX_LOCKS              ( )
        , HASHMAP_LOCKS            ( )
        {
            /* Refresh our configuration values. */
            auto_config();
        }


        /** Copy Assignment **/
        Hashmap& operator=(const Hashmap& map)
        {
            /* Database configuration. */
            DIRECTORY                = map.DIRECTORY;
            NAME                     = map.NAME;
            FLAGS                    = map.FLAGS;

            /* Hashmap configuration. */
            MAX_HASHMAPS             = map.MAX_HASHMAPS;
            MIN_LINEAR_PROBES        = map.MIN_LINEAR_PROBES;
            MAX_LINEAR_PROBES        = map.MAX_LINEAR_PROBES;
            MAX_LINEAR_PROBE_CYCLES  = map.MAX_LINEAR_PROBE_CYCLES;
            MAX_FILES_PER_HASHMAP    = map.MAX_FILES_PER_HASHMAP;
            MAX_HASHMAP_FILE_STREAMS = map.MAX_HASHMAP_FILE_STREAMS;
            MAX_FILES_PER_INDEX      = map.MAX_FILES_PER_INDEX;
            MAX_INDEX_FILE_STREAMS   = map.MAX_INDEX_FILE_STREAMS;
            PRIMARY_BLOOM_ACCURACY   = map.PRIMARY_BLOOM_ACCURACY;
            PRIMARY_BLOOM_HASHES     = map.PRIMARY_BLOOM_HASHES;
            PRIMARY_BLOOM_BITS       = map.PRIMARY_BLOOM_BITS;
            SECONDARY_BLOOM_HASHES   = map.SECONDARY_BLOOM_HASHES;
            SECONDARY_BLOOM_BITS     = map.SECONDARY_BLOOM_BITS;
            HASHMAP_TOTAL_BUCKETS    = map.HASHMAP_TOTAL_BUCKETS;

            /* We want to ensure our mutex lists are initialized here. */
            INDEX_LOCKS              = std::vector<std::mutex>(MAX_INDEX_FILE_STREAMS);
            HASHMAP_LOCKS            = std::vector<std::mutex>(MAX_HASHMAP_FILE_STREAMS);

            /* Refresh our configuration values. */
            auto_config();

            return *this;
        }


        /** Move Assignment **/
        Hashmap& operator=(Hashmap&& map)
        {
            /* Database configuration. */
            DIRECTORY                = std::move(map.DIRECTORY);
            NAME                     = std::move(map.NAME);
            FLAGS                    = std::move(map.FLAGS);

            /* Hashmap configuration. */
            MAX_HASHMAPS             = std::move(map.MAX_HASHMAPS);
            MIN_LINEAR_PROBES        = std::move(map.MIN_LINEAR_PROBES);
            MAX_LINEAR_PROBES        = std::move(map.MAX_LINEAR_PROBES);
            MAX_LINEAR_PROBE_CYCLES  = std::move(map.MAX_LINEAR_PROBE_CYCLES);
            MAX_FILES_PER_HASHMAP    = std::move(map.MAX_FILES_PER_HASHMAP);
            MAX_HASHMAP_FILE_STREAMS = std::move(map.MAX_HASHMAP_FILE_STREAMS);
            MAX_FILES_PER_INDEX      = std::move(map.MAX_FILES_PER_INDEX);
            MAX_INDEX_FILE_STREAMS   = std::move(map.MAX_INDEX_FILE_STREAMS);
            PRIMARY_BLOOM_ACCURACY   = std::move(map.PRIMARY_BLOOM_ACCURACY);
            PRIMARY_BLOOM_HASHES     = std::move(map.PRIMARY_BLOOM_HASHES);
            PRIMARY_BLOOM_BITS       = std::move(map.PRIMARY_BLOOM_BITS);
            SECONDARY_BLOOM_HASHES   = std::move(map.SECONDARY_BLOOM_HASHES);
            SECONDARY_BLOOM_BITS     = std::move(map.SECONDARY_BLOOM_BITS);
            HASHMAP_TOTAL_BUCKETS    = std::move(map.HASHMAP_TOTAL_BUCKETS);

            /* We want to ensure our mutex lists are initialized here. */
            INDEX_LOCKS              = std::vector<std::mutex>(MAX_INDEX_FILE_STREAMS);
            HASHMAP_LOCKS            = std::vector<std::mutex>(MAX_HASHMAP_FILE_STREAMS);

            /* Refresh our configuration values. */
            auto_config();

            return *this;
        }


        /** Default Destructor. **/
        ~Hashmap()
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
        std::mutex& INDEX(const uint32_t nFile) const
        {
            /* Calculate the lock that will be obtained by the given key. */
            uint64_t nLock = XXH3_64bits_withSeed((uint8_t*)&nFile, 4, 0) % INDEX_LOCKS.size();
            return INDEX_LOCKS[nLock]; //we need to use modulus here in case MAX_INDEX_FILE_STREAMS is smaller than total files
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
        std::mutex& HASHMAP(const uint32_t nHashmap, const uint32_t nFile) const
        {
            /* Calculate the lock that will be obtained by the given key. */
            uint64_t nLock = XXH3_64bits_withSeed((uint8_t*)&nHashmap, 4, nFile) % HASHMAP_LOCKS.size();
            return HASHMAP_LOCKS[nLock];
        }

    private:

        /** auto_config
         *
         *  Updates the automatic configuration values for the hashmap object.
         *
         *  We can guarentee that when this config is either moved or copied, that it is in the
         *  constructor of the LLD, and thus in its final form before being consumed. This allows us
         *  to auto-configure some dynamic states that require previous static configurations.
         *
         **/
        void auto_config()
        {
            /* Calculate our current probing range. */
            uint32_t nBeginProbeExpansion = MIN_LINEAR_PROBES, nEndProbeExpansion = MIN_LINEAR_PROBES;
            while(nEndProbeExpansion < MAX_LINEAR_PROBES)
            {
                /* Cache our previous probe begin. */
                const uint32_t nLastProbeBegin = nBeginProbeExpansion;

                /* Calculate our fibanacci expansion value. */
                nBeginProbeExpansion = nEndProbeExpansion;
                nEndProbeExpansion  += nLastProbeBegin;

                /* Increment our probing cycle. */
                ++MAX_LINEAR_PROBE_CYCLES;
            }

            /* Give one last increment to ensure full fibanacci ranges. */
            ++MAX_LINEAR_PROBE_CYCLES;

            /* Calculate our primary bloom filter number of bits. */
            PRIMARY_BLOOM_BITS = (PRIMARY_BLOOM_ACCURACY / 100.0) * MAX_HASHMAPS * PRIMARY_BLOOM_HASHES;

            /* Check for maximum limits. */
            check_limits<uint16_t>(PARAMS(MAX_HASHMAPS));
            check_limits<uint32_t>(PARAMS(MIN_LINEAR_PROBES));
            check_limits<uint32_t>(PARAMS(MAX_LINEAR_PROBES));
            check_limits<uint16_t>(PARAMS(MAX_LINEAR_PROBE_CYCLES));
            check_limits<uint32_t>(PARAMS(PRIMARY_BLOOM_BITS));   //primary bloom shouldn't exceed our range of 32-bits
            check_limits<uint16_t>(PARAMS(SECONDARY_BLOOM_BITS)); //a secondary bloom filter shouldn't ever have 65k bits
            check_limits<uint32_t>(PARAMS(HASHMAP_TOTAL_BUCKETS));

            /* Check for maximum ranges. */
            check_ranges<uint16_t>(PARAMS(MAX_FILES_PER_HASHMAP),     999); //we only provide 3 digits in the filenames, so 999 is max
            check_ranges<uint16_t>(PARAMS(MAX_HASHMAP_FILE_STREAMS), 9999);
            check_ranges<uint16_t>(PARAMS(MAX_FILES_PER_INDEX),       999); //we only provide 3 digits in the filenames, so 999 is max
            check_ranges<uint16_t>(PARAMS(MAX_INDEX_FILE_STREAMS),    999);
            check_ranges<uint32_t>(PARAMS(PRIMARY_BLOOM_ACCURACY),   1000); //this is a maximum of 10 bits per key
            check_ranges<uint16_t>(PARAMS(PRIMARY_BLOOM_HASHES),      128); //128 k-hashes should be enough
            check_ranges<uint16_t>(PARAMS(SECONDARY_BLOOM_HASHES),    128); //128 k-hashes is really excessive, but alas you can go that far

            /* We want to ensure our mutex lists are initialized here. */
            INDEX_LOCKS          = std::vector<std::mutex>(MAX_INDEX_FILE_STREAMS);
            HASHMAP_LOCKS        = std::vector<std::mutex>(MAX_HASHMAP_FILE_STREAMS);

            /* Give a warning if quick init is not enabled. */
            if(!QUICK_INIT)
                debug::warning(FUNCTION, "Quick Initialization Disabled, startup may take some time");
        }


        /** The keychain level locking hashmap. **/
        mutable std::vector<std::mutex> INDEX_LOCKS;


        /** The keychain level locking hashmap. **/
        mutable std::vector<std::mutex> HASHMAP_LOCKS;

    };
}

#endif
