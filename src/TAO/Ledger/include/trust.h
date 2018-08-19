/*__________________________________________________________________________________________
 
			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2017] ++
			
			(c) Copyright The Nexus Developers 2014 - 2017
			
			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.
			
			"ad vocem populi" - To The Voice of The People
  
____________________________________________________________________________________________*/

#ifndef NEXUS_CORE_INCLUDE_TRUST_H
#define NEXUS_CORE_INCLUDE_TRUST_H


#include "../../Util/templates/serialize.h"
#include "../../Util/include/mutex.h"
#include "../../Util/include/runtime.h"


#include "../../LLC/hash/SK.h"
#include "../../LLC/hash/macro.h"

namespace Wallet { class CWallet; }

namespace Core
{
	class CBlock;

	
	/* Class to Store the Trust Key and It's Interest Rate. */
	class CTrustKey
	{
	public:
	
		/* The Public Key associated with Trust Key. */
		std::vector<unsigned char> vchPubKey;
		
		unsigned int nVersion;
		LLC::uint1024  hashGenesisBlock;
		LLC::uint512   hashGenesisTx;
		unsigned int nGenesisTime;
		
		
		/* Previous Blocks Vector to store list of blocks of this Trust Key. */
		mutable std::vector<LLC::uint1024> hashPrevBlocks;
		
		
		CTrustKey() { SetNull(); }
		CTrustKey(std::vector<unsigned char> vchPubKeyIn, LLC::uint1024 hashBlockIn, LLC::uint512 hashTxIn, unsigned int nTimeIn)
		{
			SetNull();
			
			nVersion               = 1;
			vchPubKey              = vchPubKeyIn;
			hashGenesisBlock       = hashBlockIn;
			hashGenesisTx          = hashTxIn;
			nGenesisTime           = nTimeIn;
		}
		
		
		IMPLEMENT_SERIALIZE
		(
			READWRITE(this->nVersion);
			nVersion = this->nVersion;
			
			READWRITE(vchPubKey);
			READWRITE(hashGenesisBlock);
			READWRITE(hashGenesisTx);
			READWRITE(nGenesisTime);
		)
		
		
		/* Set the Data structure to Null. */
		void SetNull() 
		{ 
			nVersion             = 1;
			hashGenesisBlock     = 0;
			hashGenesisTx        = 0;
			nGenesisTime         = 0;
			
			vchPubKey.clear();
		}
		
		
		/* Hash of a Trust Key to Verify the Key's Root. */
		LLC::uint512 GetHash() const { return LLC::SK512(vchPubKey, BEGIN(hashGenesisBlock), END(nGenesisTime)); }
		
		
		/* Determine how old the Trust Key is From Timestamp. */
		uint64_t Age(unsigned int nTime) const;
		
		
		/* Time Since last Trust Block. */
		uint64_t BlockAge(unsigned int nTime) const;
		
		
		/* Flag to Determine if Class is Empty and Null. */
		bool IsNull()  const { return (hashGenesisBlock == 0 || hashGenesisTx == 0 || nGenesisTime == 0 || vchPubKey.empty()); }
		
		
		/* Determine if a key is expired by its age. */
		bool Expired(unsigned int nTime) const;
		
		
		/* Check the Genesis Transaction of this Trust Key. */
		bool CheckGenesis(CBlock cBlock) const;
		
		
		/* Dump to Console information about current Trust Key. */
		std::string ToString()
		{
			LLC::uint576 cKey;
			cKey.SetBytes(vchPubKey);
			
			return strprintf("CTrustKey(Hash = %s, Key = %s, Genesis = %s, Tx = %s, Time = %u, Age = %" PRIu64 ", BlockAge = %" PRIu64 ", Expired = %s)", GetHash().ToString().c_str(), cKey.ToString().c_str(), hashGenesisBlock.ToString().c_str(), hashGenesisTx.ToString().c_str(), nGenesisTime, Age(Timestamp()), BlockAge(Timestamp()), Expired(Timestamp()) ? "TRUE" : "FALSE");
		}
		
		void Print(){ printf("%s\n", ToString().c_str()); }
	};	
	
	
	/* Holding Class Structure to contain the Trust Keys. */
	class CTrustPool
	{
	public:
		
		/* Trust Keys internal Map. */
		mutable std::map<LLC::uint576, CTrustKey> mapTrustKeys;
		
		
		/* Internal Mutex. */
		Mutex_t MUTEX;
		
		
		/* The Trust Key Owned By Current Node. */
		std::vector<unsigned char>   vchTrustKey;
		
		
		/* Helper Function to Find Trust Key. */
		bool HasTrustKey(unsigned int nTime);
		
		
		/* Check the Validity of a Trust Key. */
		bool Check(const CBlock& cBlock);
		
		
		/* Accept a Trust Key into the Trust Pool. */
		bool Accept(const CBlock& cBlock, bool fInit = false);
		
		
		/* Remove a Trust Key from the Trust Pool. */
		bool Remove(const CBlock& cBlock);
		
		
		/* Check if a Trust Key Exists in the Trust Pool. */
		bool Exists(LLC::uint576 cKey)    const { return mapTrustKeys.count(cKey); }
		
		
		/* Detect if this key is still a genesis key. */
		bool IsGenesis(LLC::uint576 cKey) const { return mapTrustKeys[cKey].hashPrevBlocks.empty(); }
		
		
		/* Calculate the Interest Rate of given trust key. */
		double InterestRate(LLC::uint576 cKey, unsigned int nTime) const;
		
		
		/* The Trust score of the Trust Key. Determines the Age and Interest Rates. */
		uint64_t TrustScore(LLC::uint576 cKey, unsigned int nTime) const;
		
		
		/* Locate a Trust Key in the Trust Pool. */
		CTrustKey Find(LLC::uint576 cKey) const { return mapTrustKeys[cKey]; }
	};
	
	
	/* The holding location in memory of all the trust keys on the network and their corresponding statistics. */
	extern CTrustPool cTrustPool;
	
	
	/* __________________________________________________ (Trust Key Methods) __________________________________________________  */
	
	
	
	/* Method to Fire up the Staking Thread. */
	void StartStaking(Wallet::CWallet *pwallet);
	
	
	/* Declration of the Function that will act for the Staking Thread. */
	void StakeMinter(void* parg);
	
}


#endif
