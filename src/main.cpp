//
//  main.cpp
//  
//
//  Created by Colin Cantrell on 1/2/18.
//
//

#include "Util/include/debug.h"
#include "LLD/templates/sector.h"
#include "LLC/hash/SK.h"
#include "LLC/include/random.h"

class CBlock
{
public:
	unsigned int nBlkVersion;
	uint1024 hashPrevBlock;
	uint512 hashMerkleRoot;
	unsigned int nHeight;
	unsigned int nChannel;
	unsigned int nBits;
	uint64 nNonce;
	
	IMPLEMENT_SERIALIZE
	(
		READWRITE(nBlkVersion);
		//nVersion = this->nVersion;
		READWRITE(hashPrevBlock);
		READWRITE(hashMerkleRoot);
		READWRITE(nHeight);
		READWRITE(nChannel);
		READWRITE(nBits);
		READWRITE(nNonce);
	)
	
	CBlock()
	{
		SetNull();
	}
	
	void SetNull()
	{
		nBlkVersion = 0;
		hashPrevBlock = 0;
		hashMerkleRoot = 0;
		nHeight = 0;
		nChannel = 0;
		nNonce   = 0;
	}
	
	void SetRandom()
	{
		nBlkVersion = GetRandInt(1000);
		nHeight  = GetRandInt(1000);
		nChannel = GetRandInt(1000);
		nBits    = GetRandInt(1000);
		nNonce   = GetRand(1000000);
		
		hashMerkleRoot = GetRand512();
		hashPrevBlock  = GetRand1024();
	}
	
	uint1024 GetHash() const
	{
		return LLC::HASH::SK1024(BEGIN(nBlkVersion), END(nNonce));
	}
	
	void Print()
	{
		printf("CBlock(nVersion=%u, hashPrevBlock=%s, hashMerkleRoot=%s, nHeight=%u, nChannel=%u, nBits=%u, nNonce=%" PRIu64 ", hash=" ANSI_COLOR_BRIGHT_BLUE "%s" ANSI_COLOR_RESET ")\n", nBlkVersion, hashPrevBlock.ToString().c_str(), hashMerkleRoot.ToString().c_str(), nHeight, nChannel, nBits, nNonce, GetHash().ToString().c_str());
	}
};


class TestDB : public LLD::SectorDatabase
{
public:
	TestDB(const char* pszMode="r+") : SectorDatabase("testdb", "testdb", pszMode) {}
	
    bool WriteBlock(uint1024 hash, CBlock blk)
	{
		return Write(hash, blk);
	}
	
	bool ReadBlock(uint1024 hash, CBlock& blk)
	{
		return Read(hash, blk);
	}
};

int main(int argc, char** argv)
{
    printf("Lower Level Library Initialization...\n");
	
	TestDB db;
	
	CBlock test;
	test.SetRandom();
	
	db.WriteBlock(test.GetHash(), test);
	test.Print();
	
	CBlock newTest;
	db.ReadBlock(test.GetHash(), newTest);
	newTest.Print();
	
	CBlock newTestToo;
	db.ReadBlock(test.GetHash(), newTestToo);
	newTestToo.Print();
    
    return 0;
}
