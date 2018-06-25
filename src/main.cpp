
#include "TAO/Core/include/sigchain.h"

#include "Util/include/debug.h"
#include "Util/include/runtime.h"
#include "Util/include/hex.h"

#include "LLC/hash/SK.h"
#include "LLC/include/random.h"
#include "LLC/include/key.h"

#include "LLP/templates/server.h"

#include "LLD/templates/sector.h"
#include "LLD/templates/hashmap.h"
#include "LLD/templates/filemap.h"

#include "leveldb/db.h"

#include <db_cxx.h>

#include <sys/time.h>

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


class TestDB : public LLD::SectorDatabase<LLD::BinaryFileMap>
{
public:
    TestDB(const char* pszMode="r+") : SectorDatabase("testdb", pszMode) {}

    bool WriteBlock(uint1024 hash, CBlock blk)
    {
        return Write(hash, blk);
    }

    bool ReadBlock(uint1024 hash, CBlock& blk)
    {
        return Read(hash, blk);
    }
};


//NETF - ADS - Application Development Standard - Document to define new applicaqtion programming interface calls required by developers
//NETF - NOS - Nexus Operation Standard - Document to define operation needs, formal design, and byte slot, and NETF engineers to develop the CASE statement
//NETF - ORS - Object Register Standard - Document to define a specific object register for purpose of ADS standards, with NOS standards being capable of supporting methods


/** Operation Layer Byte Code. **/
enum
{
    //register operations
    OP_WRITE      = 0x01, //OP_WRITE <vchAddress> <vchData> return fSuccess
    OP_READ       = 0x02, //OP_READ <vchAddress> return <vchData>
    OP_REGISTER   = 0x03, //OP_REGISTER <vchAddress> return fSuccess
    OP_AUTHORIZE  = 0x04, //OP_AUTHORIZE OP_GETHASH <vchPubKey> return fSuccess
    OP_TRANSFER   = 0x05, //OP_TRANSFER <vchAddress> <vchGenesisID> return fSuccess
    OP_GETHASH    = 0x06, //OP_GETHASH <vchData> return vchHashData
    OP_EXPIRE     = 0x07, //OP_EXPIRE <nTimestamp> return fExpire
    //0x08 - 0x0f UNASSIGNED


    //conditional operations
    OP_IF         = 0x10, //OP_IF <boolean expression>
    OP_ELSE       = 0x11, //OP_ELSE
    OP_ENDIF      = 0x12, //OP_ENDIF
    OP_NOT        = 0x13, //OP_NOT <bool expression>
    OP_EQUALS     = 0x14, //OP_EQUALS <vchData1> <VchData2>
    //0x15 - 0x1f UNASSIGNED


    //financial operators
    OP_ACCOUNT    = 0x20, //OP_ACCOUNT <vchAddress> <vchIdentifier> return fSuccess - create account
    OP_CREDIT     = 0x21, //OP_CREDIT <hashTransaction> <nAmount> return fSuccess
    OP_DEBIT      = 0x22, //OP_DEBIT <vchAccount> <nAmount> return fSuccess
    OP_BALANCE    = 0x23, //OP_BALANCE <vchAccount> <vchIdentifier> return nBalance


    //joint ownership TODO (I own 50% of this copyright, you own 50%, when a royalty transaction hits, disperse to accounts)
    OP_LICENSE    = 0x30, //OP_LICENSE <vchAddress>


    //chain state operations
    OP_HEIGHT     = 0x40, //OP_HEIGHT return nHeight
    OP_TIMESTAMP  = 0x41, //OP_TIMESTAMP return UnifiedTimestamp()
    OP_TXID       = 0x42, //OP_TXID return GetHash() - callers transaction hash
};

class COperation
{
    std::vector<unsigned char> vchRegisters[4]; //MAX SIZE 64 bytes - 512bit hash

    std::vector<unsigned char> vchOperations;

    COperation(std::vector<unsigned char> vchOperationsIn) : vchOperations(vchOperationsIn) {}

