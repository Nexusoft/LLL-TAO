/*__________________________________________________________________________________________
 
			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2017] ++
			
			(c) Copyright The Nexus Developers 2014 - 2017
			
			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.
			
			"fides in stellis, virtus in numeris" - Faith in the Stars, Power in Numbers
  
____________________________________________________________________________________________*/

#ifndef NEXUS_LLD_TEMPLATES_SECTOR_H
#define NEXUS_LLD_TEMPLATES_SECTOR_H

#include "keychain.h"
#include "transaction.h"

namespace LLD
{

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
	class SectorDatabase
	{
	protected:
		/** Mutex for Thread Synchronization. 
			TODO: Lock Mutex based on Read / Writes on a per Sector Basis. 
			Will allow higher efficiency for thread concurrency. **/
		Mutex_t SECTOR_MUTEX;
		
		
		/** The String to hold the Disk Location of Database File. 
			Each Database File Acts as a New Table as in Conventional Design.
			Key can be any Type, which is how the Database Records are Accessed. **/
		std::string strBaseName;
		
		
		/** Location of the Directory to host Database File System. 
			Main File Components are Derived from Base Name.
			Contains Key and Cache Databases. **/
		std::string strBaseLocation;
		
		
		/** Keychain Registry:
			The nameof the Keychain Registry. **/
		std::string strKeychainRegistry;
		
		
		/** Memory Structure to Track the Database Sizes. **/
		std::vector<unsigned int> vDatabaseSizes;
		
		
		/** Read only Flag for Sectors. **/
		bool fReadOnly = false;
		
		
		/** Class to handle Transaction Data. **/
		SectorTransaction* pTransaction;
	public:
		/** The Database Constructor. To determine file location and the Bytes per Record. **/
		SectorDatabase(std::string strName, std::string strKeychain, const char* pszMode="r+")
		{
			strKeychainRegistry = strKeychain;
			strBaseLocation = GetDataDir().string() + "/datachain/"; 
			strBaseName = strName;
			
			/** Read only flag when instantiating new database. **/
			fReadOnly = (!strchr(pszMode, '+') && !strchr(pszMode, 'w'));
			
			Initialize();
		}
		~SectorDatabase()
        { 
            if (pTransaction) 
                delete pTransaction;
        }
		
		/** Initialize Sector Database. **/
		void Initialize()
		{
			/** Create the Sector Database Directories. **/
			boost::filesystem::path dir(strBaseLocation);
			boost::filesystem::create_directory(dir);
			
			/** TODO: Make a worker or thread to check sizes of files and automatically create new file.
				Keep independent of reads and writes for efficiency. **/
			std::fstream fIncoming(strprintf("%s%s%u.dat", strBaseLocation.c_str(), strBaseName.c_str(), 0).c_str(), std::ios::in | std::ios::binary);
			if(!fIncoming) {
				std::ofstream cStream(strprintf("%s%s%u.dat", strBaseLocation.c_str(), strBaseName.c_str(), 0).c_str(), std::ios::binary);
				cStream.close();
			}
			
			/* Register the Keychain in Global LLD scope if it hasn't been registered already. */
			if(!KeychainRegistered(strKeychainRegistry))
				RegisterKeychain(strKeychainRegistry);
			
			pTransaction = NULL;
		}
		
		template<typename Key>
		bool Exists(const Key& key)
		{
			/** Serialize Key into Bytes. **/
			CDataStream ssKey(SER_LLD, DATABASE_VERSION);
			ssKey.reserve(GetSerializeSize(key, SER_LLD, DATABASE_VERSION));
			ssKey << key;
			std::vector<unsigned char> vKey(ssKey.begin(), ssKey.end());
			
			/** Read a Record from Binary Data. **/
			KeyDatabase* SectorKeys = GetKeychain(strKeychainRegistry);
			if(!SectorKeys)
				return error("Exists() : Sector Keys not Registered for Name %s\n", strKeychainRegistry.c_str());
			
			/** Return the Key existance in the Keychain Database. **/
			return SectorKeys->HasKey(vKey);
		}
		
