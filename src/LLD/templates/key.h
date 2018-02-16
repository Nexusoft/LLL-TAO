/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2017] ++
            
            (c) Copyright The Nexus Developers 2014 - 2017
            
            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.
            
            "fides in stellis, virtus in numeris" - Faith in the Stars, Power in Numbers

____________________________________________________________________________________________*/

#ifndef NEXUS_LLD_TEMPLATES_KEY_H
#define NEXUS_LLD_TEMPLATES_KEY_H

#include <fstream>


#include "../../LLC/hash/SK.h"
#include "../../LLC/hash/macro.h"
#include "../../LLC/types/bignum.h"

#include "../../Util/include/args.h"
#include "../../Util/include/config.h"
#include "../../Util/include/mutex.h"
#include "../../Util/include/hex.h"
#include "../../Util/templates/serialize.h"

namespace LLD
{
    /** Enumeration for each State.
        Allows better thread concurrency
        Allow Reads on READY and READ. 
        
        Only Flush to Database if not Cached. (TODO) **/
    enum 
    {
        EMPTY 			= 0,
        READ  			= 1,
        WRITE 			= 2,
        READY 			= 3,
        TRANSACTION     = 4
    };
    
    /* Maximum size a file can be in the keychain. */
    const unsigned int MAX_KEYCHAIN_FILE_SIZE = 1024 * 1024 * 1024; //1 GB per File
    
    
    /* Total Buckets for Key Searching. */
    const unsigned int TOTAL_KEYCHAIN_BUCKETS = 256 * 256;
    
    
    /** Key Class to Hold the Location of Sectors it is referencing. 
        This Indexes the Sector Database. **/
    class SectorKey
    {
    public:
        
        /** The Key Header:
            Byte 0: nState
            Byte 1 - 3: nLength (The Size of the Sector)
            Byte 3 - 5: nSector (The Sector Number [0 - x])
        **/
        unsigned char   		   	nState;
        unsigned short 			   nLength;
        
        /** These three hold the location of 
            Sector in the Sector Database of 
            Given Sector Key. **/
        unsigned short 			   nSectorFile;
        unsigned short   		   nSectorSize;
        unsigned int   			   nSectorStart;
        
        /* The binary data of the Sector key. */
        std::vector<unsigned char> vKey;
        
        /** Checksum of Original Data to ensure no database corrupted sectors. 
            TODO: Consider the Original Data from a checksum.
            When transactions implemented have transactions stored in a sector Database.
            Ensure Original keychain and new keychain are stored on disk.
            If there is failure here before it is commited to the main database it will only
            corrupt the database backups. This will ensure the core database never gets corrupted.
            On startup ensure that the checksums match to ensure that the database was not stopped
            in the middle of a write. **/
        unsigned int nChecksum;
        
        /* Serialization Macro. */
        IMPLEMENT_SERIALIZE
        (
            READWRITE(nState);
            READWRITE(nLength);
            READWRITE(nSectorFile);
            READWRITE(nSectorSize);
            READWRITE(nSectorStart);
            READWRITE(nChecksum);
        )
        
        /* Constructors. */
        SectorKey() : nState(0), nLength(0), nSectorFile(0), nSectorSize(0), nSectorStart(0) { }
        SectorKey(unsigned char nStateIn, std::vector<unsigned char> vKeyIn, unsigned short nSectorFileIn, unsigned int nSectorStartIn, unsigned short nSectorSizeIn) : nState(nStateIn), nSectorFile(nSectorFileIn), nSectorSize(nSectorSizeIn), nSectorStart(nSectorStartIn)
        { 
            nLength = vKeyIn.size();
            vKey    = vKeyIn;
        }
        
        
        /* Iterator to the beginning of the raw key. */
        unsigned int Begin() { return 15; }
        
        
        /* Return the Size of the Key Sector on Disk. */
        unsigned int Size() { return (15 + nLength); }
        
        
        /* Dump Key to Debug Console. */
        void Print() { printf("SectorKey(nState=%u, nLength=%u, nSectorFile=%u, nSectorSize=%u, nSectorStart=%u, nChecksum=%" PRIu64 ")\n", nState, nLength, nSectorFile, nSectorSize, nSectorStart, nChecksum); }
        
        
        /* Check for Key Activity on Sector. */
        bool Empty() { return (nState == EMPTY); }
        bool Ready() { return (nState == READY); }
        bool IsTxn() { return (nState == TRANSACTION); }
        
    };