    bool Execute()
    {
        switch()
        {
            case OP_WRITE:
            break;

            case OP_READ:
            break;

            case OP_CREDIT:
            break;

            case OP_DEBUT:
            break;

            case OP_REGISTER:
            break;

            case OP_AUTHORIZE:
            break;

            case OP_TRANSFER:
            break;

            case: OP_GETHASH:
            break;

            case OP_ACCOUNT:
            break;

            case OP_EXPIRE:
            break;

            case OP_IF:
            break;

            case OP_ELSE:
            break;

            case OP_ENDIF:
            break;
        }
    }
}


class CStateRegister
{
public:

    /** Flag to determine if register is Read Only. **/
    bool fReadOnly;


    /** The version of the state of the register. **/
    unsigned short nVersion;


    /** The length of the state register. **/
    unsigned short nLength;


    /** The byte level data of the register. **/
    std::vector<unsigned char> vchState;


    /** The address space of the register. **/
    uint256 hashAddress;


    /** The chechsum of the state register for use in pruning. */
    uint64 hashChecksum;


    IMPLEMENT_SERIALIZE
    (
        READWRITE(fReadOnly);
        READWRITE(nVersion);
        READWRITE(nLength);
        READWRITE(vchState);
        READWRITE(hashAddress);
        READWRITE(hashChecksum);
    )


    CStateRegister() : fReadOnly(false), nVersion(1), nLength(0), hashAddress(0), hashChecksum(0)
    {
        vchState.clear();
    }


    CStateRegister(std::vector<unsigned char> vchData) : fReadOnly(false), nVersion(1), nLength(vchData.size()), vchState(vchData), hashAddress(0), hashChecksum(0)
    {

    }


    CStateRegister(uint64 hashChecksumIn) : fReadOnly(false), nVersion(1), nLength(0), hashAddress(0), hashChecksum(hashChecksumIn)
    {

    }


    /** Set the State Register into a NULL state. **/
    void SetNull()
    {
        nVersion = 1;
        hashAddress = 0;
        nLength   = 0;
        vchState.size() == 0;
        hashChecksum == 0;
    }


    /** NULL Checking flag for a State Register. **/
    bool IsNull()
    {
        return (nVersion == 1 && hashAddress == 0 && nLength == 0 && vchState.size() == 0 && hashChecksum == 0);
    }


    /** Flag to determine if the state register has been pruned. **/
    bool IsPruned()
    {
        return (fReadOnly == true && nVersion == 0 && nLength == 0 && vchState.size() == 0 && hashChecksum != 0);
    }


    /** Set the Memory Address of this Register's Index. **/
    void SetAddress(uint256 hashAddressIn)
    {
        hashAddress = hashAddressIn;
    }


    /** Set the Checksum of this Register. **/
    void SetChecksum()
    {
        hashChecksum = LLC::HASH::SK64(BEGIN(nVersion), END(hashAddress));
    }


    /** Get the State from the Register. **/
    std::vector<unsigned char> GetState()
    {
        return vchState;
    }


    /** Set the State from Byte Vector. **/
    void SetState(std::vector<unsigned char> vchStateIn)
    {
        vchState = vchStateIn;
        nLength  = vchStateIn.size();

        SetChecksum();
    }

};



class CTritiumTransaction
{

    /** The transaction version for extensibility. **/
    unsigned int nVersion;

    /** The computed hash of the next key in the series. **/
    uint256 hashNext;

    /** The genesis ID of this signature chain. **/
    uint256 hashGenesis;

    /** The serialized byte code that is to be encoded in the chain. **/
    std::vector<unsigned char> vchLedgerData;

    /** MEMORY ONLY: The Binary data of the Public Key revealed in this transaction. **/
    mutable std::vector<unsigned char> vchPubKey;

    /** MEMORY ONLY: The Binary data of the Signature revealed in this transaction. **/
    mutable std::vector<unsigned char> vchSignature;