		template<typename Key>
		bool Erase(const Key& key)
		{
			/** Serialize Key into Bytes. **/
			CDataStream ssKey(SER_LLD, DATABASE_VERSION);
			ssKey.reserve(GetSerializeSize(key, SER_LLD, DATABASE_VERSION));
			ssKey << key;
			std::vector<unsigned char> vKey(ssKey.begin(), ssKey.end());
			
			if(pTransaction){
				pTransaction->EraseTransaction(vKey);
				
				return true;
			}
		
			/** Read a Record from Binary Data. **/
			KeyDatabase* SectorKeys = GetKeychain(strKeychainRegistry);
			if(!SectorKeys)
				return error("Erase() : Sector Keys not Registered for Name %s\n", strKeychainRegistry.c_str());
			
			/** Return the Key existance in the Keychain Database. **/
			return SectorKeys->Erase(vKey);
		}
		
		template<typename Key, typename Type>
		bool Read(const Key& key, Type& value)
		{
			/** Serialize Key into Bytes. **/
			CDataStream ssKey(SER_LLD, DATABASE_VERSION);
			ssKey.reserve(GetSerializeSize(key, SER_LLD, DATABASE_VERSION));
			ssKey << key;
			std::vector<unsigned char> vKey(ssKey.begin(), ssKey.end());
			
			/** Get the Data from Sector Database. **/
			std::vector<unsigned char> vData;
			if(!Get(vKey, vData))
				return false;

			/** Deserialize Value. **/
			try {
				CDataStream ssValue(vData, SER_LLD, DATABASE_VERSION);
				ssValue >> value;
			}
			catch (std::exception &e) {
				return false;
			}

			return true;
		}

		template<typename Key, typename Type>
		bool Write(const Key& key, const Type& value)
		{
			if (fReadOnly)
				assert(!"Write called on database in read-only mode");

			/** Serialize the Key. **/
			CDataStream ssKey(SER_LLD, DATABASE_VERSION);
			ssKey.reserve(GetSerializeSize(key, SER_LLD, DATABASE_VERSION));
			ssKey << key;
			std::vector<unsigned char> vKey(ssKey.begin(), ssKey.end());

			/** Serialize the Value **/
			CDataStream ssValue(SER_LLD, DATABASE_VERSION);
			ssValue.reserve(value.GetSerializeSize(SER_LLD, DATABASE_VERSION));
			ssValue << value;
			std::vector<unsigned char> vData(ssValue.begin(), ssValue.end());

			/** Commit to the Database. **/
			if(pTransaction)
			{
				
				std::vector<unsigned char> vOriginalData;
				//Get(vKey, vOriginalData);
				
				return pTransaction->AddTransaction(vKey, vData, vOriginalData);
			}
			
			return Put(vKey, vData);
		}
		
