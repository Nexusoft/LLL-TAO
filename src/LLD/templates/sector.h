/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_LLD_TEMPLATES_SECTOR_H
#define NEXUS_LLD_TEMPLATES_SECTOR_H

#include <functional>
#include <atomic>

#include <LLD/templates/key.h>
#include <LLD/templates/transaction.h>

#include <Util/include/runtime.h>
#include <Util/include/filesystem.h>

//static std::fstream* streamTest;

namespace LLD
{

    /* Maximum size a file can be in the keychain. */
    const uint32_t MAX_SECTOR_FILE_SIZE = 1024 * 1024 * 1024; //1 GB per File


    /* Maximum cache buckets for sectors. */
    const uint32_t MAX_SECTOR_CACHE_SIZE = 1024 * 1024 * 128; //512 MB Max Cache


    /* The maximum amount of bytes allowed in the memory buffer for disk flushes. **/
    const uint32_t MAX_SECTOR_BUFFER_SIZE = 1024 * 1024 * 128; //512 MB Max Disk Buffer


    /** Base Template Class for a Sector Database.
        Processes main Lower Level Disk Communications.
        A Sector Database Is a Fixed Width Data Storage Medium.

        It is ideal for data structures to be stored that do not
        change in their size. This allows the greatest efficiency
        in fixed data storage (structs, class, etc.).

        It is not ideal for data structures that may vary in size
        over their lifetimes. The Dynamic Database will allow that.

        Key Type can be of any type. Data lengths are attributed to
        each key type. Keys are assigned sectors and stored in the
        key storage file. Sector files are broken into maximum of 1 GB
        for stability on all systems, key files are determined the same.

        Multiple Keys can point back to the same sector to allow multiple
        access levels of the sector. This specific class handles the lower
        level disk communications for the sector database.

        If each sector was allowed to be varying sizes it would remove the
        ability to use free space that becomes available upon an erase of a
        record. Use this Database purely for fixed size structures. Overflow
        attempts will trigger an error code.

        TODO:: Add in the Database File Searching from Sector Keys. Allow Multiple Files.

    **/
    template<typename KeychainType, typename CacheType> class SectorDatabase
    {
    protected:
        /* Mutex for Thread Synchronization.
            TODO: Lock Mutex based on Read / Writes on a per Sector Basis.
            Will allow higher efficiency for thread concurrency. */
        Mutex_t SECTOR_MUTEX;
        Mutex_t BUFFER_MUTEX;


        /* The String to hold the Disk Location of Database File. */
        std::string strBaseLocation, strName;


        /* Read only Flag for Sectors. */
        bool fReadOnly = false;


        /* Destructor Flag. */
        bool fDestruct = false;


        /* Initialize Flag. */
        bool fInitialized = false;


        /* Timer for Runtime Calculations. */
        Timer runtime;


        /* Class to handle Transaction Data. */
        SectorTransaction* pTransaction;


        /* Sector Keys Database. */
        KeychainType* SectorKeys;


        /* For the Meter. */
        uint32_t nBytesRead;
        uint32_t nBytesWrote;


        /* Cache Pool */
        CacheType* cachePool;


        /* The current File Position. */
        mutable uint32_t nCurrentFile;
        mutable uint32_t nCurrentFileSize;


        /* Cache Writer Thread. */
        Thread_t CacheWriterThread;


        /* The meter thread. */
        Thread_t MeterThread;


        /* Disk Buffer Vector. */
        std::vector< std::pair< std::vector<uint8_t>, std::vector<uint8_t> > > vDiskBuffer;


        /* Disk Buffer Memory Size. */
        std::atomic<uint32_t> nBufferBytes;

    public:
        /** The Database Constructor. To determine file location and the Bytes per Record. **/
        SectorDatabase(std::string strNameIn, const char* pszMode="r+") : strName(strNameIn), strBaseLocation(GetDataDir() + "/" + strNameIn + "/datachain/"), cachePool(new CacheType(MAX_SECTOR_CACHE_SIZE)), nBytesRead(0), nBytesWrote(0), nCurrentFile(0), nCurrentFileSize(0), CacheWriterThread(std::bind(&SectorDatabase::CacheWriter, this)), MeterThread(std::bind(&SectorDatabase::Meter, this)), nBufferBytes(0)
        {
            if(GetBoolArg("-runtime", false))
                runtime.Start();

            /* Read only flag when instantiating new database. */
            fReadOnly = (!strchr(pszMode, '+') && !strchr(pszMode, 'w'));

            /* Initialize the Keys Class. */
            SectorKeys = new KeychainType((GetDataDir() + "/" + strName + "/keychain/"));

            /* Initialize the Database. */
            Initialize();

            if(GetBoolArg("-runtime", false))
                printf(ANSI_COLOR_GREEN FUNCTION "executed in %u micro-seconds\n" ANSI_COLOR_RESET, __PRETTY_FUNCTION__, runtime.ElapsedMicroseconds());
        }

        ~SectorDatabase()
        {
            fDestruct = true;

            CacheWriterThread.join();

            delete pTransaction;
            delete cachePool;
            delete SectorKeys;
        }


        /** Initialize Sector Database. **/
        void Initialize()
        {
            /* Create directories if they don't exist yet. */
            if(filesystem::create_directories(strBaseLocation))
                printf(FUNCTION "Generated Path %s\n", __PRETTY_FUNCTION__, strBaseLocation.c_str());

            /* Find the most recent append file. */
            while(true)
            {

                /* TODO: Make a worker or thread to check sizes of files and automatically create new file.
                    Keep independent of reads and writes for efficiency. */
                std::fstream fIncoming(strprintf("%s_block.%05u", strBaseLocation.c_str(), nCurrentFile).c_str(), std::ios::in | std::ios::binary);
                if(!fIncoming)
                {

                    /* Assign the Current Size and File. */
                    if(nCurrentFile > 0)
                        nCurrentFile--;
                    else
                    {
                        /* Create a new file if it doesn't exist. */
                        std::ofstream cStream(strprintf("%s_block.%05u", strBaseLocation.c_str(), nCurrentFile).c_str(), std::ios::binary | std::ios::out | std::ios::trunc);
                        cStream.close();
                    }

                    break;
                }

                /* Get the Binary Size. */
                fIncoming.seekg(0, std::ios::end);
                nCurrentFileSize = fIncoming.tellg();
                fIncoming.close();

                /* Increment the Current File */
                nCurrentFile++;
            }

            pTransaction = NULL;
            fInitialized = true;
        }


        /* Get the keys for this sector database from the keychain.  */
        std::vector< std::vector<uint8_t> > GetKeys()
        {
            return SectorKeys->GetKeys();
        }


        template<typename Key>
        bool Exists(const Key& key)
        {
            /** Serialize Key into Bytes. **/
            CDataStream ssKey(SER_LLD, DATABASE_VERSION);
            ssKey.reserve(GetSerializeSize(key, SER_LLD, DATABASE_VERSION));
            ssKey << key;
            std::vector<uint8_t> vKey(ssKey.begin(), ssKey.end());

            /* Return the Key existance in the Keychain Database. */
            SectorKey cKey;
            return SectorKeys->Get(vKey, cKey);
        }

        template<typename Key>
        bool Erase(const Key& key)
        {
            if(GetBoolArg("-runtime", false))
                runtime.Start();

            /* Serialize Key into Bytes. */
            CDataStream ssKey(SER_LLD, DATABASE_VERSION);
            ssKey.reserve(GetSerializeSize(key, SER_LLD, DATABASE_VERSION));
            ssKey << key;
            std::vector<uint8_t> vKey(ssKey.begin(), ssKey.end());

            if(pTransaction){
                pTransaction->EraseTransaction(vKey);

                return true;
            }


            /* Return the Key existance in the Keychain Database. */
            bool fErased = SectorKeys->Erase(vKey);

            if(GetBoolArg("-runtime", false))
                printf(ANSI_COLOR_GREEN FUNCTION "executed in %u micro-seconds\n" ANSI_COLOR_RESET, __PRETTY_FUNCTION__, runtime.ElapsedMicroseconds());

            return fErased;
        }

        template<typename Key, typename Type>
        bool Read(const Key& key, Type& value)
        {
            /** Serialize Key into Bytes. **/
            DataStream ssKey(SER_LLD, DATABASE_VERSION);
            ssKey << key;

            /** Get the Data from Sector Database. **/
            std::vector<uint8_t> vData;
            if(!Get(ssKey, vData))
                return false;

            /** Deserialize Value. **/
            DataStream ssValue(vData, SER_LLD, DATABASE_VERSION);
            ssValue >> value;

            return true;
        }

        template<typename Key, typename Type>
        bool Write(const Key& key, const Type& value)
        {
            if (fReadOnly)
                assert(!"Write called on database in read-only mode");

            /* Serialize the Key. */
            DataStream ssKey(SER_LLD, DATABASE_VERSION);
            ssKey << key;

            /* Serialize the Value */
            DataStream ssData(SER_LLD, DATABASE_VERSION);
            ssData << value;

            return Put(ssKey, ssData);
        }

        /** Get a Record from the Database with Given Key. **/
        bool Get(std::vector<uint8_t> vKey, std::vector<uint8_t>& vData)
        {
            nBytesRead += (vKey.size() + vData.size());

            if(cachePool->Get(vKey, vData))
                return true;

            /* Check that the key is not pending in a transaction for Erase. */
            if(pTransaction && pTransaction->mapEraseData.count(vKey))
                return false;

            /* Check if the new data is set in a transaction to ensure that the database knows what is in volatile memory. */
            if(pTransaction && pTransaction->mapTransactions.count(vKey))
            {
                vData = pTransaction->mapTransactions[vKey];

                if(GetArg("-verbose", 0) >= 4)
                    printf(FUNCTION "%s\n", __PRETTY_FUNCTION__, HexStr(vData.begin(), vData.end()).c_str());

                return true;
            }

            SectorKey cKey;
            if(SectorKeys->Get(vKey, cKey))
            {
                LOCK(SECTOR_MUTEX);

                /* Open the Stream to Read the data from Sector on File. */
                std::string strFilename = strprintf("%s_block.%05u", strBaseLocation.c_str(), cKey.nSectorFile);
                std::fstream fStream(strFilename.c_str(), std::ios::in | std::ios::binary);
                if(!fStream)
                    return error(FUNCTION "Sector File %s Doesn't Exist\n", __PRETTY_FUNCTION__, strFilename.c_str());

                /* Seek to the Sector Position on Disk. */
                fStream.seekg(cKey.nSectorStart, std::ios::beg);

                /* Read the State and Size of Sector Header. */
                vData.resize(cKey.nSectorSize);
                fStream.read((char*) &vData[0], vData.size());
                fStream.close();

                /* Check the Data Integrity of the Sector by comparing the Checksums. */
                uint32_t nChecksum = LLC::SK32(vData);
                if(cKey.nChecksum != nChecksum)
                    return error(FUNCTION "Checksums don't match data. Corrupted Sector.", __PRETTY_FUNCTION__);

                if(GetArg("-verbose", 0) >= 4)
                    printf(FUNCTION "%s\n", __PRETTY_FUNCTION__, HexStr(vData.begin(), vData.end()).c_str());

                return true;
            }
            else
                return error(FUNCTION "KEY NOT FOUND", __PRETTY_FUNCTION__);

            return false;
        }


        /** Add / Update A Record in the Database **/
        bool Put2(std::vector<uint8_t> vKey, std::vector<uint8_t> vData)
        {
            /* Write Header if First Update. */
            LOCK(SECTOR_MUTEX);

            if(nCurrentFileSize > MAX_SECTOR_FILE_SIZE)
            {
                if(GetArg("-verbose", 0) >= 4)
                    printf(FUNCTION "Current File too Large, allocating new File %u\n", __PRETTY_FUNCTION__, nCurrentFileSize, nCurrentFile + 1);

                nCurrentFile ++;
                nCurrentFileSize = 0;

                std::ofstream fStream(strprintf("%s_block.%05u", strBaseLocation.c_str(), nCurrentFile).c_str(), std::ios::out | std::ios::binary);
                fStream.close();
            }

            /* Open the Stream to Read the data from Sector on File. */
            std::string strFilename = strprintf("%s_block.%05u", strBaseLocation.c_str(), nCurrentFile);
            std::fstream fStream(strFilename.c_str(), std::ios::in | std::ios::out | std::ios::binary);

            /* If it is a New Sector, Assign a Binary Position.
                TODO: Track Sector Database File Sizes. */
            fStream.seekp(nCurrentFileSize, std::ios::beg);
            fStream.write((char*) &vData[0], vData.size());
            fStream.close();

            /* Create a new Sector Key. */
            SectorKey cKey = SectorKey(READY, vKey, nCurrentFile, nCurrentFileSize, vData.size());

            /* Check the Data Integrity of the Sector by comparing the Checksums. */
            cKey.nChecksum    = LLC::SK32(vData);

            /* Increment the current filesize */
            nCurrentFileSize += vData.size();

            /* Assign the Key to Keychain. */
            SectorKeys->Put(cKey);


            if(GetArg("-verbose", 0) >= 4)
                printf(FUNCTION "%s | Current File: %u | Current File Size: %u\n", __PRETTY_FUNCTION__, HexStr(vData.begin(), vData.end()).c_str(), nCurrentFile, nCurrentFileSize);

            if(GetBoolArg("-runtime", false))
                printf(ANSI_COLOR_GREEN FUNCTION "executed in %u micro-seconds\n" ANSI_COLOR_RESET, __PRETTY_FUNCTION__, runtime.ElapsedMicroseconds());

            return true;
        }


        /** Add / Update A Record in the Database **/
        bool Put(std::vector<uint8_t> vKey, std::vector<uint8_t> vData)
        {
            /* Write the data into the memory cache. */
            cachePool->Put(vKey, vData);
            while(!fShutdown && nBufferBytes > MAX_SECTOR_BUFFER_SIZE)
                Sleep(1);

            /* Add to the write buffer thread. */
            { LOCK(BUFFER_MUTEX);
                vDiskBuffer.push_back(std::make_pair(vKey, vData));
                nBufferBytes += (vKey.size() + vData.size());
            }

            return true;
        }


        /* Helper Thread to Batch Write to Disk. */
        void CacheWriter()
        {
            while(!fShutdown)
            {
                /* Wait for Database to Initialize. */
                if(!fInitialized)
                {
                    Sleep(10);

                    continue;
                }

                /* Check for data to be written. */
                if(nBufferBytes == 0)
                {
                    if(fDestruct)
                        return;

                    Sleep(1);

                    continue;
                }


                /* Swap the buffer object to get ready for writes. */
                std::vector< std::pair<std::vector<uint8_t>, std::vector<uint8_t>> > vIndexes;
                {
                    LOCK(BUFFER_MUTEX);
                    
                    vIndexes.swap(vDiskBuffer);
                    nBufferBytes = 0;
                }

                /* Create a new file if the sector file size is over file size limits. */
                if(nCurrentFileSize > MAX_SECTOR_FILE_SIZE)
                {
                    printf(FUNCTION "Generated Sector File %u\n", __PRETTY_FUNCTION__, nCurrentFile + 1);

                    /* Iterate the current file and reset current file sie. */
                    nCurrentFile ++;
                    nCurrentFileSize = 0;

                    /* Create a new file for next writes. */
                    std::fstream stream(strprintf("%s_block.%05u", strBaseLocation.c_str(), nCurrentFile), std::ios::out | std::ios::binary | std::ios::trunc);
                    stream.close();
                }

                /* Open the Stream to Read the data from Sector on File. */
                std::fstream stream(strprintf("%s_block.%05u", strBaseLocation.c_str(), nCurrentFile), std::ios::in | std::ios::out | std::ios::binary);

                /* Seek to the end of the file */
                stream.seekp(nCurrentFileSize, std::ios::beg);

                /* Iterate through buffer to queue disk writes. */
                std::vector<uint8_t> vWrite;
                for(auto vObj : vIndexes)
                {
                    /* Create a new Sector Key. */
                    SectorKey cTmp = SectorKey(READY, vObj.first, nCurrentFile, nCurrentFileSize, vObj.second.size());

                    /* Check the Data Integrity of the Sector by comparing the Checksums. */
                    cTmp.nChecksum = LLC::SK32(vObj.second);

                    /* Increment the current filesize */
                    nCurrentFileSize += vObj.second.size();

                    /* Assign the Key to Keychain. */
                    SectorKeys->Put(cTmp);

                    /* Add data to the write buffer */
                    vWrite.insert(vWrite.end(), vObj.second.begin(), vObj.second.end());

                    /* Add the file size to the written bytes. */
                    nBytesWrote += vObj.first.size();

                    /* Flush to disk on periodic intervals. */
                    if(vWrite.size() > 10 * 1024 * 1024)
                    {
                        nBytesWrote += (vWrite.size());

                        stream.write((char*)&vWrite[0], vWrite.size());
                        vWrite.clear();
                    }
                }

                nBytesWrote += (vWrite.size());

                /* Flush remaining to disk. */
                stream.write((char*)&vWrite[0], vWrite.size());
                stream.close();

                printf(FUNCTION "Flushed %u Bytes to Disk\n", __PRETTY_FUNCTION__, vWrite.size());
            }
        }

        /* LLP Meter Thread. Tracks the Requests / Second. */
        void Meter()
        {
            if(!GetBoolArg("-meters", false))
                return;

            Timer TIMER;
            TIMER.Start();

            while(!fShutdown)
            {
                Sleep(10000);

                double WPS = nBytesWrote / (TIMER.Elapsed() * 1024.0);
                double RPS = nBytesRead / (TIMER.Elapsed() * 1024.0);

                printf(">>>>> LLD Writing at %f Kb/s\n", WPS);
                printf(">>>>> LLD Reading at %f Kb/s\n", RPS);

                TIMER.Reset();
                nBytesWrote = 0;
                nBytesRead  = 0;
            }
        }

        /** Start a New Database Transaction.
            This will put all the database changes into pending state.
            If any of the database updates fail in procewss it will roll the database back to its previous state. **/
        void TxnBegin()
        {
            /** Delete a previous database transaction pointer if applicable. **/
            if(pTransaction)
                delete pTransaction;

            /** Create the new Database Transaction Object. **/
            pTransaction = new SectorTransaction();

            if(GetArg("-verbose", 0) >= 4)
                printf(FUNCTION "New Sector Transaction Started.\n", __PRETTY_FUNCTION__);
        }

        /** Abort the current transaction that is pending in the transaction chain. **/
        void TxnAbort()
        {
            /** Delete the previous transaction pointer if applicable. **/
            if(pTransaction)
                delete pTransaction;

            /** Set the transaction pointer to null also acting like a flag **/
            pTransaction = NULL;
        }

        /** Return the database state back to its original state before transactions are commited. **/
        bool RollbackTransactions()
        {
                /** Iterate the original data memory map to reset the database to its previous state. **/
            for(typename std::map< std::vector<uint8_t>, std::vector<uint8_t> >::iterator nIterator = pTransaction->mapOriginalData.begin(); nIterator != pTransaction->mapOriginalData.end(); nIterator++ )
                if(!Put(nIterator->first, nIterator->second))
                    return false;

            return true;
        }

        /** Commit the Data in the Transaction Object to the Database Disk.
            TODO: Handle the Transaction Rollbacks with a new Transaction Keychain and Sector Database.
            Make it temporary and named after the unique identity of the sector database.
            Fingerprint is SK64 hash of unified time and the sector database name along with some other data
            To be determined...

            1. TxnStart()
                + Create a new Transaction Record (TODO: Find how this will be indentified. Maybe Unique Tx Hash and Registry in Journal of Active Transaction?)
                + Create a new Transaction Memory Object

            2. Put()
                + Add new data to the Transaction Record
                + Add new data to the Transaction Memory
                + Keep states of keys in a valid object for recover of corrupted keychain.

            3. Get()  NOTE: Read from the Transaction object rather than the database
                + Read the data from the Transaction Memory

            4. Commit()
                +

            **/
        bool TxnCommit()
        {
            LOCK(SECTOR_MUTEX);

            if(GetBoolArg("-runtime", false))
                runtime.Start();

            if(GetArg("-verbose", 0) >= 4)
                printf(FUNCTION "Commiting Transactin to Datachain.\n", __PRETTY_FUNCTION__);

            /** Check that there is a valid transaction to apply to the database. **/
            if(!pTransaction)
                return error(FUNCTION "No Transaction data to Commit.", __PRETTY_FUNCTION__);

            /** Habdle setting the sector key flags so the database knows if the transaction was completed properly. **/
            if(GetArg("-verbose", 0) >= 4)
                printf(FUNCTION "Commiting Keys to Keychain.\n", __PRETTY_FUNCTION__);

            /** Set the Sector Keys to an Invalid State to know if there are interuptions the sector was not finished successfully. **/
            for(typename std::map< std::vector<uint8_t>, std::vector<uint8_t> >::iterator nIterator = pTransaction->mapTransactions.begin(); nIterator != pTransaction->mapTransactions.end(); nIterator++ )
            {
                SectorKey cKey;
                if(SectorKeys->HasKey(nIterator->first)) {
                    if(!SectorKeys->Get(nIterator->first, cKey))
                        return error(FUNCTION "Couldn't get the Active Sector Key.", __PRETTY_FUNCTION__);

                    cKey.nState = TRANSACTION;
                    SectorKeys->Put(cKey);
                }
            }

            /** Update the Keychain with Checksums and READY Flag letting sectors know they were written successfully. **/
            if(GetArg("-verbose", 0) >= 4)
                printf(FUNCTION "Erasing Sector Keys Flagged for Deletion.\n", __PRETTY_FUNCTION__);

            /** Erase all the Transactions that are set to be erased. That way if they are assigned a TRANSACTION flag we know to roll back their key to orginal data. **/
            for(typename std::map< std::vector<uint8_t>, uint32_t >::iterator nIterator = pTransaction->mapEraseData.begin(); nIterator != pTransaction->mapEraseData.end(); nIterator++ )
            {
                if(!SectorKeys->Erase(nIterator->first))
                    return error(FUNCTION "Couldn't get the Active Sector Key for Delete.", __PRETTY_FUNCTION__);
            }

            /** Commit the Sector Data to the Database. **/
            if(GetArg("-verbose", 0) >= 4)
                printf(FUNCTION "Commit Data to Datachain Sector Database.\n", __PRETTY_FUNCTION__);

            for(typename std::map< std::vector<uint8_t>, std::vector<uint8_t> >::iterator nIterator = pTransaction->mapTransactions.begin(); nIterator != pTransaction->mapTransactions.end(); nIterator++ )
            {
                /** Declare the Key and Data for easier reference. **/
                std::vector<uint8_t> vKey  = nIterator->first;
                std::vector<uint8_t> vData = nIterator->second;

                /* Write Header if First Update. */
                if(!SectorKeys->HasKey(vKey))
                {
                    if(nCurrentFileSize > MAX_SECTOR_FILE_SIZE)
                    {
                        if(GetArg("-verbose", 0) >= 4)
                            printf(FUNCTION "Current File too Large, allocating new File %u\n", __PRETTY_FUNCTION__, nCurrentFileSize, nCurrentFile + 1);

                        nCurrentFile ++;
                        nCurrentFileSize = 0;

                        std::ofstream fStream(strprintf("%s_block.%05u", strBaseLocation.c_str(), nCurrentFile).c_str(), std::ios::out | std::ios::binary);
                        fStream.close();
                    }

                    /* Open the Stream to Read the data from Sector on File. */
                    std::string strFilename = strprintf("%s_block.%05u", strBaseLocation.c_str(), nCurrentFile);
                    std::fstream fStream(strFilename.c_str(), std::ios::in | std::ios::out | std::ios::binary);

                    /* If it is a New Sector, Assign a Binary Position.
                        TODO: Track Sector Database File Sizes. */
                    fStream.seekp(nCurrentFileSize, std::ios::beg);

                    fStream.write((char*) &vData[0], vData.size());
                    fStream.close();

                    /* Create a new Sector Key. */
                    SectorKey cKey(READY, vKey, nCurrentFile, nCurrentFileSize, vData.size());

                    /* Check the Data Integrity of the Sector by comparing the Checksums. */
                    cKey.nChecksum    = LLC::SK32(vData);

                    /* Increment the current filesize */
                    nCurrentFileSize += vData.size();

                    /* Assign the Key to Keychain. */
                    SectorKeys->Put(cKey);
                }
                else
                {
                    /* Get the Sector Key from the Keychain. */
                    SectorKey cKey;
                    if(!SectorKeys->Get(vKey, cKey)) {
                        SectorKeys->Erase(vKey);

                        return false;
                    }

                    /* Open the Stream to Read the data from Sector on File. */
                    std::string strFilename = strprintf("%s_block.%05u", strBaseLocation.c_str(), cKey.nSectorFile);
                    std::fstream fStream(strFilename.c_str(), std::ios::in | std::ios::out | std::ios::binary);

                    /* Locate the Sector Data from Sector Key.
                        TODO: Make Paging more Efficient in Keys by breaking data into different locations in Database. */
                    fStream.seekp(cKey.nSectorStart, std::ios::beg);
                    if(vData.size() > cKey.nSectorSize){
                        fStream.close();
                        printf(FUNCTION "PUT (TOO LARGE) NO TRUNCATING ALLOWED (Old %u :: New %u):%s\n", __PRETTY_FUNCTION__, cKey.nSectorSize, vData.size(), HexStr(vData.begin(), vData.end()).c_str());

                        return false;
                    }

                    /* Assign the Writing State for Sector. */
                    //TODO: use memory maps

                    fStream.write((char*) &vData[0], vData.size());
                    fStream.close();

                    cKey.nState    = READY;
                    cKey.nChecksum = LLC::SK32(vData);

                    SectorKeys->Put(cKey);
                }
            }

            /** Update the Keychain with Checksums and READY Flag letting sectors know they were written successfully. **/
            if(GetArg("-verbose", 0) >= 4)
                printf(FUNCTION "Commiting Key Valid States to Keychain.\n", __PRETTY_FUNCTION__);

            for(typename std::map< std::vector<uint8_t>, std::vector<uint8_t> >::iterator nIterator = pTransaction->mapTransactions.begin(); nIterator != pTransaction->mapTransactions.end(); nIterator++ )
            {
                /** Assign the Writing State for Sector. **/
                SectorKey cKey;
                if(!SectorKeys->Get(nIterator->first, cKey))
                    return error(FUNCTION "Failed to Get Key from Keychain.", __PRETTY_FUNCTION__);

                /** Set the Sector states back to Active. **/
                cKey.nState    = READY;
                cKey.nChecksum = LLC::SK32(nIterator->second);

                /** Commit the Keys to Keychain Database. **/
                if(!SectorKeys->Put(cKey))
                    return error(FUNCTION "Failed to Commit Key to Keychain.", __PRETTY_FUNCTION__);
            }

            /** Clean up the Sector Transaction Key.
                TODO: Delete the Sector and Keychain for Current Transaction Commit ID. **/
            delete pTransaction;
            pTransaction = NULL;

            if(GetBoolArg("-runtime", false))
                printf(ANSI_COLOR_GREEN FUNCTION "executed in %u micro-seconds\n" ANSI_COLOR_RESET, __PRETTY_FUNCTION__, runtime.ElapsedMicroseconds());

            return true;
        }
    };
}

#endif