    IMPLEMENT_SERIALIZE
    (
        READWRITE(nVersion);
        READWRITE(hashNext);
        READWRITE(FLATDATA(vchLedgerData));

        if(!(nType & SER_LLD)) {
          READWRITE(vchPubKey);
          READWRITE(vchSignature);
        }
    }


    //TODO: Get this indexing right
    uint512 GetPrevHash() const
    {
        //return LLC::HASH::SK256(vchPubKey); //INDEX
    }


    uint512 GetHash() const
    {
        return LLC::HASH::SK512(BEGIN(nVersion), END(vchLedgerData));
    }


    /** IsValid
     *
     *  Checks if the transaction is valid. Only called on validation process, not to be run from disk since it requires memory only data.
     *
     *   @return Returns whether the key is valid.
     */
    bool IsValid()
    {
        /* Generate the key object to check signature. */
        LLC::CKey key(NID_brainpoolP512t1, 64);
        key.SetPubKey(vchPubKey);

        /* Verify that the signature is valid. */
        if(!key.Verify(GetHash().GetBytes(), vchSignature))
            return false;
    }

};


int main(int argc, char** argv)
{
    ParseParameters(argc, argv);

    for(int i = 0; ; i++)
    {
        uint512 hashSeed = GetRand512();
        uint256 hash256  = GetRand256();

        Core::SignatureChain sigChain("username", hashSeed.ToString());
        uint512 hashGenesis = sigChain.Generate(i, hash256.ToString());

        printf("Genesis Priv %s\n", hashGenesis.ToString().c_str());

        LLC::CKey key(NID_brainpoolP512t1, 64);

        std::vector<unsigned char> vchData = hashGenesis.GetBytes();

        LLC::CSecret vchSecret(vchData.begin(), vchData.end());
        if(!key.SetSecret(vchSecret, true))
            return 0;

        LLC::CKey key512(key);

        std::vector<unsigned char> vPubKey = key.GetPubKey();

        printf("Genesis Pub %s\n", HexStr(vPubKey).c_str());

        uint512 genesisID = LLC::HASH::SK512(vPubKey);

        printf("Genesis ID %s\n", genesisID.ToString().c_str());

        printf("Keys Created == %s\n", (key == key512) ? "TRUE" : "FALSE");

        std::vector<unsigned char> vchSig;
        bool fSigned = key512.Sign(hashSeed, vchSig, 256);
        //vchSig[0] = 0x11;

        LLC::CKey keyVerify(NID_brainpoolP512t1, 64);
        keyVerify.SetPubKey(vPubKey);

        bool fVerify = keyVerify.Verify(hashSeed, vchSig, 256);
        printf("[%i] Signature %s [%u Bytes] Signed %s and Verified %s\n", i, HexStr(vchSig).c_str(), vchSig.size(), fSigned ? "True" : "False", fVerify ? "True" : "False");

        if(!fVerify)
            return 0;
    }

    return 0;
}