    /** Base Key Database Class.
        Stores and Contains the Sector Keys to Access the
        Sector Database.
    **/
    class KeyDatabase
    {
    protected:
        /** Mutex for Thread Synchronization. **/
        mutable Mutex_t KEY_MUTEX;
        
        
        /** The String to hold the Disk Location of Database File. 
            Each Database File Acts as a New Table as in Conventional Design.
            Key can be any Type, which is how the Database Records are Accessed. **/
        std::string strBaseLocation;
        std::string strDatabaseName;
        std::string strLocation;
        
        /** Caching Flag
            TODO: Expand the Caching System. **/
        bool fMemoryCaching = false;
        
        /** Caching Size.
            TODO: Make this a variable actually enforced. **/
        unsigned int nCacheSize = 0;
        
        
        /* Use these for iterating file locations. */
        mutable unsigned short nCurrentFile;
        mutable unsigned int nCurrentFileSize;
        
        /* Hashmap Custom Hash Using SK. */
        struct SK_Hashmap
        {
            std::size_t operator()(const std::vector<unsigned char>& k) const {
                return LLC::HASH::SK32(k);
            }
        };
        
    public:	
        /** Map to Contain the Binary Positions of Each Key.
            Used to Quickly Read the Database File at Given Position
            To Obtain the Record from its Database Key. This is Read
            Into Memory on Database Initialization. **/
        mutable typename std::map< std::vector<unsigned char>, std::pair<unsigned short, unsigned int> > mapKeys[TOTAL_KEYCHAIN_BUCKETS];
        
        
        /** Caching Memory Map. Keep within caching limits. Pop on and off like a stack
            using seperate caching class if over limits. **/
        mutable typename std::map< std::vector<unsigned char>, SectorKey > mapKeysCache[TOTAL_KEYCHAIN_BUCKETS];
        
        
        /** The Database Constructor. To determine file location and the Bytes per Record. **/
        KeyDatabase(std::string strBaseLocationIn, std::string strDatabaseNameIn) : strBaseLocation(strBaseLocationIn), strDatabaseName(strDatabaseNameIn), nCurrentFile(0), nCurrentFileSize(0) { strLocation = strBaseLocation + strDatabaseName; }
        
        
        /** Clean up Memory Usage. **/
        ~KeyDatabase() {
            for(int i = 0; i < TOTAL_KEYCHAIN_BUCKETS; i++)
                mapKeys[i].clear();
        }
        
        
        /** Return the Keys to the Records Held in the Database. **/
        std::vector< std::vector<unsigned char> > GetKeys()
        {
            std::vector< std::vector<unsigned char> > vKeys;
            for(int i = 0; i < TOTAL_KEYCHAIN_BUCKETS; i++)
                for(typename std::map< std::vector<unsigned char>, std::pair<unsigned short, unsigned int> >::iterator nIterator = mapKeys[i].begin(); nIterator != mapKeys[i].end(); nIterator++ )
                    vKeys.push_back(nIterator->first);
                
            return vKeys;
        }
        
        
        /** Return Whether a Key Exists in the Database. **/
        bool HasKey(const std::vector<unsigned char>& vKey) { return mapKeys[GetBucket(vKey)].count(vKey); }
        
        
        /** Handle the Assigning of a Map Bucket. **/
        unsigned int GetBucket(const std::vector<unsigned char>& vKey) const
        {
            assert(vKey.size() > 1);
            return ((vKey[0] << 8) + vKey[1]);
        }
        
        
        /** Read the Database Keys and File Positions. **/
        void Initialize()
        {
            LOCK(KEY_MUTEX);
            
            /* Create the Sector Database Directories. */
            boost::filesystem::path dir(strBaseLocation);
            if(!boost::filesystem::exists(dir))
                boost::filesystem::create_directory(dir);
            
            
            /* Stats variable for collective keychain size. */
            unsigned int nKeychainSize = 0, nTotalKeys = 0;
            
            
            /* Iterate through the files detected. */
            while(true)
            {
                std::string strFilename = strprintf("%s-%u.keys", strLocation.c_str(), nCurrentFile);
                printf("[DATABASE] Checking File %s\n", strFilename.c_str());
                
                /* Get the Filename at given File Position. */
                std::fstream fIncoming(strFilename.c_str(), std::ios::in | std::ios::binary);
                if(!fIncoming)
                {
                    if(nCurrentFile > 0)
                        nCurrentFile --;
                    else
                    {
                        std::ofstream fStream(strFilename.c_str(), std::ios::out | std::ios::binary);
                        fStream.close();
                    }
                    
                    break;
                }
                
                /* Get the Binary Size. */
                fIncoming.ignore(std::numeric_limits<std::streamsize>::max());
                nCurrentFileSize = fIncoming.gcount();
                nKeychainSize += nCurrentFileSize;
                
                printf("[DATABASE] Keychain File %u Loading [%u bytes]...\n", nCurrentFile, nCurrentFileSize);
                
                
                fIncoming.seekg (0, std::ios::beg);
                std::vector<unsigned char> vKeychain(nCurrentFileSize, 0);
                fIncoming.read((char*) &vKeychain[0], vKeychain.size());
                fIncoming.close();
                
                
                /* Iterator for Key Sectors. */
                unsigned int nIterator = 0;
                while(nIterator < nCurrentFileSize)
                {
                    
                    /* Get Binary Data */
                    std::vector<unsigned char> vKey(vKeychain.begin() + nIterator, vKeychain.begin() + nIterator + 15);
                    
                    
                    /* Read the State and Size of Sector Header. */
                    SectorKey cKey;
                    CDataStream ssKey(vKey, SER_LLD, DATABASE_VERSION);
                    ssKey >> cKey;
                    

                    /* Skip Empty Sectors for Now. 
                        TODO: Handle any sector and keys gracfully here to ensure that the Sector is returned to a valid state from the transaction journal in case there was a failure reading and writing in the sector. This will most likely be held in the sector database code. */
                    if(cKey.Ready())
                    {
                    
                        /* Read the Key Data. */
                        std::vector<unsigned char> vKey(vKeychain.begin() + nIterator + 15, vKeychain.begin() + nIterator + 15 + cKey.nLength);
                        
                        /* Set the Key Data. */
                        unsigned int nBucket = GetBucket(vKey);
                        mapKeys[nBucket][vKey] = std::make_pair(nCurrentFile, nIterator);
                        //mapKeysCache[nBucket][vKey] = cKey;
                        
                        /* Debug Output of Sector Key Information. */
                        if(GetArg("-verbose", 0) >= 5)
                            printf("KeyDB::Load() : State: %u Length: %u File: %u Location: %u Key: %s\n", cKey.nState, cKey.nLength, mapKeys[nBucket][vKey].first, mapKeys[nBucket][vKey].second, HexStr(vKey.begin(), vKey.end()).c_str());
                    
                        nTotalKeys++;
                    }
                    else 
                    {
                        
                        /* Debug Output of Sector Key Information. */
                        if(GetArg("-verbose", 0) >= 5)
                            printf("KeyDB::Load() : Skipping Sector State: %u Length: %u\n", cKey.nState, cKey.nLength);
                    }
                    
                    /* Increment the Iterator. */
                    nIterator += cKey.Size();
                }
                
                /* Iterate the current file. */
                nCurrentFile++;
                
                /* Clear the keychain data. */
                vKeychain.clear();
            }
            
            printf("[DATABASE] Keychain Initialized with %u Keys | Total Size %u | Total Files %u | Current Size %u\n", nTotalKeys, nKeychainSize, nCurrentFile + 1, nCurrentFileSize);
        }
        
