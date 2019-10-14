/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLD_TEMPLATES_FILEMAP_H
#define NEXUS_LLD_TEMPLATES_FILEMAP_H

#include <cstdint>
#include <string>
#include <map>
#include <mutex>
#include <vector>

namespace LLD
{
    /** Forward declarations **/
    class SectorKey;


    /* Maximum size a file can be in the keychain. */
    const uint32_t FILEMAP_MAX_FILE_SIZE = 1024 * 1024 * 1024; //1 GB per File


    /** BinaryFileMap
     *
     *  Binary File Map Database Class.
     *  Stores and Contains the Sector Keys to Access the Sector Database.
     *
     **/
    class BinaryFileMap
    {
    protected:

        /** Mutex for Thread Synchronization. **/
        mutable std::mutex KEY_MUTEX;


        /** The String to hold the Disk Location of Database File.
            Each Database File Acts as a New Table as in Conventional Design.
            Key can be any Type, which is how the Database Records are Accessed. **/
        std::string strBaseLocation;


        /** Caching Size.
            TODO: Make this a variable actually enforced. **/
        uint32_t nCacheSize = 0;


        /* Use these for iterating file locations. */
        mutable uint32_t nCurrentFileSize;
        mutable uint16_t nCurrentFile;

        /* The flags */
        uint8_t nFlags;

        /** Caching Flag */
        bool fMemoryCaching;

    public:

        /** Map to Contain the Binary Positions of Each Key **/
        mutable typename std::map< std::vector<uint8_t>, std::pair<uint16_t, uint32_t> > mapKeys;


        /** Default Constructor. **/
        BinaryFileMap() = delete;


        /** The Database Constructor. To determine file location and the Bytes per Record. **/
        BinaryFileMap(const std::string& strBaseLocationIn, const uint8_t nFlagsIn);


        /** Copy Constructor **/
        BinaryFileMap(const BinaryFileMap& map);


        /** Move Constructor **/
        BinaryFileMap(BinaryFileMap&& map) noexcept;


        /** Copy Assignment Operator **/
        BinaryFileMap& operator=(const BinaryFileMap& map);


        /** Move Assignment Operator **/
        BinaryFileMap& operator=(BinaryFileMap&& map) noexcept;


        /** Default Destructor **/
        ~BinaryFileMap();


        /** HasKey
         *
         *  Determines if the database has the Key.
         *
         *  @param[in] vKey The Key to search for.
         *
         *  Return Whether a Key Exists in the Database.
         *
         **/
        bool HasKey(const std::vector<uint8_t>& vKey) const;


        /** Initialize
         *
         *  Read the Database Keys and File Positions.
         *
         **/
        void Initialize();


        /** Put
         *
         *  Add / Update A Record in the Database
         *
         **/
        bool Put(const SectorKey &cKey) const;


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
         *  Simple Erase for now, not efficient in Data Usage of HD but quick
         *  to get erase function working.
         *
         *  @param[in] vKey The key to erase.
         *
         *  @return Returns true if key erase was successful, false otherwise.
         *
         **/
        bool Erase(const std::vector<uint8_t>& vKey);


        /** Get
         *
         *  Get a Record from the Database with Given Key.
         *
         *  @param[in] vKey The key to search for.
         *  @param[out] cKey The Record to retrieve from the database.
         *
         *  @return True if successful retrieval, false otherwise.
         *
         **/
        bool Get(const std::vector<uint8_t>& vKey, SectorKey& cKey);
    };
}

#endif
