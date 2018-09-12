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





    TAO::Register::State test = TAO::Register::State();

    uint256_t rand = LLC::GetRand256();

    test << rand;

    test.print();

    uint256_t newRand;
    test >> newRand;

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
    next2 << (uint8_t)TAO::Operation::OP_DEBIT << hashRegister1 << hashRegister << (uint64_t)(5);
    next2.Sign(chain1.Generate(next2.nSequence, "1111"));
    if(next2.IsValid())
        next2.print();

    legDB->WriteTx(next2.GetHash(), next2);

    TAO::Operation::Execute(next2.vchLedgerData, regDB, legDB, next2.hashGenesis);

    TAO::Register::State stateRead2;
    if(!regDB->ReadState(hashRegister1, stateRead2))
        return error("Failed to read state");

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
    next3 << (uint8_t)TAO::Operation::OP_CREDIT << next2.GetHash() << uint256_t(0) << hashRegister << (uint64_t)(5);
    next3.Sign(chain1.Generate(next1.nSequence, "1111"));
    if(next3.IsValid())
        next3.print();

    if(!regDB->ReadState(hashRegister, stateRead))
        return error("Failed to read state");

    TAO::Register::Account account;
    stateRead >> account;

    account.print();
    

    //mock test some basic operations running through the ledger data


/*
        case TAO::Operation::OP_CREDIT:
        {
            uint512_t hashTx;    //read the txid and then read the OP_DEBIT to ensure that hashTo matches
            uint256_t hashProof; //the register address that proves ownership -> equates to that register having hash owner of this genesis. Also equates to a token being used to claim ownership of the HashTo register.
            //1. hashProof -> OP_READ register -> then get identifier -> OP_READ register address of identifier to get token total if register is a token -> divide that by total nBalance in hashProof register. Verify this to nAmount to ensure token claim is true
            //This above is if hashProof resolves to an Account register that is not the same identifier of OP_DEBIT
            //If same token identifier -> OP_READ register of hashProof -> check that this sigchain owns register, if so, credit nAmount
            uint256_t hashCreditTo;    //the account to send amount to (will usually be the same as hashTo in debit. If so -> skip all token stuff)
            uint64_t  nAmount;   //the amount to be credited

            ssRead >> hashTx >> hashProof >> nAmount;

            ////////////////////////// hashTx checks
            TAO::Ledger::Transaction tx;
            if(!txDB.ReadTx(hashTx, tx)) //fail

            CDataStream ssData(tx.vchLedgerData, SER_NETWORK, LLD::DATABASE_VERSION);

            uint8_t OP;
            ssData >> OP;

            //use templates for these on the operations layers
            if(OP != TAO::Operation::OP_DEBIT) //fail

            //todo deserialize these easier
            uint256_t hashFrom;
            ssData >> hashFrom;

            uint256_t hashTo;
            ssData >> hashTo;

            uint64_t nAmount;
            ssData >> nAmount;

            uint32_t nExpires;
            ssData >> nExpires;

            if(UnifiedTimestamp() > tx.nTimestamp + nExpires) //fail if debit is expired

            TAO::Register::State stateFrom;
            if(!regDB.ReadState(hashFrom, stateFrom)) //fail

            CDataStream ssState(stateFrom.GetState(), SER_NETWORK, LLD::DATABASE_VERSION);

            //repetitive deserializing of objects. use templates for this on register layer
            if(testState.nType != TAO::Register::OBJECT_ACCOUNT) //fail for now... handle other types later

            TAO::Register::Account acctFrom;
            ssState >> acctFrom;

            TAO::Register State stateTo;
            if(!regDB.ReadState(hashTo, stateTo))

            CDataStream ssState(stateTo.GetState(), SER_NETWORK, LLD::DATABASE_VERSION);
            if(stateTo.nType == TAO::Register::OBJECT_RAW) //this states that debit was to a raw register. This is data to the logical layer.
            {
                uint256_t hashOwner = stateTo.hashOwner;

                TAO::Register::State stateOwner;
                if(!regDB.ReadState(hashOwner, stateOWner)) //fail

                if(stateOwner.nType != TAO::Register::OBJECT_TOKEN) //fail - disable anything not owned by token account for now

                CDataStream ssToken(stateOwner.GetState(), SER_NETWORK, LLD::DATABASE_VERSION);

                TAO::Register::Token token;
                ssToken >> token;

                TAO::Register::State stateProof;
                if(!regDB.ReadState(hashProof, stateProof)) //fail

                CDataStream ssAccount(stateProof.GetState(), SER_NETWORK, LLD::DATABASE_VERSION);
                if(stateProof.nType != TAO::Register::OBJECT_ACCOUNT) //fail

                TAO::Register::Account acctProof;
                ssAccount >> acctProof;

                if(stateProof.hashOwner != Transaction.hashGenesis) //fail - you have to own the account proof you are using

                if(acctProof.nIdentifier != token.nIdentifier) //fail - your account proof is not showing valid token balance

                int nTotal = token.nMaxSupply * nAmount / acctProof.nBalance; //the total you can claim is based on max supply of token vs your token balance

                TAO::Register::State stateDebitTo;
                if(!regDB.ReadState(hashDebitTo, stateDebitTo)) //fail

                if(stateDebitTo.nType != TAO::Register::OBJECT_ACCOUNT) //fail

                TAO::Register::Account accountTo;
                CDataStream ssAccountTo(stateDebitTo.GetState(), SER_NETWORK, LLD::DATABASE_VERSION);

                ssAccountTo >> accountTo;
                accountTo.nBalance += nTotal;

                if(stateDebitTo.hashOwner != Transaction.hashGenesis) //fail

                if(hashProof != stateTo.hashOwner) //fail

                stateDebitTo.SetState(accountTo.Serialize()); //need serialize in and out methods. CDataStream is too clunky here
                regDB.WriteState(hashDebitTo, stateDebitTo); //only you can write this state since you are the owner of this register. handle in OP_WRITE

                //get the amounts set and credit your claimed hashTo for credit with native hashFrom tokens


            }
            else if(stateTo.nType == TAO::Register::OBJECT_ACCOUNT)
            {
                //this is a normal credit operation. prove that you own this account and write state.
                ssState >> accountTo;

                accountTo.nBalance += nAmount;

                if(stateTo.hashOwner != Transaction.hashGenesis) //fail

                if(hashProof != stateTo.hashOwner) //fail

                stateTo.SetState(accountTo.Serialize()); //need serialize in and out methods. CDataStream is too clunky here
                regDB.WriteState(hashTo, stateTo); //only you can write this state since you are the owner of this register. handle in OP_WRITE operation
            }
        }

        */



    //execute the transaction operations

    //need a bytes switch for object registers from byte code OBJ_ACCOUNT, OBJ_TOKEN, OBJ_ESCROW

    return 0;
}