        /** Add / Update A Record in the Database **/
        bool Put(SectorKey cKey) const
        {
            LOCK(KEY_MUTEX);
            
            /* Write Header if First Update. */
            unsigned int nBucket = GetBucket(cKey.vKey);
            if(!mapKeys[nBucket].count(cKey.vKey))
            {
                /* Check the Binary File Size. */
                if(nCurrentFileSize > MAX_KEYCHAIN_FILE_SIZE)
                {
                    if(GetArg("-verbose", 0) >= 4)
                        printf("KEY::Put(): Current File too Large, allocating new File %u\n", nCurrentFileSize, nCurrentFile + 1);
                        
                    nCurrentFile ++;
                    nCurrentFileSize = 0;
                    
                    std::ofstream fStream(strprintf("%s-%u.keys", strLocation.c_str(), nCurrentFile).c_str(), std::ios::out | std::ios::binary);
                    fStream.close();
                }
                
                mapKeys[nBucket][cKey.vKey] = std::make_pair(nCurrentFile, nCurrentFileSize);
            }
            
            
            /* Establish the Outgoing Stream. */
            std::string strFilename = strprintf("%s-%u.keys", strLocation.c_str(), mapKeys[nBucket][cKey.vKey].first);
            std::fstream fStream(strFilename.c_str(), std::ios::in | std::ios::out | std::ios::binary);
            
            
            /* Seek File Pointer */
            fStream.seekp(mapKeys[nBucket][cKey.vKey].second, std::ios::beg);
                
            
            /* Handle the Sector Key Serialization. */
            CDataStream ssKey(SER_LLD, DATABASE_VERSION);
            ssKey.reserve(cKey.Size());
            ssKey << cKey;
                
            
            /* Write to Disk. */
            std::vector<unsigned char> vData(ssKey.begin(), ssKey.end());
            vData.insert(vData.end(), cKey.vKey.begin(), cKey.vKey.end());
            fStream.write((char*) &vData[0], vData.size());
            
            /* Increment current File Size. */
            nCurrentFileSize += cKey.Size();
            
            
            /* Debug Output of Sector Key Information. */
            if(GetArg("-verbose", 0) >= 4)
                printf("KEY::Put(): State: %s | Length: %u | Location: %u | File: %u | Sector File: %u | Sector Size: %u | Sector Start: %u\n Key: %s\nCurrent File: %u | Current File Size: %u\n", cKey.nState == READY ? "Valid" : "Invalid", cKey.nLength, mapKeys[nBucket][cKey.vKey].second, mapKeys[nBucket][cKey.vKey].first, cKey.nSectorFile, cKey.nSectorSize, cKey.nSectorStart, HexStr(cKey.vKey.begin(), cKey.vKey.end()).c_str(), nCurrentFile, nCurrentFileSize);
            
            
            return true;
        }
        
