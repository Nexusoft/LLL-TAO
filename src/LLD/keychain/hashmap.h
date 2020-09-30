/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLD_KEYCHAIN_HASHMAP_H
#define NEXUS_LLD_KEYCHAIN_HASHMAP_H

#include <LLD/keychain/keychain.h>
#include <LLD/cache/template_lru.h>
#include <LLD/include/enum.h>
#include <LLD/config/hashmap.h>

#include <cstdint>
#include <string>
#include <fstream>
#include <vector>
#include <mutex>

namespace LLD
{
    /** BinaryHashMap
     *
     *  This class is responsible for managing the keys to the sector database.
     *
     *  It contains a Binary Hash Map with a minimum complexity of O(1).
     *  It uses a linked file list based on index to iterate trhough files and binary Positions
     *  when there is a collision that is found.
     *
     **/
    class BinaryHashMap : public Keychain
    {
    protected:

        /** Mutex for Thread Synchronization. **/
        mutable std::mutex KEY_MUTEX;


        /** Internal Hashmap Config Object. **/
        const Config::Hashmap& CONFIG;


        /** Internal pre-calculated index size. **/
        const uint16_t INDEX_FILTER_SIZE;


        /** Keychain stream object. **/
        TemplateLRU<uint16_t, std::fstream*>* pFileStreams;


        /** Keychain index stream. **/
        std::fstream* pindex;


    public:


        /** Default Constructor. **/
        BinaryHashMap() = delete;


        /** The Database Constructor. To determine file location and the Bytes per Record. **/
        BinaryHashMap(const Config::Hashmap& config);


        /** Copy Constructor **/
        BinaryHashMap(const BinaryHashMap& map);


        /** Move Constructor **/
        BinaryHashMap(BinaryHashMap&& map);


        /** Copy Assignment Operator **/
        BinaryHashMap& operator=(const BinaryHashMap& map);


        /** Move Assignment Operator **/
        BinaryHashMap& operator=(BinaryHashMap&& map);


        /** Default Destructor **/
        virtual ~BinaryHashMap();


        /** Initialize
         *
         *  Initialize the binary hash map keychain.
         *
         **/
        void Initialize();


        /** Get
         *
         *  Read a key index from the disk hashmaps.
         *
         *  @param[in] vKey The binary data of key.
         *  @param[out] key The key object to return.
         *
         *  @return True if the key was found, false otherwise.
         *
         **/
        bool Get(const std::vector<uint8_t>& vKey, SectorKey &key);


        /** Put
         *
         *  Write a key to the disk hashmaps.
         *
         *  @param[in] key The key object to write.
         *
         *  @return True if the key was written, false otherwise.
         *
         **/
        bool Put(const SectorKey& key);


        /** Flush
         *
         *  Flush all buffers to disk if using ACID transaction.
         *
         **/
        void Flush();


        /** Restore
         *
         *  Restore an erased key from keychain.
         *
         *  @param[in] vKey the key to restore.
         *
         *  @return True if the key was restored.
         *
         **/
        bool Restore(const std::vector<uint8_t>& vKey);


        /** Erase
         *
         *  Erase a key from the disk hashmaps.
         *  TODO: This should be optimized further.
         *
         *  @param[in] vKey the key to erase.
         *
         *  @return True if the key was erased, false otherwise.
         *
         **/
        bool Erase(const std::vector<uint8_t>& vKey);


    private:

        /** compress_key
         *
         *  Compresses a given key until it matches size criteria.
         *  This function is one way and efficient for reducing key sizes.
         *
         *  @param[out] vKey The binary data of key to compress.
         *
         **/
        std::vector<uint8_t> compress_key(const std::vector<uint8_t>& vKey);


        /** get_bucket
         *
         *  Calculates a bucket to be used for the hashmap allocation.
         *
         *  @param[in] vKey The key object to calculate with.
         *
         *  @return The bucket assigned to the key.
         *
         **/
        uint32_t get_bucket(const std::vector<uint8_t>& vKey);


        /** get_file_stream
         *
         *  Find a file stream from the local LRU cache.
         *
         *  @param[in] nHashmap The file identifier we are searching for.
         *
         *  @return the file stream from the cache.
         *
         **/
        std::fstream* get_file_stream(const uint32_t nHashmap);


        /** flush_index
         *
         *  Flush the current buffer to disk.
         *
         *  @param[in] vBuffer The buffer to write to disk
         *  @param[in] nBucket The current bucket to flush.
         *  @param[in] nOffset The current bucket offset to flush
         *
         *  @return true if the index was flushed successfully
         *
         **/
        bool flush_index(const std::vector<uint8_t>& vBuffer, const uint32_t nBucket, const uint32_t nOffset = 0);


        /** find_and_write
         *
         *  Finds an available hashmap slot within the probing range and writes it to disk.
         *
         *  @param[in] key The current sector key to write
         *  @param[out] vBuffer The current index buffer to check and pass back by reference
         *  @param[out] nHashmap The file to start searching from.
         *  @param[out] nBucket The bucket we are writing for.
         *  @param[in] nProbes The total adjacent buckets to probe for.
         *
         *  @return true if the key was written
         *
         **/
        bool find_and_write(const SectorKey& key, std::vector<uint8_t> &vBuffer,
            uint16_t &nHashmap, uint32_t &nBucket, const uint32_t nProbes = 1);

