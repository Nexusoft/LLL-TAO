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
        
        
        /* Use these for iterating file locations. */
        //mutable unsigned short nCurrentFile;
        //mutable unsigned int nCurrentFileSize;
        
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
        mutable typename std::map< std::vector<unsigned char>, unsigned int > mapKeys[TOTAL_KEYCHAIN_BUCKETS];
        
        
        /** Caching Memory Map. Keep within caching limits. Pop on and off like a stack
            using seperate caching class if over limits. **/
        mutable typename std::map< std::vector<unsigned char>, SectorKey > mapKeysCache[TOTAL_KEYCHAIN_BUCKETS];
        
        
        /** The Database Constructor. To determine file location and the Bytes per Record. **/
        BinaryHashMap(std::string strBaseLocationIn, std::string strDatabaseNameIn) : strBaseLocation(strBaseLocationIn), strDatabaseName(strDatabaseNameIn) { strLocation = strBaseLocation + strDatabaseName; }
        
        
        /** Clean up Memory Usage. **/
        ~BinaryHashMap() {
            for(int i = 0; i < TOTAL_KEYCHAIN_BUCKETS; i++)
                mapKeys[i].clear();
        }
        
        
        /** Return the Keys to the Records Held in the Database. **/
        std::vector< std::vector<unsigned char> > GetKeys()
        {
            std::vector< std::vector<unsigned char> > vKeys;
            for(int i = 0; i < TOTAL_KEYCHAIN_BUCKETS; i++)
                for(typename std::map< std::vector<unsigned char>, unsigned int >::iterator nIterator = mapKeys[i].begin(); nIterator != mapKeys[i].end(); nIterator++ )
                    vKeys.push_back(nIterator->first);
                
            return vKeys;
        }
        
        
        /** Handle the Assigning of a Map Bucket. **/
        unsigned int GetBucket(const std::vector<unsigned char>& vKey) const
        {
            //assert(vKey.size() > 1);
            //uint64 nBucket = LLC::HASH::SK64(vKey);
            
            //return nBucket % TOTAL_KEYCHAIN_BUCKETS;//65536;
            
            uint64 nBucket = 0;
            for(int i = 0; i < vKey.size() && i < 8; i++)
                nBucket += vKey[i] << (8 * i);
            
            return nBucket % TOTAL_KEYCHAIN_BUCKETS;
        }
        
        
        /** Return Whether a Key Exists in the Database. **/
        bool Find(const std::vector<unsigned char> vKey, unsigned int& nBucket, unsigned int& nIterator) 
        {
            /* Write Header if First Update. */
            nBucket = GetBucket(vKey);
            
            /* If key is stashed in Memory. */
            if(fMemoryCaching && mapKeys[nBucket].count(vKey))
            {
                nIterator = mapKeys[nBucket][vKey];
                
                return true;
            }
            
            /* Otherwise search the file bucket. */
            else
            {
                /* Establish the Outgoing Stream. */
                std::string strFilename = strprintf("%s-%u.keys", strLocation.c_str(), nBucket);
                std::fstream fIncoming(strFilename.c_str(), std::ios::in | std::ios::out | std::ios::binary);
            
                fIncoming.ignore(std::numeric_limits<std::streamsize>::max());
                unsigned int nCurrentFileSize = fIncoming.gcount();
                
                fIncoming.seekg (0, std::ios::beg);
                std::vector<unsigned char> vKeychain(nCurrentFileSize, 0);
                fIncoming.read((char*) &vKeychain[0], vKeychain.size());
                fIncoming.close();
                        
                        
                /* Iterator for Key Sectors. */
                nIterator = 0;
                while(nIterator < nCurrentFileSize)
                {
                            
                    /* Get Binary Data */
                    std::vector<unsigned char> vData(vKeychain.begin() + nIterator, vKeychain.begin() + nIterator + 15);
                            
                            
                    /* Read the State and Size of Sector Header. */
                    SectorKey cKey;
                    CDataStream ssKey(vData, SER_LLD, DATABASE_VERSION);
                    ssKey >> cKey;
                            

                    /* Skip Empty Sectors for Now. 
                        TODO: Handle any sector and keys gracfully here to ensure that the Sector is returned to a valid state from the transaction journal in case there was a failure reading and writing in the sector. This will most likely be held in the sector database code. */
                    if(cKey.Ready())
                    {
                            
                        /* Read the Key Data. */
                        std::vector<unsigned char> vKeyIn(vKeychain.begin() + nIterator + 15, vKeychain.begin() + nIterator + 15 + cKey.nLength);
                                
                        /* Found the Binary Position. */
                        if(vKeyIn == vKey)
                            return true;
                            
                    }
                    
                    nIterator += cKey.Size();
                }
            }
            
            return false;
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
            fMemoryCaching = GetBoolArg("-memorycache", false);
            for(int nCurrentFile = 0; nCurrentFile < TOTAL_KEYCHAIN_BUCKETS; nCurrentFile++)
            {
                std::string strFilename = strprintf("%s-%u.keys", strLocation.c_str(), nCurrentFile);
                //printf("[DATABASE] Checking File %s\n", strFilename.c_str());
                
                /* Get the Filename at given File Position. */
                std::fstream fIncoming(strFilename.c_str(), std::ios::in | std::ios::binary);
                if(!fIncoming)
                {
                    std::ofstream fStream(strFilename.c_str(), std::ios::out | std::ios::binary);
                    fStream.close();
                    
                    continue;
                }
                
                if(fMemoryCaching)
                {
                    
                    /* Get the Binary Size. */
                    fIncoming.ignore(std::numeric_limits<std::streamsize>::max());
                    unsigned int nCurrentFileSize = fIncoming.gcount();
                    nKeychainSize += nCurrentFileSize;
                    
                    
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
                            mapKeys[nBucket][vKey] = nIterator;
                            //mapKeysCache[nBucket][vKey] = cKey;
                            
                            /* Debug Output of Sector Key Information. */
                            if(GetArg("-verbose", 0) >= 5)
                                printf("KeyDB::Load() : State: %u Length: %u File: %u Location: %u Key: %s\n", cKey.nState, cKey.nLength, nBucket, mapKeys[nBucket][vKey], HexStr(vKey.begin(), vKey.end()).c_str());
                        
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
                    
                    /* Clear the keychain data. */
                    vKeychain.clear();
                }
            }
            
            if(fMemoryCaching)
                printf("[DATABASE] Memory Cache::Keychain Initialized with %u Keys | Total Size %u\n", nTotalKeys, nKeychainSize);
        }
        
        /** Add / Update A Record in the Database **/
        bool Put(SectorKey cKey, unsigned int nBucket, unsigned int nIterator) const
        {
            LOCK(KEY_MUTEX);
            
            /* Establish the Outgoing Stream. */
            std::string strFilename = strprintf("%s-%u.keys", strLocation.c_str(), nBucket);
            std::fstream fIncoming(strFilename.c_str(), std::ios::in | std::ios::out | std::ios::binary);
            
            
            /* Seek File Pointer */
            fIncoming.seekp(nIterator, std::ios::beg);
            
            
            /* Handle the Sector Key Serialization. */
            CDataStream ssKey(SER_LLD, DATABASE_VERSION);
            ssKey.reserve(cKey.Size());
            ssKey << cKey;
                
            
            /* Write to Disk. */
            std::vector<unsigned char> vData(ssKey.begin(), ssKey.end());
            vData.insert(vData.end(), cKey.vKey.begin(), cKey.vKey.end());
            fIncoming.write((char*) &vData[0], vData.size());
            
            
            /* Debug Output of Sector Key Information. */
            if(GetArg("-verbose", 0) >= 4)
                printf("KEY::Put(): Bucket %u | Iterator %u | State: %s | Length: %u | Location: %u | File: %u | Sector File: %u | Sector Size: %u | Sector Start: %u\n Key: %s\n", nBucket, nIterator, cKey.nState == READY ? "Valid" : "Invalid", cKey.nLength, nIterator, nBucket, cKey.nSectorFile, cKey.nSectorSize, cKey.nSectorStart, HexStr(cKey.vKey.begin(), cKey.vKey.end()).c_str());
            
            
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
            std::string strFilename = strprintf("%s-%u.keys", strLocation.c_str(), nBucket);
            std::fstream fStream(strFilename.c_str(), std::ios::in | std::ios::out | std::ios::binary);
            
            
            /* Set to put at the right file and sector position. */
            fStream.seekp(mapKeys[nBucket][vKey], std::ios::beg);
            
            
            /* Establish the Sector State as Empty. */
            std::vector<unsigned char> vData(1, EMPTY);
            fStream.write((char*) &vData[0], vData.size());
                
            
            /* Remove the Sector Key from the Memory Map. */
            mapKeys[nBucket].erase(vKey);
            
            
            return true;
        }
        
        /** Get a Record from the Database with Given Key. **/
        bool Get(const std::vector<unsigned char> vKey, SectorKey& cKey, unsigned int nBucket, unsigned int nIterator)
        {
            LOCK(KEY_MUTEX);

            
            /* Check the Memory Cache. */
            if(fMemoryCaching && mapKeysCache[nBucket].count(vKey)) {
                cKey = mapKeysCache[nBucket][vKey];

                return true;
            }
            
            
            /* Open the Stream Object. */
            std::string strFilename = strprintf("%s-%u.keys", strLocation.c_str(), nBucket);
            std::ifstream fIncoming(strFilename.c_str(), std::ios::in | std::ios::binary);
            
                
            /* Seek to the Sector Position on Disk. */
            fIncoming.seekg(nIterator, std::ios::beg);
            
                
            /* Read the State and Size of Sector Header. */
            std::vector<unsigned char> vData(15, 0);
            fIncoming.read((char*) &vData[0], 15);
                
                
            /* De-serialize the Header. */
            CDataStream ssHeader(vData, SER_LLD, DATABASE_VERSION);
            ssHeader >> cKey;
                
                
            /* Debug Output of Sector Key Information. */
            if(GetArg("-verbose", 0) >= 4)
                printf("KEY::Get(): Bucket %u | Iterator %u State: %s | Length: %u | Location: %u | File: %u | Sector File: %u | Sector Size: %u | Sector Start: %u\n Key: %s\n", nBucket, nIterator, cKey.nState == READY ? "Valid" : "Invalid", cKey.nLength, mapKeys[nBucket][vKey], nBucket, cKey.nSectorFile, cKey.nSectorSize, cKey.nSectorStart, HexStr(vKey.begin(), vKey.end()).c_str());
                        
                
            /* Skip Empty Sectors for Now. (TODO: Expand to Reads / Writes) */
            if(cKey.Ready())
            {
                
                /* Read the Key Data. */
                std::vector<unsigned char> vKeyIn(cKey.nLength, 0);
                fIncoming.read((char*) &vKeyIn[0], vKeyIn.size());
                    
                /* Check the Keys Match Properly. */
                if(vKeyIn != vKey)
                    return error("Key Mistmatch: DB:: %s MEM %s\n", HexStr(vKeyIn.begin(), vKeyIn.end()).c_str(), HexStr(vKey.begin(), vKey.end()).c_str());
                    
                /* Assign Key to Sector. */
                cKey.vKey = vKeyIn;
                    
                return true;

            }
            
            return false;
        }
    };
}
    
#endif
