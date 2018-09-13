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

#include <TAO/Ledger/types/transaction.h>
#include <TAO/Ledger/types/sigchain.h>

#include <TAO/Register/objects/account.h>
#include <TAO/Register/objects/token.h>
#include <TAO/Register/include/state.h>
#include <TAO/Register/include/enum.h>

#include <TAO/Operation/include/enum.h>
#include <TAO/Operation/include/stream.h>
#include <TAO/Operation/include/execute.h>

#include <LLC/include/random.h>

#include <LLD/include/global.h>
#include <LLD/include/version.h>

LLD::RegisterDB*   regDB;
LLD::LedgerDB*     legDB;

int main(int argc, char** argv)
{
    /* Handle all the signals with signal handler method. */
    SetupSignals();


    /* Parse out the parameters */
    ParseParameters(argc, argv);


    /* Read the configuration file. */
    if (!boost::filesystem::is_directory(GetDataDir(false)))
    {
        fprintf(stderr, "Error: Specified directory does not exist\n");

        return 0;
    }
    ReadConfigFile(mapArgs, mapMultiArgs);


    /* Handle Commandline switch */
    for (int i = 1; i < argc; i++)
    {
        if (!IsSwitchChar(argv[i][0]) && !(strlen(argv[i]) >= 7 && strncasecmp(argv[i], "Nexus:", 7) == 0))
        {
            //int ret = Net::CommandLineRPC(argc, argv);
            //exit(ret);
        }
    }


    /* Setup Register Database. */
    regDB = new LLD::RegisterDB("r+");
    legDB =  new LLD::LedgerDB("r+");





    TAO::Operation::Stream test = TAO::Operation::Stream();

    uint256_t rand = LLC::GetRand256();

    test << rand;

    printf("memory %u\n", &rand);

    //test.print();

    uint256_t newRand;
    test >> newRand;

    printf("memory %u\n", &newRand);

    printf("rand1 %s\n", rand.ToString().c_str());
    printf("rand2 %s\n", newRand.ToString().c_str());



    TAO::Ledger::SignatureChain chain("username", "password");
    TAO::Ledger::Transaction genesis = TAO::Ledger::Transaction();
    genesis.NextHash(chain.Generate(genesis.nSequence + 1, "1111"));
    genesis.hashPrevTx  = 0; //sign of genesis
    genesis.nSequence   = 0; //sign of genesis
    genesis.hashGenesis = 0;

    CDataStream ssGen(SER_GENESISHASH, genesis.nVersion);
    ssGen << genesis;
    genesis.Sign(chain.Generate(genesis.nSequence, "1111"));

    if(genesis.IsValid())
        genesis.print();


    uint256_t hashGenesis = LLC::SK256(ssGen.begin(), ssGen.end());

    legDB->WriteTx(genesis.GetHash(), genesis);


    //create the first transaction
    TAO::Ledger::Transaction next = TAO::Ledger::Transaction();
    next.nSequence = 1;

    next.NextHash(chain.Generate(next.nSequence + 1, "1111"));
    next.hashPrevTx = genesis.GetHash();
    next.hashGenesis = hashGenesis;

    assert(next.hashGenesis == hashGenesis);

    //create an object register account
    TAO::Register::Account acct(105, 15);
    acct.print();

    printf("\n");

    //create a state register
    uint256_t hashRegister = LLC::GetRand256();
    TAO::Register::State state = TAO::Register::State((uint8_t)TAO::Register::OBJECT_ACCOUNT, hashRegister, hashGenesis);
    state << acct;

    //add the data to the ledger
    next << (uint8_t)TAO::Operation::OP_REGISTER << hashRegister << state;
    next.Sign(chain.Generate(next.nSequence, "1111"));
    if(next.IsValid())
        next.print();

    legDB->WriteTx(next.GetHash(), next);

    assert(next.nSequence  == genesis.nSequence + 1); //ledger rule
    assert(next.PrevHash() == genesis.hashNext);      //ledger rule


    TAO::Operation::Execute(next.vchLedgerData, regDB, legDB, next.hashGenesis);

    TAO::Register::State stateRead;
    if(!regDB->ReadState(hashRegister, stateRead))
        return error("Failed to read state");


printf("\n");
///////////////////////////second sigchain set

    TAO::Ledger::SignatureChain chain1("username", "password11");
    TAO::Ledger::Transaction genesis1 = TAO::Ledger::Transaction();
    genesis1.NextHash(chain1.Generate(genesis1.nSequence + 1, "1111"));
    genesis1.hashPrevTx  = 0; //sign of genesis
    genesis1.nSequence   = 0; //sign of genesis
    genesis1.hashGenesis = 0;

    CDataStream ssGen1(SER_GENESISHASH, genesis1.nVersion);
    ssGen1 << genesis1;
    genesis1.Sign(chain1.Generate(genesis1.nSequence, "1111"));

    if(genesis1.IsValid())
        genesis1.print();

    legDB->WriteTx(genesis1.GetHash(), genesis1);

    uint256_t hashGenesis1 = LLC::SK256(ssGen1.begin(), ssGen1.end());


    //create the first transaction
    TAO::Ledger::Transaction next1 = TAO::Ledger::Transaction();
    next1.nSequence = 1;

    next1.NextHash(chain1.Generate(next1.nSequence + 1, "1111"));
    next1.hashPrevTx = genesis1.GetHash();
    next1.hashGenesis = hashGenesis1;

    assert(next1.hashGenesis == hashGenesis1);

    //create an object register account
    TAO::Register::Account acct1(105, 100);
    acct1.print();

    printf("\n");

    //create a state register
    uint256_t hashRegister1 = LLC::GetRand256();
    TAO::Register::State state1 = TAO::Register::State((uint8_t)TAO::Register::OBJECT_ACCOUNT, hashRegister1, hashGenesis1);
    state1 << acct1;

    //add the data to the ledger
    next1 << (uint8_t)TAO::Operation::OP_REGISTER << hashRegister1 << state1;
    next1.Sign(chain1.Generate(next1.nSequence, "1111"));
    if(next1.IsValid())
        next1.print();

    legDB->WriteTx(next1.GetHash(), next1);

    TAO::Operation::Execute(next1.vchLedgerData, regDB, legDB, next1.hashGenesis);

    TAO::Register::State stateRead1;
    if(!regDB->ReadState(hashRegister1, stateRead1))
        return error("Failed to read state");


    TAO::Register::Account acct1t;
    stateRead1 >> acct1t;
    acct1t.print();


    //create the first debit transaction
    TAO::Ledger::Transaction next2 = TAO::Ledger::Transaction();
    next2.nSequence = 2;

    next2.NextHash(chain1.Generate(next2.nSequence + 1, "1111"));
    next2.hashPrevTx = genesis1.GetHash();
    next2.hashGenesis = hashGenesis1;

    assert(next2.hashGenesis == hashGenesis1);

    //add the data to the ledger
    uint256_t hashRegRead = hashRegister1;
    next2 << (uint8_t)TAO::Operation::OP_DEBIT << hashRegRead << hashRegister << (uint64_t)(55);
    next2.Sign(chain1.Generate(next2.nSequence, "1111"));
    if(next2.IsValid())
        next2.print();

    legDB->WriteTx(next2.GetHash(), next2);

    TAO::Operation::Execute(next2.vchLedgerData, regDB, legDB, next2.hashGenesis);

    TAO::Register::State stateRead2;
    if(!regDB->ReadState(hashRegister1, stateRead2))
        return error("Failed to read state2");

    stateRead2.print();


    TAO::Register::Account acct2;
    stateRead2 >> acct2;

    acct2.print();



    ////////////////////////////////////////////////// credit test

    //create the first transaction
    TAO::Ledger::Transaction next3 = TAO::Ledger::Transaction();
    next3.nSequence = next.nSequence + 1;

    next3.NextHash(chain.Generate(next.nSequence + 1, "1111"));
    next3.hashPrevTx = genesis.GetHash();
    next3.hashGenesis = hashGenesis;

    assert(next3.hashGenesis == hashGenesis);

    //add the data to the ledger
    next3 << (uint8_t)TAO::Operation::OP_CREDIT << next2.GetHash() << uint256_t(0) << hashRegister << (uint64_t)(55);
    next3.Sign(chain1.Generate(next1.nSequence, "1111"));
    if(next3.IsValid())
        next3.print();

    legDB->WriteTx(next3.GetHash(), next3);

    TAO::Operation::Execute(next3.vchLedgerData, regDB, legDB, next3.hashGenesis);

    if(!regDB->ReadState(hashRegister, stateRead))
        return error("Failed to read state");

    TAO::Register::Account account;
    stateRead >> account;

    account.print();





    //create the first transaction
    TAO::Ledger::Transaction next4 = TAO::Ledger::Transaction();
    next4.nSequence = next3.nSequence + 1;

    next4.NextHash(chain.Generate(next4.nSequence + 1, "1111"));
    next4.hashPrevTx = genesis.GetHash();
    next4.hashGenesis = hashGenesis;

    assert(next3.hashGenesis == hashGenesis);


    uint256_t hashRegister4 = LLC::GetRand256();
    TAO::Register::State state4 = TAO::Register::State((uint8_t)TAO::Register::OBJECT_TOKEN, hashRegister4, hashGenesis);

    TAO::Register::Token token(22, 100); //token with 100 max tokens
    state4 << token;

    //add the data to the ledger
    next4 << (uint8_t)TAO::Operation::OP_REGISTER << hashRegister4 << state4;
    next4.Sign(chain1.Generate(next4.nSequence, "1111"));
    if(next4.IsValid())
        next4.print();

    legDB->WriteTx(next4.GetHash(), next4);

    TAO::Operation::Execute(next4.vchLedgerData, regDB, legDB, next4.hashGenesis);

    TAO::Register::State stateRead4;
    if(!regDB->ReadState(hashRegister4, stateRead4))
        return error("Failed to read state");

    TAO::Register::Token token4;
    stateRead4 >> token4;

    token4.print();




    //create the first transaction
    TAO::Ledger::Transaction next5 = TAO::Ledger::Transaction();
    next5.nSequence = next3.nSequence + 1;

    next5.NextHash(chain.Generate(next5.nSequence + 1, "1111"));
    next5.hashPrevTx = genesis.GetHash();
    next5.hashGenesis = hashGenesis;

    assert(next3.hashGenesis == hashGenesis);


    uint256_t hashRegister5 = LLC::GetRand256();
    TAO::Register::State state5 = TAO::Register::State((uint8_t)TAO::Register::OBJECT_RAW, hashRegister5, hashRegister4);

    std::string strData = "(c) copyright token ART\nTitle: Song\nAlbum: Greatest\n";
    state5 << strData.c_str();

    //add the data to the ledger
    next5 << (uint8_t)TAO::Operation::OP_REGISTER << hashRegister5 << state5;
    next5.Sign(chain1.Generate(next5.nSequence, "1111"));
    if(next5.IsValid())
        next5.print();

    legDB->WriteTx(next5.GetHash(), next5);

    TAO::Operation::Execute(next5.vchLedgerData, regDB, legDB, next5.hashGenesis);

    TAO::Register::State stateRead5;
    if(!regDB->ReadState(hashRegister5, stateRead5))
        return error("Failed to read state");

    char* data;
    stateRead5 >> data;

    printf("%s\n", data);


    //create the first debit transaction
    TAO::Ledger::Transaction next6 = TAO::Ledger::Transaction();
    next6.nSequence = 2;

    next6.NextHash(chain1.Generate(next6.nSequence + 1, "1111"));
    next6.hashPrevTx = genesis1.GetHash();
    next6.hashGenesis = hashGenesis;

    assert(next6.hashGenesis == hashGenesis);

    //create an object register account
    TAO::Register::Account acct6(22, 50);
    acct.print();

    printf("\n");

    //create a state register
    uint256_t hashRegister6 = LLC::GetRand256();
    TAO::Register::State state6 = TAO::Register::State((uint8_t)TAO::Register::OBJECT_ACCOUNT, hashRegister6, next6.hashGenesis);
    state6 << acct6;

    //add the data to the ledger
    next6 << (uint8_t)TAO::Operation::OP_REGISTER << hashRegister6 << state6;

    next6.Sign(chain1.Generate(next6.nSequence, "1111"));
    if(next6.IsValid())
        next6.print();

    legDB->WriteTx(next6.GetHash(), next2);

    TAO::Operation::Execute(next6.vchLedgerData, regDB, legDB, next6.hashGenesis);

    TAO::Register::State stateRead6;
    if(!regDB->ReadState(hashRegister6, stateRead6))
        return error("Failed to read state2");

    TAO::Register::Account acct66;
    stateRead2 >> acct66;

    acct66.print();



    ////////////////////////////////////////////////// credit test

    //create the first transaction
    TAO::Ledger::Transaction next7 = TAO::Ledger::Transaction();
    next7.nSequence = next7.nSequence + 1;

    next7.NextHash(chain.Generate(next.nSequence + 1, "1111"));
    next7.hashPrevTx = genesis.GetHash();
    next7.hashGenesis = hashGenesis1;

    //add the data to the ledger
    next7 << (uint8_t)TAO::Operation::OP_DEBIT << hashRegister1 << hashRegister5 << uint64_t(20);
    next7.Sign(chain1.Generate(next1.nSequence, "1111"));
    if(next7.IsValid())
        next7.print();

    legDB->WriteTx(next7.GetHash(), next7);

    TAO::Operation::Execute(next7.vchLedgerData, regDB, legDB, next7.hashGenesis);


    ////////////////////////////////////////////////// credit test

    //create the first transaction
    TAO::Ledger::Transaction next8 = TAO::Ledger::Transaction();
    next8.nSequence = next.nSequence + 1;

    next8.NextHash(chain.Generate(next.nSequence + 1, "1111"));
    next8.hashPrevTx = genesis.GetHash();
    next8.hashGenesis = hashGenesis;

    assert(next8.hashGenesis == hashGenesis);

    //add the data to the ledger
    next8 << (uint8_t)TAO::Operation::OP_CREDIT << next7.GetHash() << hashRegister6 << hashRegister << (uint64_t)(10);
    next8.Sign(chain1.Generate(next1.nSequence, "1111"));
    if(next8.IsValid())
        next8.print();

    legDB->WriteTx(next8.GetHash(), next3);

    TAO::Operation::Execute(next8.vchLedgerData, regDB, legDB, next8.hashGenesis);

    TAO::Register::State stateRead8;
    if(!regDB->ReadState(hashRegister, stateRead8))
        return error("Failed to read state");

    TAO::Register::Account account8;
    stateRead8 >> account8;

    account8.print();


    return 0;
}