        /** find_and_read
         *
         *  Finds a key within a hashmap with given probing ranges
         *
         *  @param[out] key The current sector key to write
         *  @param[in] vBuffer The current index buffer to check and pass back by reference
         *  @param[out] nHashmap The file to start searching from.
         *  @param[out] nBucket The bucket we are reading for.
         *  @param[in] nProbes The total adjacent buckets to probe for.
         *
         *  @return true if the key was found
         *
         **/
        bool find_and_read(SectorKey &key, const std::vector<uint8_t>& vBuffer,
            uint16_t &nHashmap, uint32_t &nBucket, const uint32_t nProbes = 1);


        /** get_current_file
         *
         *  Helper method to grab the current file from the input buffer.
         *
         *  @param[in] vBuffer The byte stream of input data
         *  @param[in] nOffset The starting offset in the stream.
         *
         *  @return The current hashmap file for given key index.
         *
         **/
        uint16_t get_current_file(const std::vector<uint8_t>& vBuffer, const uint32_t nOffset = 0);


        /** set_current_file
         *
         *  Helper method to set the current file in the input buffer.
         *
         *  @param[in] nCurrent The current file to set
         *  @param[out] vBuffer The byte stream of output data
         *  @param[in] nOffset The starting offset in the stream.
         *
         **/
        void set_current_file(const uint16_t nCurrent, std::vector<uint8_t> &vBuffer, const uint32_t nOffset = 0);


        /** check_hashmap_available
         *
         *  Helper method to check if a file is unoccupied in input buffer.
         *
         *  @param[in] nHashmap The current file to set
         *  @param[in] vBuffer The byte stream of input data
         *  @param[in] nOffset The starting offset in the stream.
         *
         **/
        bool check_hashmap_available(const uint32_t nHashmap, const std::vector<uint8_t>& vBuffer, const uint32_t nOffset = 0);


        /** set_hashmap_available
         *
         *  Helper method to set a file index as unoccupied.
         *
         *  @param[in] nHashmap The current file to set
         *  @param[out] vBuffer The byte stream of input data
         *  @param[in] nOffset The starting offset in the stream.
         *
         **/
        void set_hashmap_available(const uint32_t nHashmap, std::vector<uint8_t> &vBuffer, const uint32_t nOffset = 0);


        /** secondary_bloom_size
         *
         *  Helper method to get the total bytes in the secondary bloom filter.
         *
         *  @return The total bytes in secondary bloom filter.
         *
         **/
        uint32_t secondary_bloom_size();


        /** set_secondary_bloom
         *
         *  Set a given key is within a secondary bloom filter.
         *
         *  @param[in] vKey The key level data in bytes
         *  @param[out] vBuffer The buffer to write bloom filter into
         *  @param[in] nHashmap The hashmap file that we are writing for
         *  @param[in] nOffset The starting offset in the stream
         *
         *
         **/
        void set_secondary_bloom(const std::vector<uint8_t>& vKey, std::vector<uint8_t> &vBuffer, const uint32_t nHashmap, const uint32_t nOffset = 0);


        /** check_secondary_bloom
         *
         *  Check if a given key is within a secondary bloom filter.
         *
         *  @param[in] vKey The key level data in bytes
         *  @param[in] vBuffer The buffer to check bloom filter within
         *  @param[in] nHashmap The hashmap file that we are checking for
         *  @param[in] nOffset The starting offset in the stream
         *
         *  @return True if given key is in secondary bloom, false otherwise.
         *
         **/
        bool check_secondary_bloom(const std::vector<uint8_t>& vKey, const std::vector<uint8_t>& vBuffer, const uint32_t nHashmap, const uint32_t nOffset = 0);


        /** primary_bloom_size
         *
         *  Helper method to get the total bytes in the primary bloom filter.
         *
         *  @return The total bytes in primary bloom filter.
         *
         **/
        uint32_t primary_bloom_size();


        /** set_primary_bloom
         *
         *  Set a given key is within a primary bloom filter.
         *
         *  @param[in] vKey The key level data in bytes
         *  @param[out] vBuffer The buffer to write bloom filter into
         *  @param[in] nOffset The starting offset in the stream
         *
         *
         **/
        void set_primary_bloom(const std::vector<uint8_t>& vKey, std::vector<uint8_t> &vBuffer, const uint32_t nOffset = 0);


        /** check_primary_bloom
         *
         *  Check if a given key is within a primary bloom filter.
         *
         *  @param[in] vKey The key level data in bytes
         *  @param[in] vBuffer The buffer to check bloom filter within
         *  @param[in] nOffset The starting offset in the stream
         *
         *  @return True if given key is in secondary bloom, false otherwise.
         *
         **/
        bool check_primary_bloom(const std::vector<uint8_t>& vKey, const std::vector<uint8_t>& vBuffer, const uint32_t nOffset = 0);


        /** fibanacci_expansion
         *
         *  Expand a beginning and ending iterator based on fibanacci sequence.
         *
         *  @param[out] nBeginProbeExpansion The starting iterator of the fibanacci cycle.
         *  @param[out] nEndProbeExpansion The ending iterator of the fibanacci cycle.
         *  @param[in] nExpansionCycle The total number of fibanacci expansion cycles to calculate.
         *
         **/
        void fibanacci_expansion(uint32_t &nBeginProbeExpansion, uint32_t &nEndProbeExpansion, const uint32_t nExpansionCycles);
    };
}

#endif
