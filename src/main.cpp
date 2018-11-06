/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <Util/include/args.h>
#include <Util/include/config.h>
#include <Util/include/signals.h>
#include <Util/include/convert.h>
#include <Util/include/runtime.h>
#include <Util/include/filesystem.h>


#include <LLC/include/random.h>

#include <LLD/templates/sector.h>

#include <LLD/keychain/hashmap.h>
#include <LLD/keychain/filemap.h>

#include <LLD/cache/binary_lru.h>

#include <LLP/include/tritium.h>
#include <LLP/templates/server.h>
#include <LLP/include/legacy.h>

#include <TAO/Ledger/types/transaction.h>


class TestDB : public LLD::SectorDatabase<LLD::BinaryHashMap, LLD::BinaryLRU>
{
public:
    TestDB(const char* pszMode="r+") : SectorDatabase("testdb", pszMode) {}

    bool WriteTx(uint512_t hashTransaction, TAO::Ledger::Transaction tx)
    {
        return Write(std::make_pair(std::string("tx"), hashTransaction), tx);
    }

    bool ReadTx(uint512_t hashTransaction, TAO::Ledger::Transaction& tx)
    {
        return Read(std::make_pair(std::string("tx"), hashTransaction), tx);
    }
};



int main(int argc, char** argv)
{
    /* Handle all the signals with signal handler method. */
    SetupSignals();


    /* Parse out the parameters */
    ParseParameters(argc, argv);


    /* Create directories if they don't exist yet. */
    if(filesystem::create_directory(GetDataDir(false)))
        printf(FUNCTION "Generated Path %s\n", __PRETTY_FUNCTION__, GetDataDir(false).c_str());


    /* Read the configuration file. */
    ReadConfigFile(mapArgs, mapMultiArgs);



    /* Create the database instances. */
    //LLD::regDB = new LLD::RegisterDB("r+");
    //LLD::legDB = new LLD::LedgerDB("r+");


    /* Handle Commandline switch */
    for (int i = 1; i < argc; i++)
    {
        if (!IsSwitchChar(argv[i][0]) && !(strlen(argv[i]) >= 7 && strncasecmp(argv[i], "Nexus:", 7) == 0))
        {
            //int ret = Net::CommandLineRPC(argc, argv);
            //exit(ret);
        }
    }

    TestDB* test = new TestDB();

    uint512_t  hashTest("c861dffe8d1f5f59c05b726546b05a1e57742004317519a4dee454dcefb3f838c4005625d4799646aac8694aad41a9c447686d26da05a95fe5d20ce7ce979962");

    //TAO::Ledger::Transaction tx;
    //if(!test->ReadTx(hashTest, tx))
    //    return error("FAILED");

    //tx.print();


    int nCounter = 1;
    uint32_t nAverage = 0;
    Timer timer;
    timer.Start();

    TAO::Ledger::Transaction tx;
    tx.hashGenesis = LLC::GetRand256();
    uint512_t rand = LLC::GetRand512();
    //tx << rand << rand << rand << rand << rand << rand << rand << rand;
    uint512_t hash = tx.GetHash();
    uint512_t base = tx.GetHash();
    //tx.print();

    test->Write(hash, tx);

    //Sleep(1000);
    TAO::Ledger::Transaction tx2;
    //test->Read(hash, tx2);
    //tx2.print();



    LLP::Server<LLP::LegacyNode>* SERVER = new LLP::Server<LLP::LegacyNode>(GetArg("-port", 9323), 10, 30, false, 0, 0, 60, GetBoolArg("-listen", true), true);

    if(mapMultiArgs["-addnode"].size() > 0)
        for(auto node : mapMultiArgs["-addnode"])
            SERVER->AddConnection(node, GetArg("-port", 9323));


    while(!fShutdown)
        Sleep(1000);


    uint32_t wps = 0;
    uint32_t total = 0;
    while(!fShutdown)
    {
        Sleep(5, true);

        tx.hashGenesis = LLC::GetRand256();
        hash = tx.GetHash();
        //hash = hash + 1;

        //std::vector<uint8_t> vKey((uint8_t*)&hash, (uint8_t*)&hash + sizeof(hash));
        //std::vector<uint8_t> vData((uint8_t*)&tx, (uint8_t*)&tx + tx.GetSerializeSize(SER_DISK, LLD::DATABASE_VERSION));
        //cachePool->Put(vKey, vData);
        //test->Put(vKey, vData);
        test->Write(hash, tx);

        //LLC::SK256(hash.GetBytes());

        //TAO::Ledger::Transaction tx1;
        //test->ReadTx(hash, tx1);

        //tx1.print();
        //Sleep(10);

        if(nCounter % 100000 == 0)
        {
            timer.Stop();

            nAverage++;
            uint32_t nTimer = timer.ElapsedMilliseconds();
            wps += (100000.0 / nTimer);

            printf("100k records written in %u ms WPS = %uk / s\n", nTimer, wps / nAverage);

            timer.Reset();
            for(int i = 0; i < 100000; i++)
                test->Read(hash, tx);

            timer.Stop();

            printf("100k records read in %u ms\n", timer.ElapsedMilliseconds());

            timer.Reset();

            //Sleep(1000);
        }

        //Sleep(true, 1);

        nCounter++;
    }


    return 0;
}