        /** Simple Erase for now, not efficient in Data Usage of HD but quick to get erase function working. **/
        bool Erase(const std::vector<unsigned char> vKey)
        {
            LOCK(KEY_MUTEX);
            
            /* Check for the Key. */
            unsigned int nBucket = GetBucket(vKey);
            if(!mapKeys[nBucket].count(vKey))
                return error("Key Erase() : Key doesn't Exist");
            
            
            /* Establish the Outgoing Stream. */
            std::string strFilename = strprintf("%s-%u.keys", strLocation.c_str(), mapKeys[nBucket][vKey].first);
            std::fstream fStream(strFilename.c_str(), std::ios::in | std::ios::out | std::ios::binary);
            
            
            /* Set to put at the right file and sector position. */
            fStream.seekp(mapKeys[nBucket][vKey].second, std::ios::beg);
            
            
            /* Establish the Sector State as Empty. */
            std::vector<unsigned char> vData(1, EMPTY);
            fStream.write((char*) &vData[0], vData.size());
                
            
            /* Remove the Sector Key from the Memory Map. */
            mapKeys[nBucket].erase(vKey);
            
            
            return true;
        }
        
        /** Get a Record from the Database with Given Key. **/
        bool Get(const std::vector<unsigned char> vKey, SectorKey& cKey)
        {
            LOCK(KEY_MUTEX);

            
            /* Check the Memory Cache. */
            unsigned int nBucket = GetBucket(vKey);
            if(mapKeysCache[nBucket].count(vKey)) {
                cKey = mapKeysCache[nBucket][vKey];

                return true;
            }
            
            
            /* Read a Record from Binary Data. */
            if(mapKeys[nBucket].count(vKey))
            {
                
                /* Open the Stream Object. */
                std::string strFilename = strprintf("%s-%u.keys", strLocation.c_str(), mapKeys[nBucket][vKey].first);
                std::ifstream fStream(strFilename.c_str(), std::ios::in | std::ios::binary);

                
                /* Seek to the Sector Position on Disk. */
                fStream.seekg(mapKeys[nBucket][vKey].second, std::ios::beg);
            
                
                /* Read the State and Size of Sector Header. */
                std::vector<unsigned char> vData(15, 0);
                fStream.read((char*) &vData[0], 15);
                
                
                /* De-serialize the Header. */
                CDataStream ssHeader(vData, SER_LLD, DATABASE_VERSION);
                ssHeader >> cKey;
                
                
                /* Debug Output of Sector Key Information. */
                if(GetArg("-verbose", 0) >= 4)
                    printf("KEY::Get(): State: %s | Length: %u | Location: %u | File: %u | Sector File: %u | Sector Size: %u | Sector Start: %u\n Key: %s\n", cKey.nState == READY ? "Valid" : "Invalid", cKey.nLength, mapKeys[nBucket][vKey].second, mapKeys[nBucket][vKey].first, cKey.nSectorFile, cKey.nSectorSize, cKey.nSectorStart, HexStr(cKey.vKey.begin(), cKey.vKey.end()).c_str());
                        
                
                /* Skip Empty Sectors for Now. (TODO: Expand to Reads / Writes) */
                if(cKey.Ready() || cKey.IsTxn()) {
                
                    /* Read the Key Data. */
                    std::vector<unsigned char> vKeyIn(cKey.nLength, 0);
                    fStream.read((char*) &vKeyIn[0], vKeyIn.size());
                    
                    /* Check the Keys Match Properly. */
                    if(vKeyIn != vKey)
                        return error("Key Mistmatch: DB:: %s MEM %s\n", HexStr(vKeyIn.begin(), vKeyIn.end()).c_str(), HexStr(vKey.begin(), vKey.end()).c_str());
                    
                    /* Assign Key to Sector. */
                    cKey.vKey = vKeyIn;
                    
                    return true;
                }

            }
            
            return false;
        }
    };
}

#endif
