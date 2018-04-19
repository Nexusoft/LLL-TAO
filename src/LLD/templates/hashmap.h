/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2017] ++
            
            (c) Copyright The Nexus Developers 2014 - 2017
            
            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.
            
            "fides in stellis, virtus in numeris" - Faith in the Stars, Power in Numbers

____________________________________________________________________________________________*/

#ifndef NEXUS_LLD_TEMPLATES_HASHMAP_H
#define NEXUS_LLD_TEMPLATES_HASHMAP_H

#include <fstream>

#include "key.h"

namespace LLD
{

    /** The Maximum buckets allowed in the hashmap. */
    const unsigned int HASHMAP_TOTAL_BUCKETS = 256;
    
    /** Base Key Database Class.
        Stores and Contains the Sector Keys to Access the
        Sector Database.
    **/
    class BinaryHashMap
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
        
    public:	
        
        /** The Database Constructor. To determine file location and the Bytes per Record. **/
        BinaryHashMap(std::string strBaseLocationIn, std::string strDatabaseNameIn) : strBaseLocation(strBaseLocationIn), strDatabaseName(strDatabaseNameIn), nCurrentFile(0), nCurrentFileSize(0), strLocation(strBaseLocationIn + strDatabaseNameIn) 
        { 
            Initialize();
        }
        
        
        /** Clean up Memory Usage. **/
        ~BinaryHashMap() {}
        
        
        /** Return the Keys to the Records Held in the Database. **/
        std::vector< std::vector<unsigned char> > GetKeys()
        {
            std::vector< std::vector<unsigned char> > vKeys;
                
            return vKeys;
        }
        
        
        /** Return Whether a Key Exists in the Database. **/
        bool HasKey(const std::vector<unsigned char>& vKey) 
        { 
            return false;
        }
        
        
        /** Handle the Assigning of a Map Bucket. **/
        unsigned int GetBucket(const std::vector<unsigned char>& vKey) const
        {
            uint64 nBucket = 0;
            for(int i = 0; i < vKey.size() && i < 8; i++)
                nBucket += vKey[i] << (8 * i);
            
            return nBucket % HASHMAP_TOTAL_BUCKETS;
        }
        
        
        /** Read the Database Keys and File Positions. **/
        void Initialize()
        {
            LOCK(KEY_MUTEX);
            
            /* Create the Sector Database Directories. */
            boost::filesystem::path dir(strBaseLocation);
            if(!boost::filesystem::exists(dir))
                boost::filesystem::create_directory(dir);
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