int TestLLD(int argc, char** argv)
{
    ParseParameters(argc, argv);

    printf("Lower Level Library Initialization...\n");

    TestDB* db = new TestDB();

    CBlock test;
    test.SetRandom();

    std::map<uint1024, CBlock> mapBlocks;
    std::vector<uint1024> vBlocks;
    CBlock bulkTest;
    bulkTest.SetRandom();

    unsigned int nTotalRecords = GetArg("-testwrite", 1);
    for(int i = 0; i < nTotalRecords; i++)
    {
        bulkTest.nChannel = i;
        mapBlocks[bulkTest.GetHash()] = bulkTest;
        vBlocks.push_back(bulkTest.GetHash());
    }

    printf(ANSI_COLOR_BRIGHT_BLUE "\nWriting %u Keys to Nexus LLD, Google LevelDB, and Oracle BerkeleyDB\n\n" ANSI_COLOR_RESET, nTotalRecords);

    unsigned int nTotalElapsed = 0;
    Timer timer;
    timer.Start();
    for(typename std::map< uint1024, CBlock >::iterator blk = mapBlocks.begin(); blk != mapBlocks.end(); blk++ )
    {
        db->WriteBlock(blk->first, blk->second);
    }

    unsigned int nElapsed = timer.ElapsedMicroseconds();
    printf(ANSI_COLOR_GREEN "LLD Write Performance: %u micro-seconds | %f ops/s\n" ANSI_COLOR_RESET, nElapsed, (nTotalRecords * 1000000.0) / nElapsed);
    nTotalElapsed += nElapsed;



    timer.Reset();
    std::random_shuffle(vBlocks.begin(), vBlocks.end());
    for(auto hash : vBlocks)
    {
        db->ReadBlock(hash, test);
    }

    nElapsed = timer.ElapsedMicroseconds();
    printf(ANSI_COLOR_GREEN "LLD Read Performance: %u micro-seconds | %f ops/s\n" ANSI_COLOR_RESET, nElapsed, (nTotalRecords * 1000000.0) / nElapsed);
    nTotalElapsed += nElapsed;

    timer.Reset();
    delete db;
    nElapsed = timer.ElapsedMicroseconds();
    printf(ANSI_COLOR_GREEN "LLD Destruct Performance: %u micro-seconds\n" ANSI_COLOR_RESET, nElapsed);
    nTotalElapsed += nElapsed;

    printf(ANSI_COLOR_YELLOW "LLD Total Running Time: %f seconds | %f ops/s\n\n" ANSI_COLOR_RESET, nTotalElapsed / 1000000.0, (nTotalRecords * 1000000.0) / nTotalElapsed);
    nTotalElapsed = 0;


    // Set up database connection information and open database
    leveldb::DB* ldb;
    leveldb::Options options;
    options.create_if_missing = true;
    options.block_size = 256000;

    leveldb::Status status = leveldb::DB::Open(options, GetDataDir().string() + "/leveldb" , &ldb);

    if (false == status.ok())
    {
        return error("Unable to Open Leveldb\n");
    }

    // Add 256 values to the database
    leveldb::WriteOptions writeOptions;
    leveldb::ReadOptions readoptions;

    timer.Reset();
    for(typename std::map< uint1024, CBlock >::iterator blk = mapBlocks.begin(); blk != mapBlocks.end(); blk++ )
    {
        CDataStream ssKey(SER_LLD, DATABASE_VERSION);
        ssKey << blk->first;

        std::vector<char> vKey(ssKey.begin(), ssKey.end());

        CDataStream ssData(SER_LLD, DATABASE_VERSION);
        ssData << blk->second;

        std::vector<char> vData(ssData.begin(), ssData.end());

        leveldb::Slice slKey(&vKey[0], vKey.size());
        leveldb::Slice slData(&vData[0], vData.size());

        ldb->Put(writeOptions, slKey, slData);
    }
    nElapsed = timer.ElapsedMicroseconds();
    printf(ANSI_COLOR_GREEN "LevelDB Write Performance: %u micro-seconds | %f ops/s\n" ANSI_COLOR_RESET, nElapsed, (nTotalRecords * 1000000.0) / nElapsed);
    nTotalElapsed += nElapsed;

    Sleep(2000);


    timer.Reset();
    std::random_shuffle(vBlocks.begin(), vBlocks.end());
    for(auto hash : vBlocks)
    {
        CDataStream ssKey(SER_LLD, DATABASE_VERSION);
        ssKey << hash;

        std::vector<char> vKey(ssKey.begin(), ssKey.end());

        leveldb::Slice slKey(&vKey[0], vKey.size());
        std::string strValue;

        ldb->Get(readoptions, slKey, &strValue);
    }
    nElapsed = timer.ElapsedMicroseconds();
    printf(ANSI_COLOR_GREEN "LevelDB Read Performance: %u micro-seconds | %f ops/s\n" ANSI_COLOR_RESET, nElapsed, (nTotalRecords * 1000000.0) / nElapsed);
    nTotalElapsed += nElapsed;

    timer.Reset();
    delete ldb;
    nElapsed = timer.ElapsedMicroseconds();
    printf(ANSI_COLOR_GREEN "LevelDB Destruct Performance: %u micro-seconds\n" ANSI_COLOR_RESET, nElapsed);
    nTotalElapsed += nElapsed;


    printf(ANSI_COLOR_YELLOW "LevelDB Total Running Time: %f seconds | %f ops/s\n\n" ANSI_COLOR_RESET, nTotalElapsed / 1000000.0, (nTotalRecords * 1000000.0) / nTotalElapsed);
    nTotalElapsed = 0;


    int nDbCache = GetArg("-dbcache", 25);
    DbEnv dbenv(0);
    dbenv.set_cachesize(nDbCache / 1024, (nDbCache % 1024)*1048576, 1);
    dbenv.set_lg_bsize(1048576);
    dbenv.set_lg_max(10485760);
    dbenv.set_lk_max_locks(10000);
    dbenv.set_lk_max_objects(10000);
    dbenv.set_flags(DB_TXN_WRITE_NOSYNC, 1);
    //dbenv.set_flags(DB_AUTO_COMMIT, 1);
    dbenv.log_set_config(DB_LOG_AUTO_REMOVE, 1);
    dbenv.open((GetDataDir().string() + "/bdb").c_str(),
                                DB_CREATE     |
                                DB_INIT_LOCK  |
                                DB_INIT_LOG   |
                                DB_INIT_MPOOL |
                                DB_INIT_TXN   |
                                DB_THREAD     |
                                DB_RECOVER, S_IRUSR | S_IWUSR);

    Db* pdb = new Db(&dbenv, 0);
    pdb->open(NULL, "bdb.dat", NULL, DB_BTREE, DB_CREATE | DB_THREAD, 0);

    timer.Reset();
    for(typename std::map< uint1024, CBlock >::iterator blk = mapBlocks.begin(); blk != mapBlocks.end(); blk++ )
    {
        CDataStream ssKey(SER_LLD, DATABASE_VERSION);
        ssKey << blk->first;

        CDataStream ssData(SER_LLD, DATABASE_VERSION);
        ssData << blk->second;

        Dbt datKey(&ssKey[0], ssKey.size());
        Dbt datValue(&ssData[0], ssData.size());

        // Write
        pdb->put(0, &datKey, &datValue, 0);
    }

    nElapsed = timer.ElapsedMicroseconds();
    printf(ANSI_COLOR_GREEN "BerkeleyDB Write Performance: %u micro-seconds | %f ops/s\n" ANSI_COLOR_RESET, nElapsed, (nTotalRecords * 1000000.0) / nElapsed);
    nTotalElapsed += nElapsed;

    timer.Reset();
    std::random_shuffle(vBlocks.begin(), vBlocks.end());
    for(auto hash : vBlocks)
    {
        CDataStream ssKey(SER_LLD, DATABASE_VERSION);
        ssKey << hash;

        Dbt datKey(&ssKey[0], ssKey.size());
        Dbt datValue;
        datValue.set_flags(DB_DBT_MALLOC);

        // Write
        pdb->get(0, &datKey, &datValue, 0);
    }

    nElapsed = timer.ElapsedMicroseconds();
    printf(ANSI_COLOR_GREEN "BerkeleyDB Read Performance: %u micro-seconds | %f ops/s\n" ANSI_COLOR_RESET, nElapsed, (nTotalRecords * 1000000.0) / nElapsed);
    nTotalElapsed += nElapsed;

    timer.Reset();
    delete pdb;
    nElapsed = timer.ElapsedMicroseconds();
    printf(ANSI_COLOR_GREEN "BerkeleyDB Destruct Performance: %u micro-seconds\n" ANSI_COLOR_RESET, nElapsed);
    nTotalElapsed += nElapsed;

    printf(ANSI_COLOR_YELLOW "BerkeleyDB Total Running Time: %f seconds | %f ops/s\n\n" ANSI_COLOR_RESET, nTotalElapsed / 1000000.0, (nTotalRecords * 1000000.0) / nTotalElapsed);
}