		/** Get a Record from the Database with Given Key. **/
		bool Get(std::vector<unsigned char> vKey, std::vector<unsigned char>& vData)
		{
			LOCK(SECTOR_MUTEX);
			
			/** Read a Record from Binary Data. **/
			KeyDatabase* SectorKeys = GetKeychain(strKeychainRegistry);
			if(!SectorKeys)
				return error("Get() : Sector Keys not Registered for Name %s\n", strKeychainRegistry.c_str());
			
			if(SectorKeys->HasKey(vKey))
			{	
                /* Check that the key is not pending in a transaction for Erase. */
                if(pTransaction && pTransaction->mapEraseData.count(vKey))
                    return false;
                
                /* Check if the new data is set in a transaction to ensure that the database knows what is in volatile memory. */
                if(pTransaction && pTransaction->mapTransactions.count(vKey))
                {
                    vData = pTransaction->mapTransactions[vKey];
                    
                    if(GetArg("-verbose", 0) >= 4)
                        printf("SECTOR GET:%s\n", HexStr(vData.begin(), vData.end()).c_str());
                    
                    return true;
                }
                
				/** Read the Sector Key from Keychain. **/
				SectorKey cKey;
				if(!SectorKeys->Get(vKey, cKey))
					return false;
				
				/** Open the Stream to Read the data from Sector on File. **/
				std::fstream fStream(strprintf("%s%s%u.dat", strBaseLocation.c_str(), strBaseName.c_str(), cKey.nSectorFile).c_str(), std::ios::in | std::ios::binary);

				/** Seek to the Sector Position on Disk. **/
				fStream.seekg(cKey.nSectorStart);
			
				/** Read the State and Size of Sector Header. **/
				vData.resize(cKey.nSectorSize);
				fStream.read((char*) &vData[0], vData.size());
				fStream.close();
				
				/** Check the Data Integrity of the Sector by comparing the Checksums. **/
				uint64 nChecksum = LLC::HASH::SK64(vData);
				if(cKey.nChecksum != nChecksum)
					return error("Sector Get() : Checksums don't match data. Corrupted Sector.");
				
				if(GetArg("-verbose", 0) >= 4)
					printf("SECTOR GET:%s\n", HexStr(vData.begin(), vData.end()).c_str());
				
				return true;
			}
			
			return false;
		}
		
		
		/** Add / Update A Record in the Database **/
		bool Put(std::vector<unsigned char> vKey, std::vector<unsigned char> vData)
		{
			LOCK(SECTOR_MUTEX);
			
			KeyDatabase* SectorKeys = GetKeychain(strKeychainRegistry);
			if(!SectorKeys)
				return error("Put() : Sector Keys not Registered for Name %s\n", strKeychainRegistry.c_str());
			
			/** Write Header if First Update. **/
			if(!SectorKeys->HasKey(vKey))
			{
				/** TODO:: Assign a Sector File based on Database Sizes. **/
				unsigned short nSectorFile = 0;
				
				/** Open the Stream to Read the data from Sector on File. **/
				std::fstream fStream(strprintf("%s%s%u.dat", strBaseLocation.c_str(), strBaseName.c_str(), nSectorFile).c_str(), std::ios::in | std::ios::out | std::ios::binary);
				
				/** Create a new Sector Key. **/
				SectorKey cKey(WRITE, vKey, nSectorFile, 0, vData.size()); 
				
				/** Assign the Key to Keychain. **/
				SectorKeys->Put(cKey);
				
				/** If it is a New Sector, Assign a Binary Position. 
					TODO: Track Sector Database File Sizes. **/
				unsigned int nBegin = fStream.tellg();
				fStream.seekg (0, std::ios::end);
				
				unsigned int nStart = (unsigned int) fStream.tellg() - nBegin;
				fStream.seekp(nStart, std::ios::beg);
				
				fStream.write((char*) &vData[0], vData.size());
				fStream.close();
				
				/** Assign New Data to the Sector Key. **/
				cKey.nState       = READY;
				cKey.nSectorSize  = vData.size();
				cKey.nSectorStart = nStart;
				
				/** Check the Data Integrity of the Sector by comparing the Checksums. **/
				cKey.nChecksum    = LLC::HASH::SK64(vData);
				
				/** Assign the Key to Keychain. **/
				SectorKeys->Put(cKey);
			}
			else
			{
				/** Get the Sector Key from the Keychain. **/
				SectorKey cKey;
				if(!SectorKeys->Get(vKey, cKey)) {
					SectorKeys->Erase(vKey);
					
					return false;
				}
					
				/** Open the Stream to Read the data from Sector on File. **/
				std::fstream fStream(strprintf("%s%s%u.dat", strBaseLocation.c_str(), strBaseName.c_str(), cKey.nSectorFile).c_str(), std::ios::in | std::ios::out | std::ios::binary);
				
				/** Locate the Sector Data from Sector Key. 
					TODO: Make Paging more Efficient in Keys by breaking data into different locations in Database. **/
				fStream.seekp(cKey.nSectorStart, std::ios::beg);
				if(vData.size() > cKey.nSectorSize){
					fStream.close();
					printf("ERROR PUT (TOO LARGE) NO TRUNCATING ALLOWED (Old %u :: New %u):%s\n", cKey.nSectorSize, vData.size(), HexStr(vData.begin(), vData.end()).c_str());
					
					return false;
				}
				
				/** Assign the Writing State for Sector. **/
				cKey.nState = WRITE;
				SectorKeys->Put(cKey);
				
				fStream.write((char*) &vData[0], vData.size());
				fStream.close();
				
				cKey.nState    = READY;
				cKey.nChecksum = LLC::HASH::SK64(vData);
				
				SectorKeys->Put(cKey);
			}
			
			if(GetArg("-verbose", 0) >= 4)
				printf("SECTOR PUT:%s\n", HexStr(vData.begin(), vData.end()).c_str());
			
			return true;
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
				printf("TransactionStart() : New Sector Transaction Started.\n");
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
			for(typename std::map< std::vector<unsigned char>, std::vector<unsigned char> >::iterator nIterator = pTransaction->mapOriginalData.begin(); nIterator != pTransaction->mapOriginalData.end(); nIterator++ )
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
			
			if(GetArg("-verbose", 0) >= 4)
				printf("TransactionCommit() : Commiting Transactin to Datachain.\n");
			
			/** Check that there is a valid transaction to apply to the database. **/
			if(!pTransaction)
				return error("TransactionCommit() : No Transaction data to Commit.");
			
			/** Get the Keychain from the Sector Database. **/
			KeyDatabase* SectorKeys = GetKeychain(strKeychainRegistry);
			if(!SectorKeys)
				return error("TransactionCommit() : Sector Keys not Registered for Name %s\n", strKeychainRegistry.c_str());
			
			/** Habdle setting the sector key flags so the database knows if the transaction was completed properly. **/
			if(GetArg("-verbose", 0) >= 4)
				printf("TransactionCommit() : Commiting Keys to Keychain.\n");
			
			/** Set the Sector Keys to an Invalid State to know if there are interuptions the sector was not finished successfully. **/
			for(typename std::map< std::vector<unsigned char>, std::vector<unsigned char> >::iterator nIterator = pTransaction->mapTransactions.begin(); nIterator != pTransaction->mapTransactions.end(); nIterator++ )
			{
				SectorKey cKey;
				if(SectorKeys->HasKey(nIterator->first)) {
					if(!SectorKeys->Get(nIterator->first, cKey))
						return error("CommitTransaction() : Couldn't get the Active Sector Key.");
					
					cKey.nState = TRANSACTION;
					SectorKeys->Put(cKey);
				}
			}
			
			/** Update the Keychain with Checksums and READY Flag letting sectors know they were written successfully. **/
			if(GetArg("-verbose", 0) >= 4)
				printf("TransactionCommit() : Erasing Sector Keys Flagged for Deletingn.\n");
			
			/** Erase all the Transactions that are set to be erased. That way if they are assigned a TRANSACTION flag we know to roll back their key to orginal data. **/
			for(typename std::map< std::vector<unsigned char>, unsigned int >::iterator nIterator = pTransaction->mapEraseData.begin(); nIterator != pTransaction->mapEraseData.end(); nIterator++ )
			{
				if(!SectorKeys->Erase(nIterator->first))
					return error("CommitTransaction() : Couldn't get the Active Sector Key for Delete.");
			}
			
			/** Commit the Sector Data to the Database. **/
			if(GetArg("-verbose", 0) >= 4)
				printf("TransactionCommit() : Commit Data to Datachain Sector Database.\n");
			
			for(typename std::map< std::vector<unsigned char>, std::vector<unsigned char> >::iterator nIterator = pTransaction->mapTransactions.begin(); nIterator != pTransaction->mapTransactions.end(); nIterator++ )
			{
				/** Declare the Key and Data for easier reference. **/
				std::vector<unsigned char> vKey  = nIterator->first;
				std::vector<unsigned char> vData = nIterator->second;
				
				/** Write Header if First Update. **/
				if(!SectorKeys->HasKey(vKey))
				{
					/** TODO:: Assign a Sector File based on Database Sizes. **/
					unsigned short nSectorFile = 0;
					
					/** Open the Stream to Read the data from Sector on File. **/
					std::fstream fStream(strprintf("%s%s%u.dat", strBaseLocation.c_str(), strBaseName.c_str(), nSectorFile).c_str(), std::ios::in | std::ios::out | std::ios::binary);
					
					/** Create a new Sector Key. **/
					SectorKey cKey(TRANSACTION, vKey, nSectorFile, 0, vData.size());
					
					/** If it is a New Sector, Assign a Binary Position. 
						TODO: Track Sector Database File Sizes. **/
					unsigned int nBegin = fStream.tellg();
					fStream.seekg (0, std::ios::end);
					
					/** Find the Binary Starting Location. **/
					unsigned int nStart = (unsigned int) fStream.tellg() - nBegin;
					
					/** Assign the Key to Keychain. **/
					cKey.nSectorStart = nStart;
					SectorKeys->Put(cKey);
					
					/** Write the Data to the Sector. **/
					fStream.seekp(nStart, std::ios::beg);
					fStream.write((char*) &vData[0], vData.size());
					fStream.close();
				}
				else
				{
					/** Get the Sector Key from the Keychain. **/
					SectorKey cKey;
					if(!SectorKeys->Get(vKey, cKey))
						return false;
						
					/** Open the Stream to Read the data from Sector on File. **/
					std::fstream fStream(strprintf("%s%s%u.dat", strBaseLocation.c_str(), strBaseName.c_str(), cKey.nSectorFile).c_str(), std::ios::in | std::ios::out | std::ios::binary);
					
					/** Locate the Sector Data from Sector Key. 
						TODO: Make Paging more Efficient in Keys by breaking data into different locations in Database. **/
					fStream.seekp(cKey.nSectorStart, std::ios::beg);
					if(vData.size() > cKey.nSectorSize){
						fStream.close();
						printf("ERROR PUT (TOO LARGE) NO TRUNCATING ALLOWED (Old %u :: New %u):%s\n", cKey.nSectorSize, vData.size(), HexStr(vData.begin(), vData.end()).c_str());
						
						return false;
					}
					
					/** Assign the Writing State for Sector. **/
					fStream.write((char*) &vData[0], vData.size());
					fStream.close();
				}
			}
			
			/** Update the Keychain with Checksums and READY Flag letting sectors know they were written successfully. **/
			if(GetArg("-verbose", 0) >= 4)
				printf("TransactionCommit() : Commiting Key Valid States to Keychain.\n");
			
			for(typename std::map< std::vector<unsigned char>, std::vector<unsigned char> >::iterator nIterator = pTransaction->mapTransactions.begin(); nIterator != pTransaction->mapTransactions.end(); nIterator++ )
			{
				/** Assign the Writing State for Sector. **/
				SectorKey cKey;
				if(!SectorKeys->Get(nIterator->first, cKey))
					return error("CommitTransaction() : Failed to Get Key from Keychain.");
				
				/** Set the Sector states back to Active. **/
				cKey.nState    = READY;
				cKey.nChecksum = LLC::HASH::SK64(nIterator->second);
				
				/** Commit the Keys to Keychain Database. **/
				if(!SectorKeys->Put(cKey))
					return error("CommitTransaction() : Failed to Commit Key to Keychain.");
			}
			
			/** Clean up the Sector Transaction Key. 
				TODO: Delete the Sector and Keychain for Current Transaction Commit ID. **/
			delete pTransaction;
			pTransaction = NULL;
			
			return true;
		}
	};
}

#endif
