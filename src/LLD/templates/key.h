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
		//TODO: Implement Multikeys 
		//unsigned short					nSector;
		
		/** These three hold the location of 
			Sector in the Sector Database of 
			Given Sector Key. **/
		unsigned short 			   nSectorFile;
		unsigned short   		   	nSectorSize;
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
		uint64 nChecksum;
		
		/** Serialization Macro. **/
		IMPLEMENT_SERIALIZE
		(
			READWRITE(nState);
			READWRITE(nLength);
			//READWRITE(nSector);
			
			if (!(nType & SER_LLD_KEY_HEADER))
			{
				READWRITE(nSectorFile);
				READWRITE(nSectorSize);
				READWRITE(nSectorStart);
				READWRITE(nChecksum);
			}
		)
		
		/** Constructors. **/
		SectorKey() : nState(0), nLength(0), nSectorFile(0), nSectorSize(0), nSectorStart(0) { }
		SectorKey(unsigned char nStateIn, std::vector<unsigned char> vKeyIn, unsigned short nSectorFileIn, unsigned int nSectorStartIn, unsigned short nSectorSizeIn) : nState(nStateIn), nSectorFile(nSectorFileIn), nSectorSize(nSectorSizeIn), nSectorStart(nSectorStartIn)
		{ 
			nLength = vKeyIn.size();
			vKey    = vKeyIn;
		}
		
		/** Return the Size of the Key Sector on Disk. **/
		unsigned int Size() { return (19 + nLength); }
		
		/** Check for Key Activity on Sector. **/
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
		
	public:	
		/** Map to Contain the Binary Positions of Each Key.
			Used to Quickly Read the Database File at Given Position
			To Obtain the Record from its Database Key. This is Read
			Into Memory on Database Initialization. **/
		mutable typename std::map< std::vector<unsigned char>, unsigned int > mapKeys;
		
        
		/** Caching Memory Map. Keep within caching limits. Pop on and off like a stack
			using seperate caching class if over limits. **/
		mutable typename std::map< std::vector<unsigned char>, SectorKey > mapKeysCache;
		
        
		/** The Database Constructor. To determine file location and the Bytes per Record. **/
		KeyDatabase(std::string strBaseLocationIn, std::string strDatabaseNameIn) : strBaseLocation(strBaseLocationIn), strDatabaseName(strDatabaseNameIn) { strLocation = strBaseLocation + strDatabaseName + ".keys"; }
		
		/** Clean up Memory Usage. **/
		~KeyDatabase() {
			mapKeys.clear();
		}
		
		/** Return the Keys to the Records Held in the Database. **/
		std::vector< std::vector<unsigned char> > GetKeys()
		{
			std::vector< std::vector<unsigned char> > vKeys;
			for(typename std::map< std::vector<unsigned char>, unsigned int >::iterator nIterator = mapKeys.begin(); nIterator != mapKeys.end(); nIterator++ )
				vKeys.push_back(nIterator->first);
				
			return vKeys;
		}
		
		/** Return Whether a Key Exists in the Database. **/
		bool HasKey(std::vector<unsigned char> vKey) { return mapKeys.count(vKey); }
		
		
		/** Read the Database Keys and File Positions. **/
		void Initialize()
		{
			LOCK(KEY_MUTEX);
			
			/** Open the Stream to Read from the File Stream. **/
			std::fstream fIncoming(strLocation.c_str(), std::ios::in | std::ios::binary);
			if(!fIncoming)
			{
				printf("[DATABASE] Creating %s Keychain....\n", strDatabaseName.c_str());
				
				/** Create the Sector Database Directories. **/
				boost::filesystem::path dir(strBaseLocation);
				boost::filesystem::create_directory(dir);
			
				std::ofstream cStream(strLocation.c_str(), std::ios::binary);
				cStream.close();
					
				return;
			}
			
			/** Get the Binary Size. **/
			fIncoming.seekg (0, std::ios::end);
			unsigned int nBinarySize = fIncoming.tellg();
			
			/** Iterator for Key Sectors. **/
			fIncoming.seekg (0, std::ios::beg);
			while(fIncoming.tellg() < nBinarySize)
			{
				/** Get the current file pointer position. **/
				unsigned int nPosition = fIncoming.tellg();
				
				/** Read the State and Size of Sector Header. **/
				std::vector<unsigned char> vHeader(fMemoryCaching ? 19 : 3, 0);
				fIncoming.read((char*) &vHeader[0], vHeader.size());
				
				SectorKey cKey;
				CDataStream ssKey(vHeader, fMemoryCaching ? SER_LLD : SER_LLD_KEY_HEADER, DATABASE_VERSION);
				ssKey >> cKey;
				
				
				/** Skip Empty Sectors for Now. 
                    TODO: Handle any sector and keys gracfully here to ensure that the Sector is returned to a valid state from the transaction journal in case there was a failure reading and writing in the sector. This will most likely be held in the sector database code. **/
				if(cKey.Ready()) {
				
					/** Read the Key Data. **/
					std::vector<unsigned char> vKey(cKey.nLength, 0);
					fIncoming.seekg(nPosition + 19);
					fIncoming.read((char*) &vKey[0], vKey.size());
					
					/** Set the Key Data. **/
					mapKeys[vKey] = nPosition;
					
					/** Set the Memory Cache. **/
					//if(fMemoryCaching)
					//	mapKeysCache[vKey] = cKey;
					
					/** Debug Output of Sector Key Information. **/
					if(GetArg("-verbose", 0) >= 5)
						printf("KeyDB::Load() : State: %u Length: %u Location: %u Key: %s\n", cKey.nState, cKey.nLength, mapKeys[vKey], HexStr(vKey.begin(), vKey.end()).c_str());
				}
				else {
					/** Move the Cursor to the Next Record. **/
					fIncoming.seekg(nPosition + cKey.Size(), std::ios::beg);
                    
                    /** Debug Output of Sector Key Information. **/
					if(GetArg("-verbose", 0) >= 3)
						printf("KeyDB::Load() : Skiping Sector State: %u Length: %u\n", cKey.nState, cKey.nLength);
                }
			}
			
			fIncoming.close();
		}
		
		/** Add / Update A Record in the Database **/
		bool Put(SectorKey cKey) const
		{
			LOCK(KEY_MUTEX);
			
			/** Establish the Outgoing Stream. **/
			std::fstream fStream(strLocation.c_str(), std::ios::in | std::ios::out | std::ios::binary);
			
			/** Write Header if First Update. **/
			if(!mapKeys.count(cKey.vKey))
			{
				/** If it is a New Sector, Assign a Binary Position. **/
				unsigned int nBegin = fStream.tellg();
				fStream.seekg (0, std::ios::end);

				mapKeys[cKey.vKey] = (unsigned int) fStream.tellg() - nBegin;
			}
			
			/** Update the Key Memory Cache. **/
			if(fMemoryCaching)
				mapKeysCache[cKey.vKey] = cKey;
			
			/** Seek File Pointer **/
			fStream.seekp(mapKeys[cKey.vKey], std::ios::beg);
				
			/** Handle the Sector Key Serialization. **/
			CDataStream ssKey(SER_LLD, DATABASE_VERSION);
			ssKey.reserve(cKey.Size());
			ssKey << cKey;
				
			/** Write to Disk. **/
			std::vector<unsigned char> vData(ssKey.begin(), ssKey.end());
			vData.insert(vData.end(), cKey.vKey.begin(), cKey.vKey.end());
			fStream.write((char*) &vData[0], vData.size());
			fStream.close();
				
			/** Debug Output of Sector Key Information. **/
			if(GetArg("-verbose", 0) >= 4)
				printf("KEY::Put(): State: %s | Length: %u | Location: %u | Sector File: %u | Sector Size: %u | Sector Start: %u\n Key: %s\n", cKey.nState == READY ? "Valid" : "Invalid", cKey.nLength, mapKeys[cKey.vKey], cKey.nSectorFile, cKey.nSectorSize, cKey.nSectorStart, HexStr(cKey.vKey.begin(), cKey.vKey.end()).c_str());
			
			return true;
		}
		
		/** Simple Erase for now, not efficient in Data Usage of HD but quick to get erase function working. **/
		bool Erase(const std::vector<unsigned char> vKey)
		{
			/** Check for the Key. **/
			if(!mapKeys.count(vKey))
				return error("Key Erase() : Key doesn't Exist");
			
			/** Establish the Outgoing Stream. **/
			std::fstream fStream(strLocation.c_str(), std::ios::in | std::ios::out | std::ios::binary);
			fStream.seekp(mapKeys[vKey], std::ios::beg);
			
			/** Establish the Sector State as Empty. **/
			std::vector<unsigned char> vData(1, EMPTY);
			fStream.write((char*) &vData[0], vData.size());
			fStream.close();
				
			/** Remove the Sector Key from the Memory Map. **/
			mapKeys.erase(vKey);
			
			return true;
		}
		
		/** Get a Record from the Database with Given Key. **/
		bool Get(const std::vector<unsigned char> vKey, SectorKey& cKey)
		{
			LOCK(KEY_MUTEX);
			
			/** Cache Handler. **/
			if(mapKeysCache.count(vKey) && fMemoryCaching)
			{
				cKey = mapKeysCache[vKey];
				cKey.vKey = vKey;
				
				return true;
			}
			
			/** Read a Record from Binary Data. **/
			if(mapKeys.count(vKey))
			{
				/** Open the Stream Object. **/
				std::fstream fStream(strLocation.c_str(), std::ios::in | std::ios::binary);

				/** Seek to the Sector Position on Disk. **/
				fStream.seekg(mapKeys[vKey], std::ios::beg);
			
				/** Read the State and Size of Sector Header. **/
				std::vector<unsigned char> vData(19, 0);
				fStream.read((char*) &vData[0], 19);
				
				/** De-serialize the Header. **/
				CDataStream ssHeader(vData, SER_LLD, DATABASE_VERSION);
				ssHeader >> cKey;
				
				/** Debug Output of Sector Key Information. **/
				if(GetArg("-verbose", 0) >= 4)
					printf("KEY::Get(): State: %s Length: %u Location: %u Sector File: %u Sector Size: %u Sector Start: %u\n Key: %s\n", cKey.nState == READY ? "Valid" : "Invalid", cKey.nLength, mapKeys[cKey.vKey], cKey.nSectorFile, cKey.nSectorSize, cKey.nSectorStart, HexStr(cKey.vKey.begin(), cKey.vKey.end()).c_str());
						
				/** Skip Empty Sectors for Now. (TODO: Expand to Reads / Writes) **/
				if(cKey.Ready() || cKey.IsTxn()) {
				
					/** Read the Key Data. **/
					std::vector<unsigned char> vKeyIn(cKey.nLength, 0);
					fStream.read((char*) &vKeyIn[0], vKeyIn.size());
					fStream.close();
					
					/** Check the Keys Match Properly. **/
					if(vKeyIn != vKey)
						return error("Key Mistmatch: DB:: %s  MEM %s\n", HexStr(vKeyIn.begin(), vKeyIn.end()).c_str(), HexStr(vKey.begin(), vKey.end()).c_str());
					
					/** Assign Key to Sector. **/
					cKey.vKey = vKeyIn;
					
					return true;
				}
				
				/** Close the Stream. **/
				fStream.close();
			}
			
			return false;
		}
	};
}

#endif
