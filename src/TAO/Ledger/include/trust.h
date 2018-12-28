/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2018

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To The Voice of The People

____________________________________________________________________________________________*/

#ifndef NEXUS_TAO_LEDGER_INCLUDE_TRUST_H
#define NEXUS_TAO_LEDGER_INCLUDE_TRUST_H

namespace Core
{
	class CBlock;


	/* Class to Store the Trust Key and It's Interest Rate. */
	class CTrustKey
	{
	public:

		/* The Public Key associated with Trust Key. */
		std::vector<uint8_t> vchPubKey;

		uint32_t nVersion;
		uint1024_t  hashGenesisBlock;
		uint512_t   hashGenesisTx;
		uint32_t nGenesisTime;


		/* Previous Blocks Vector to store list of blocks of this Trust Key. */
		mutable std::vector<uint1024_t> hashPrevBlocks;


		CTrustKey() { SetNull(); }
		CTrustKey(std::vector<uint8_t> vchPubKeyIn, uint1024_t hashBlockIn, uint512_t hashTxIn, uint32_t nTimeIn)
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
		uint512_t GetHash() const { return LLC::SK512(vchPubKey, BEGIN(hashGenesisBlock), END(nGenesisTime)); }


		/* Determine how old the Trust Key is From timestamp. */
		uint64_t Age(uint32_t nTime) const;


		/* Time Since last Trust Block. */
		uint64_t BlockAge(uint32_t nTime) const;


		/* Flag to Determine if Class is Empty and Null. */
		bool IsNull()  const { return (hashGenesisBlock == 0 || hashGenesisTx == 0 || nGenesisTime == 0 || vchPubKey.empty()); }


		/* Determine if a key is expired by its age. */
		bool Expired(uint32_t nTime) const;


		/* Check the Genesis Transaction of this Trust Key. */
		bool CheckGenesis(CBlock cBlock) const;


		/* Dump to Console information about current Trust Key. */
		std::string ToString()
		{
			uint576_t cKey;
			cKey.SetBytes(vchPubKey);

			return strprintf("CTrustKey(Hash = %s, Key = %s, Genesis = %s, Tx = %s, Time = %u, Age = %" PRIu64 ", BlockAge = %" PRIu64 ", Expired = %s)", GetHash().ToString().c_str(), cKey.ToString().c_str(), hashGenesisBlock.ToString().c_str(), hashGenesisTx.ToString().c_str(), nGenesisTime, Age(timestamp()), BlockAge(runtime::timestamp()), Expired(runtime::timestamp()) ? "TRUE" : "FALSE");
		}

		void Print(){ debug::log(0, "%s", ToString().c_str()); }
	};
}


#endif
