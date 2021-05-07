/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include "util.h"

#include <LLC/include/random.h>

#include <unit/catch2/catch.hpp>

#include <LLD/include/global.h>

#include <TAO/Ledger/types/transaction.h>
#include <TAO/Ledger/include/chainstate.h>

#include <TAO/Operation/include/enum.h>
#include <TAO/Operation/include/execute.h>

#include <TAO/Register/include/create.h>

#include <TAO/API/names/types/names.h>

#include <Util/include/math.h>

TEST_CASE( "Test Tokens API - create token", "[tokens]")
{
    /* Declare variables shared across test cases */
    json::json params;
    json::json ret;
    json::json result;
    json::json error;

    /* Generate random token name */
    std::string strToken = "TOKEN" + std::to_string(LLC::GetRand());

    /* Ensure user is created and logged in for testing */
    InitializeUser(USERNAME1, PASSWORD, PIN, GENESIS1, SESSION1);

    /* tokens/create/token fail with missing pin (only relevant for multiuser)*/
    if(config::fMultiuser.load())
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["session"]  = SESSION1;
        params["supply"]   = "100";
        params["decimals"] = "2";

        /* Invoke the API */
        ret = APICall("tokens/create/token", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -129);
    }

    /* tokens/create/token fail with missing session (only relevant for multiuser)*/
    if(config::fMultiuser.load())
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"] = PIN;
        params["supply"]   = "100";
        params["decimals"] = "2";

        /* Invoke the API */
        ret = APICall("tokens/create/token", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -12);
    }

    /* tokens/create/token fail with missing supply */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"] = PIN;
        params["decimals"] = "2";
        params["session"] = SESSION1;
        params["name"] = strToken;

        /* Invoke the API */
        ret = APICall("tokens/create/token", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -119);
    }

    /* tokens/create/token success */
    {

        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"] = PIN;
        params["session"] = SESSION1;
        params["name"] = strToken;
        params["supply"] = "10000";
        params["decimals"] = "2";

        /* Invoke the API */
        ret = APICall("tokens/create/token", params);

        /* Check that the result is as we expect it to be */
        REQUIRE(ret.find("result") != ret.end());
        result = ret["result"];

        REQUIRE(result.find("txid") != result.end());
        REQUIRE(result.find("address") != result.end());
    }
}


TEST_CASE( "Test Tokens API - debit token", "[tokens]")
{
    /* Declare variables shared across test cases */
    json::json params;
    json::json ret;
    json::json result;
    json::json error;

    /* Generate random token name */
    std::string strToken = "TOKEN" +std::to_string(LLC::GetRand());

    /* Ensure user is created and logged in for testing */
    InitializeUser(USERNAME1, PASSWORD, PIN, GENESIS1, SESSION1);

    /* Create Token  */
    {

        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"] = PIN;
        params["session"] = SESSION1;
        params["name"] = strToken;
        params["supply"] = "10000";
        params["decimals"] = "2";

        /* Invoke the API */
        ret = APICall("tokens/create/token", params);

        /* Check that the result is as we expect it to be */
        REQUIRE(ret.find("result") != ret.end());
        result = ret["result"];

        REQUIRE(result.find("txid") != result.end());
        REQUIRE(result.find("address") != result.end());
    }

    /* Test fail with missing amount */
     {
        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"] = PIN;
        params["session"] = SESSION1;
        params["name"] = strToken;

        /* Invoke the API */
        ret = APICall("tokens/debit/token", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -46);
     }

     /* Test fail with missing name / address*/
     {
        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"] = PIN;
        params["session"] = SESSION1;
        params["amount"] = "100";
        params["address_to"] = TAO::Register::Address(TAO::Register::Address::ACCOUNT).ToString();

        /* Invoke the API */
        ret = APICall("tokens/debit/token", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -33);
     }

    /* Test fail with invalid name */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"] = PIN;
        params["session"] = SESSION1;
        params["amount"] = "100";
        params["name_to"] = "random";
        params["name"] = "random";

        /* Invoke the API */
        ret = APICall("tokens/debit/token", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -101);
     }

    /* Test fail with invalid address */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"] = PIN;
        params["session"] = SESSION1;
        params["amount"] = "100";
        params["address"]    = "invalid";

        /* Invoke the API */
        ret = APICall("tokens/debit/token", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -165);
     }


     /* Test fail with account not found. */
     {
         /* Build the parameters to pass to the API */
         params.clear();
         params["pin"] = PIN;
         params["session"] = SESSION1;
         params["amount"] = "100";
         params["address"] = TAO::Register::Address(TAO::Register::Address::TOKEN).ToString();

         /* Invoke the API */
         ret = APICall("tokens/debit/token", params);

         /* Check response is an error and validate error code */
         REQUIRE(ret.find("error") != ret.end());
         REQUIRE(ret["error"]["code"].get<int32_t>() == -13);
      }


    /* Test fail with missing name_to / address_to */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"] = PIN;
        params["session"] = SESSION1;
        params["name"] = strToken;
        params["amount"] = "100";

        /* Invoke the API */
        ret = APICall("tokens/debit/token", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -64);
     }

    /* Test fail with invalid name_to */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"] = PIN;
        params["session"] = SESSION1;
        params["name"] = strToken;
        params["amount"] = "100";
        params["name_to"] = "random";

        /* Invoke the API */
        ret = APICall("tokens/debit/token", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -101);
     }

    /* Create an account to send to */
    std::string strAccountAddress;
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"] = PIN;
        params["session"] = SESSION1;
        params["name"] = "main";
        params["token_name"] = strToken;

        /* Invoke the API */
        ret = APICall("tokens/create/account", params);

        /* Check that the result is as we expect it to be */
        REQUIRE(ret.find("result") != ret.end());
        result = ret["result"];

        REQUIRE(result.find("txid") != result.end());
        REQUIRE(result.find("address") != result.end());

        strAccountAddress = result["address"].get<std::string>();
    }


    /* Test failure case by address_to that doesn't resolve*/
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"] = PIN;
        params["session"] = SESSION1;
        params["name"] = strToken;
        params["amount"] = "100";
        params["address_to"] = TAO::Register::Address(TAO::Register::Address::TOKEN).ToString();

        /* Invoke the API */
        ret = APICall("tokens/debit/token", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -209);
     }


    /* Test success case by name_to */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"] = PIN;
        params["session"] = SESSION1;
        params["name"] = strToken;
        params["amount"] = "100";
        params["name_to"] = "main";

        /* Invoke the API */
        ret = APICall("tokens/debit/token", params);

        REQUIRE(result.find("txid") != result.end());
     }

    /* Test success case by address_to */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"] = PIN;
        params["session"] = SESSION1;
        params["name"] = strToken;
        params["amount"] = "100";
        params["address_to"] = strAccountAddress;

        /* Invoke the API */
        ret = APICall("tokens/debit/token", params);

        REQUIRE(result.find("txid") != result.end());
     }

}


TEST_CASE( "Test Tokens API - credit token", "[tokens]")
{
    /* Declare variables shared across test cases */
    json::json params;
    json::json ret;
    json::json result;
    json::json error;

    /* Generate random token name */
    std::string strToken = "TOKEN" +std::to_string(LLC::GetRand());

    std::string strTXID;

    /* Ensure user is created and logged in for testing */
    InitializeUser(USERNAME1, PASSWORD, PIN, GENESIS1, SESSION1);

    /* Create Token to debit and then credit*/
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"] = PIN;
        params["session"] = SESSION1;
        params["name"] = strToken;
        params["supply"] = "10000";
        params["decimals"] = "2";

        /* Invoke the API */
        ret = APICall("tokens/create/token", params);

        /* Check that the result is as we expect it to be */
        REQUIRE(ret.find("result") != ret.end());
        result = ret["result"];

        REQUIRE(result.find("txid") != result.end());
        REQUIRE(result.find("address") != result.end());
    }

    /* Create debit transaction to random address (we will be crediting it back to ourselves so it doesn't matter where it goes) */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"] = PIN;
        params["session"] = SESSION1;
        params["name"] = strToken;
        params["amount"] = "100";
        params["address_to"] = TAO::Register::Address(TAO::Register::Address::ACCOUNT).ToString();

        /* Invoke the API */
        ret = APICall("tokens/debit/token", params);

        REQUIRE(result.find("txid") != result.end());
        strTXID = result["txid"].get<std::string>();
     }

    /* Test fail with missing txid */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"] = PIN;
        params["session"] = SESSION1;

        /* Invoke the API */
        ret = APICall("tokens/credit/token", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -50);
    }

    /* Test fail with invalid txid */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"] = PIN;
        params["session"] = SESSION1;
        params["txid"]    = RandomTxid().ToString();

        /* Invoke the API */
        ret = APICall("tokens/credit/token", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -40);
     }

     /* Test success case */
     {
        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"] = PIN;
        params["session"] = SESSION1;
        params["txid"] = strTXID;

        /* Invoke the API */
        ret = APICall("tokens/credit/token", params);

        REQUIRE(result.find("txid") != result.end());
     }

}


TEST_CASE( "Test Tokens API - get token", "[tokens]")
{
    /* Declare variables shared across test cases */
    json::json params;
    json::json ret;
    json::json result;
    json::json error;

    /* Generate random token name */
    std::string strToken = "TOKEN" +std::to_string(LLC::GetRand());
    std::string strTokenAddress;
    std::string strTXID;

    /* Ensure user is created and logged in for testing */
    InitializeUser(USERNAME1, PASSWORD, PIN, GENESIS1, SESSION1);

    /* Create Token to retrieve */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"] = PIN;
        params["session"] = SESSION1;
        params["name"] = strToken;
        params["supply"] = "10000";
        params["decimals"] = "2";

        /* Invoke the API */
        ret = APICall("tokens/create/token", params);

        /* Check that the result is as we expect it to be */
        REQUIRE(ret.find("result") != ret.end());
        result = ret["result"];

        REQUIRE(result.find("txid") != result.end());
        REQUIRE(result.find("address") != result.end());
        strTokenAddress = result["address"].get<std::string>();
    }

    /* Test fail with missing name / address */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"] = PIN;
        params["session"] = SESSION1;

        /* Invoke the API */
        ret = APICall("tokens/get/token", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -33);
    }

    /* Test fail with invalid name */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"] = PIN;
        params["session"] = SESSION1;
        params["name"] = "asdfsdfsdf";

        /* Invoke the API */
        ret = APICall("tokens/get/token", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -101);
    }

    /* Test fail with invalid address */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"] = PIN;
        params["session"] = SESSION1;
        params["address"] = TAO::Register::Address(TAO::Register::Address::TOKEN).ToString();

        /* Invoke the API */
        ret = APICall("tokens/get/token", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -13);
    }

    /* Test successful get by name  */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"] = PIN;
        params["session"] = SESSION1;
        params["name"] = strToken;

        /* Invoke the API */
        ret = APICall("tokens/get/token", params);

        /* Check that the result is as we expect it to be */
        REQUIRE(ret.find("result") != ret.end());
        result = ret["result"];

        REQUIRE(result.find("owner") != result.end());
        REQUIRE(result.find("created") != result.end());
        REQUIRE(result.find("modified") != result.end());
        REQUIRE(result.find("name") != result.end());
        REQUIRE(result.find("address") != result.end());
        REQUIRE(result.find("balance") != result.end());
        REQUIRE(result.find("maxsupply") != result.end());
        REQUIRE(result.find("currentsupply") != result.end());
        REQUIRE(result.find("decimals") != result.end());
    }

    /* Test successful get by address  */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"] = PIN;
        params["session"] = SESSION1;
        params["address"] = strTokenAddress;

        /* Invoke the API */
        ret = APICall("tokens/get/token", params);

        /* Check that the result is as we expect it to be */
        REQUIRE(ret.find("result") != ret.end());
        result = ret["result"];

        REQUIRE(result.find("owner") != result.end());
        REQUIRE(result.find("created") != result.end());
        REQUIRE(result.find("modified") != result.end());
        REQUIRE(result.find("name") != result.end());
        REQUIRE(result.find("address") != result.end());
        REQUIRE(result.find("balance") != result.end());
        REQUIRE(result.find("maxsupply") != result.end());
        REQUIRE(result.find("currentsupply") != result.end());
        REQUIRE(result.find("decimals") != result.end());
    }
}

TEST_CASE( "Test Tokens API - create account", "[tokens]")
{
    /* Declare variables shared across test cases */
    json::json params;
    json::json ret;
    json::json result;
    json::json error;

    /* Generate random token name */
    std::string strToken = "TOKEN" +std::to_string(LLC::GetRand());
    std::string strTokenAddress;

    /* Ensure user is created and logged in for testing */
    InitializeUser(USERNAME1, PASSWORD, PIN, GENESIS1, SESSION1);

    /* create token to create account for  */
    {

        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"] = PIN;
        params["session"] = SESSION1;
        params["name"] = strToken;
        params["supply"] = "10000";
        params["decimals"] = "2";

        /* Invoke the API */
        ret = APICall("tokens/create/token", params);

        /* Check that the result is as we expect it to be */
        REQUIRE(ret.find("result") != ret.end());
        result = ret["result"];

        REQUIRE(result.find("txid") != result.end());
        REQUIRE(result.find("address") != result.end());
        strTokenAddress = result["address"].get<std::string>();
    }

    /* tokens/create/account fail with missing pin (only relevant for multiuser)*/
    if(config::fMultiuser.load())
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["session"] = SESSION1;

        /* Invoke the API */
        ret = APICall("tokens/create/account", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -129);
    }

    /* tokens/create/account fail with missing session (only relevant for multiuser)*/
    if(config::fMultiuser.load())
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"] = PIN;

        /* Invoke the API */
        ret = APICall("tokens/create/account", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -12);
    }

    /* tokens/create/account by token name success */
    {

        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"] = PIN;
        params["session"] = SESSION1;
        params["token_name"] = strToken;

        /* Invoke the API */
        ret = APICall("tokens/create/account", params);

        /* Check that the result is as we expect it to be */
        REQUIRE(ret.find("result") != ret.end());
        result = ret["result"];

        REQUIRE(result.find("txid") != result.end());
        REQUIRE(result.find("address") != result.end());
    }

    /* tokens/create/account by token address success */
    {

        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"] = PIN;
        params["session"] = SESSION1;
        params["token"] = strTokenAddress;

        /* Invoke the API */
        ret = APICall("tokens/create/account", params);

        /* Check that the result is as we expect it to be */
        REQUIRE(ret.find("result") != ret.end());
        result = ret["result"];

        REQUIRE(result.find("txid") != result.end());
        REQUIRE(result.find("address") != result.end());
    }

    REQUIRE(GenerateBlock());
}


TEST_CASE( "Test Tokens API - debit account", "[tokens]")
{
    /* Declare variables shared across test cases */
    json::json params;
    json::json ret;
    json::json result;
    json::json error;

    /* Generate random token name */
    std::string strToken = "TOKEN" +std::to_string(LLC::GetRand());
    TAO::Register::Address hashToken = TAO::Register::Address(TAO::Register::Address::TOKEN);
    std::string strAccount = "ACCOUNT" +std::to_string(LLC::GetRand());
    TAO::Register::Address hashAccount = TAO::Register::Address(TAO::Register::Address::ACCOUNT);

    uint512_t hashDebitTx;

    /* Ensure user is created and logged in for testing */
    InitializeUser(USERNAME1, PASSWORD, PIN, GENESIS1, SESSION1);

    /* create token to create account for  */
    {
        //create the transaction object
        TAO::Ledger::Transaction tx;
        tx.hashGenesis = GENESIS1;
        tx.nSequence   = 0;
        tx.nTimestamp  = runtime::timestamp();

        //create object
        TAO::Register::Object token = TAO::Register::CreateToken(hashToken, 1000000, 2);

        //payload
        tx[0] << uint8_t(TAO::Operation::OP::CREATE) << hashToken << uint8_t(TAO::Register::REGISTER::OBJECT) << token.GetState();

        //create name
        tx[1] = TAO::API::Names::CreateName(GENESIS1, strToken, "", hashToken);

        //generate the prestates and poststates
        REQUIRE(tx.Build());

        //verify the prestates and poststates
        REQUIRE(tx.Verify());

        //commit to disk
        REQUIRE(Execute(tx[0], TAO::Ledger::FLAGS::BLOCK));

        //commit to disk
        REQUIRE(Execute(tx[1], TAO::Ledger::FLAGS::BLOCK));

        //check that registers were created properly
        TAO::Register::Address hashName = TAO::Register::Address(strToken, GENESIS1, TAO::Register::Address::NAME);
        REQUIRE(LLD::Register->HasState(hashName));
    }

    /* create account to debit */
    {
        //create the transaction object
        TAO::Ledger::Transaction tx;
        tx.hashGenesis = GENESIS1;
        tx.nSequence   = 1;
        tx.nTimestamp  = runtime::timestamp();

        //create object
        TAO::Register::Object account = TAO::Register::CreateAccount(hashToken);

        //payload
        tx[0] << uint8_t(TAO::Operation::OP::CREATE) << hashAccount << uint8_t(TAO::Register::REGISTER::OBJECT) << account.GetState();

        //create name
        tx[1] = TAO::API::Names::CreateName(GENESIS1, strAccount, "", hashAccount);

        //generate the prestates and poststates
        REQUIRE(tx.Build());

        //verify the prestates and poststates
        REQUIRE(tx.Verify());

        //commit to disk
        REQUIRE(Execute(tx[0], TAO::Ledger::FLAGS::BLOCK));

        //commit to disk
        REQUIRE(Execute(tx[1], TAO::Ledger::FLAGS::BLOCK));

        //check that registers were created properly
        TAO::Register::Address hashName = TAO::Register::Address(strAccount, GENESIS1, TAO::Register::Address::NAME);
        REQUIRE(LLD::Register->HasState(hashName));
    }



    /* Test fail with missing amount */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"] = PIN;
        params["session"] = SESSION1;
        params["address"] = hashAccount.ToString();

        /* Invoke the API */
        ret = APICall("tokens/debit/account", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -46);
    }

    /* Test fail with missing name / address*/
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"] = PIN;
        params["session"] = SESSION1;
        params["amount"] = "100";
        params["address_to"] = TAO::Register::Address(TAO::Register::Address::ACCOUNT).ToString();

        /* Invoke the API */
        ret = APICall("tokens/debit/account", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -33);
    }

    /* Test fail with invalid name */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"] = PIN;
        params["session"] = SESSION1;
        params["amount"] = "100";
        params["name_to"] = "random";
        params["name"] = "random";

        /* Invoke the API */
        ret = APICall("tokens/debit/account", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -101);
    }

    /* Test fail with invalid address */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"] = PIN;
        params["session"] = SESSION1;
        params["amount"] = "100";
        params["address_to"] = TAO::Register::Address(TAO::Register::Address::ACCOUNT).ToString();
        params["address"] = TAO::Register::Address(TAO::Register::Address::ACCOUNT).ToString();

        /* Invoke the API */
        ret = APICall("tokens/debit/account", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -13);
    }

    /* Debit the token and credit the account so that we have some funds to debit */
    {
        TAO::Ledger::Transaction tx;
        tx.hashGenesis = GENESIS1;
        tx.nSequence   = 0;
        tx.nTimestamp  = runtime::timestamp();


        //payload
        tx[0] << uint8_t(TAO::Operation::OP::DEBIT) << hashToken << hashAccount << uint64_t(100000) << uint64_t(0);

        //generate the prestates and poststates
        REQUIRE(tx.Build());

        //verify the prestates and poststates
        REQUIRE(tx.Verify());

        //write transaction
        REQUIRE(LLD::Ledger->WriteTx(tx.GetHash(), tx));
        REQUIRE(LLD::Ledger->IndexBlock(tx.GetHash(), TAO::Ledger::ChainState::Genesis()));

        //commit to disk
        REQUIRE(Execute(tx[0], TAO::Ledger::FLAGS::BLOCK));

        hashDebitTx = tx.GetHash();
    }

    {
        TAO::Ledger::Transaction tx;
        tx.hashGenesis = GENESIS1;
        tx.nSequence   = 1;
        tx.nTimestamp  = runtime::timestamp();


        //payload
        tx[0] << uint8_t(TAO::Operation::OP::CREDIT) << hashDebitTx << uint32_t(0) << hashAccount << hashToken << uint64_t(100000);


        //generate the prestates and poststates
        REQUIRE(tx.Build());

        //verify the prestates and poststates
        REQUIRE(tx.Verify());

        //write transaction
        REQUIRE(LLD::Ledger->WriteTx(tx.GetHash(), tx));
        REQUIRE(LLD::Ledger->IndexBlock(tx.GetHash(), TAO::Ledger::ChainState::Genesis()));

        //commit to disk
        REQUIRE(Execute(tx[0], TAO::Ledger::FLAGS::BLOCK));


    }


    /* Test fail with missing name_to / address_to */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"] = PIN;
        params["session"] = SESSION1;
        params["address"] = hashAccount.ToString();
        params["amount"] = "10";

        /* Invoke the API */
        ret = APICall("tokens/debit/account", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -64);
    }

    /* Test fail with invalid name_to */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"] = PIN;
        params["session"] = SESSION1;
        params["address"] = hashAccount.ToString();
        params["amount"] = "10";
        params["name_to"] = "random";

        /* Invoke the API */
        ret = APICall("tokens/debit/account", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -101);
    }

    /* Test fail with insufficient funds */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"] = PIN;
        params["session"] = SESSION1;
        params["address"] = hashAccount.ToString();
        params["amount"] = "10000000";
        params["address_to"] = hashToken.ToString();

        /* Invoke the API */
        ret = APICall("tokens/debit/account", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -69);
    }

    /* Test fail multiple recipients with invalid JSON */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"] = PIN;
        params["session"] = SESSION1;
        params["address"] = hashAccount.ToString();
        params["recipients"] = "";

        /* Invoke the API */
        ret = APICall("tokens/debit/account", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -216);
    }

    /* Test fail multiple recipients with empty recipients */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"] = PIN;
        params["session"] = SESSION1;
        params["address"] = hashAccount.ToString();
        params["recipients"] = json::json::array();

        /* Invoke the API */
        ret = APICall("tokens/debit/account", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -217);
    }

    /* Test fail multiple recipients with too many recipients */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"] = PIN;
        params["session"] = SESSION1;
        params["address"] = hashAccount.ToString();

        /* create json array with 100 recipients */
        json::json jsonRecipients = json::json::array();

        for(int i=0; i<100; i++)
        {
            json::json jsonRecipient;
            jsonRecipient["amount"] = "1";
            jsonRecipient["name_to"] = strToken;
            jsonRecipients.push_back(jsonRecipient);
        }

        params["recipients"] = jsonRecipients;

        /* Invoke the API */
        ret = APICall("tokens/debit/account", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -215);
    }

    /* Test fail multiple recipients with missing amount */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"] = PIN;
        params["session"] = SESSION1;
        params["address"] = hashAccount.ToString();

        /* create json array with 10 recipients */
        json::json jsonRecipients = json::json::array();

        for(int i=0; i<10; i++)
        {
            json::json jsonRecipient;
            jsonRecipient["name_to"] = strToken;
            jsonRecipients.push_back(jsonRecipient);
        }

        params["recipients"] = jsonRecipients;

        /* Invoke the API */
        ret = APICall("tokens/debit/account", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -46);
    }

    /* Test fail multiple recipients with missing name_to / address_to */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"] = PIN;
        params["session"] = SESSION1;
        params["address"] = hashAccount.ToString();

        /* create json array with 10 recipients */
        json::json jsonRecipients = json::json::array();

        for(int i=0; i<10; i++)
        {
            json::json jsonRecipient;
            jsonRecipient["amount"] = "1";
            jsonRecipients.push_back(jsonRecipient);
        }

        params["recipients"] = jsonRecipients;

        /* Invoke the API */
        ret = APICall("tokens/debit/account", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -64);
    }

    /* Test fail multiple recipients with insufficient funds */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"] = PIN;
        params["session"] = SESSION1;
        params["address"] = hashAccount.ToString();

        /* create json array with 50 recipients */
        json::json jsonRecipients = json::json::array();

        for(int i=0; i<50; i++)
        {
            json::json jsonRecipient;
            jsonRecipient["amount"] = "20000";
            jsonRecipient["address_to"] = hashToken.ToString();
            jsonRecipients.push_back(jsonRecipient);
        }

        params["recipients"] = jsonRecipients;

        /* Invoke the API */
        ret = APICall("tokens/debit/account", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -69);
    }


    /* Test failure case by invalid type for name_to */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"] = PIN;
        params["session"] = SESSION1;
        params["name"]   = strAccount;
        params["amount"] = "100";
        params["name_to"] = strToken;

        /* Invoke the API */
        ret = APICall("tokens/debit/token", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -49);
     }

     /****************** SUCCESS CASES *****************/

    /* Test success case by name_to */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"] = PIN;
        params["session"] = SESSION1;
        params["name"]   = strAccount;
        params["amount"] = "100";
        params["name_to"] = strToken;

        /* Invoke the API */
        ret = APICall("tokens/debit/account", params);

        REQUIRE(ret.find("result") != ret.end());
        result = ret["result"];
        REQUIRE(result.find("txid") != result.end());
     }


    /* Test success case by address_to */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"] = PIN;
        params["session"] = SESSION1;
        params["address"] = hashAccount.ToString();
        params["amount"] = "100";
        params["address_to"] = hashToken.ToString();

        /* Invoke the API */
        ret = APICall("tokens/debit/account", params);
        REQUIRE(ret.find("result") != ret.end());
        result = ret["result"];
        REQUIRE(result.find("txid") != result.end());

    }

    /* Test success case with multiple recipients */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"] = PIN;
        params["session"] = SESSION1;
        params["address"] = hashAccount.ToString();

        /* create json array with 50 recipients */
        json::json jsonRecipients = json::json::array();
        for(int i=0; i<50; i++)
        {
            json::json jsonRecipient;
            jsonRecipient["amount"] = (double)(i+1)/10.0; // vary the amount in each contract
            jsonRecipient["address_to"] = hashToken.ToString();
            jsonRecipients.push_back(jsonRecipient);
        }

        params["recipients"] = jsonRecipients;

        /* Invoke the API */
        ret = APICall("tokens/debit/account", params);

        REQUIRE(ret.find("result") != ret.end());
        result = ret["result"];
        REQUIRE(result.find("txid") != result.end());
    }
}


TEST_CASE( "Test Tokens API - debit any", "[tokens]")
{
    /* Declare variables shared across test cases */
    json::json params;
    json::json ret;
    json::json result;
    json::json error;

    //track our accounts
    std::string strToken1   = "1token";
    std::string strAny1     = "1any";
    std::string strAccount1 = "1account1";
    std::string strAccount2 = "1account2";

    std::string strToken2   = "2token";
    std::string strAny2     = "2any";
    std::string strAccount3 = "2account1";
    std::string strAccount4 = "2account2";

    std::string strToken3   = "3token";
    std::string strAny3     = "3any";
    std::string strAccount5 = "3account1";
    std::string strAccount6 = "3account2";

    const uint8_t nDigits1  = 4;
    const uint8_t nDigits2  = 6;
    const uint8_t nDigits3  = 8;

    const uint64_t nFigures1 = math::pow(10, nDigits1);
    const uint64_t nFigures2 = math::pow(10, nDigits2);
    const uint64_t nFigures3 = math::pow(10, nDigits3);

    /* Ensure user is created and logged in for testing */
    InitializeUser(USERNAME1, PASSWORD, PIN, GENESIS1, SESSION1);


    /* Create a new token. */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"]      = PIN;
        params["session"]  = SESSION1;
        params["name"]     = strToken1;
        params["supply"]   = "1000000";
        params["decimals"] = nDigits1;

        /* Invoke the API */
        ret = APICall("tokens/create/token", params);

        REQUIRE(ret.find("result") != ret.end());
        result = ret["result"];
        REQUIRE(result.find("txid") != result.end());
    }


    /* Create a new token. */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"]      = PIN;
        params["session"]  = SESSION1;
        params["name"]     = strToken2;
        params["supply"]   = "2000000";
        params["decimals"] = nDigits2;

        /* Invoke the API */
        ret = APICall("tokens/create/token", params);

        REQUIRE(ret.find("result") != ret.end());
        result = ret["result"];
        REQUIRE(result.find("txid") != result.end());
    }


    /* Create a new token. */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"]      = PIN;
        params["session"]  = SESSION1;
        params["name"]     = strToken3;
        params["supply"]   = "3000000";
        params["decimals"] = nDigits3;

        /* Invoke the API */
        ret = APICall("tokens/create/token", params);

        REQUIRE(ret.find("result") != ret.end());
        result = ret["result"];
        REQUIRE(result.find("txid") != result.end());
    }


    /* Create a new account. */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"]        = PIN;
        params["session"]    = SESSION1;
        params["name"]       = strAccount1;
        params["token_name"] = strToken1;

        /* Invoke the API */
        ret = APICall("tokens/create/account", params);

        REQUIRE(ret.find("result") != ret.end());
        result = ret["result"];
        REQUIRE(result.find("txid") != result.end());
    }


    /* Create a new account. */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"]        = PIN;
        params["session"]    = SESSION1;
        params["name"]       = strAccount2;
        params["token_name"] = strToken1;

        /* Invoke the API */
        ret = APICall("tokens/create/account", params);

        REQUIRE(ret.find("result") != ret.end());
        result = ret["result"];
        REQUIRE(result.find("txid") != result.end());
    }


    /* Create a new account. */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"]        = PIN;
        params["session"]    = SESSION1;
        params["name"]       = strAccount3;
        params["token_name"] = strToken2;

        /* Invoke the API */
        ret = APICall("tokens/create/account", params);

        REQUIRE(ret.find("result") != ret.end());
        result = ret["result"];
        REQUIRE(result.find("txid") != result.end());
    }


    /* Create a new account. */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"]        = PIN;
        params["session"]    = SESSION1;
        params["name"]       = strAccount4;
        params["token_name"] = strToken2;

        /* Invoke the API */
        ret = APICall("tokens/create/account", params);

        REQUIRE(ret.find("result") != ret.end());
        result = ret["result"];
        REQUIRE(result.find("txid") != result.end());
    }


    /* Create a new account. */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"]        = PIN;
        params["session"]    = SESSION1;
        params["name"]       = strAccount5;
        params["token_name"] = strToken3;

        /* Invoke the API */
        ret = APICall("tokens/create/account", params);

        REQUIRE(ret.find("result") != ret.end());
        result = ret["result"];
        REQUIRE(result.find("txid") != result.end());
    }


    /* Create a new account. */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"]        = PIN;
        params["session"]    = SESSION1;
        params["name"]       = strAccount6;
        params["token_name"] = strToken3;

        /* Invoke the API */
        ret = APICall("tokens/create/account", params);

        REQUIRE(ret.find("result") != ret.end());
        result = ret["result"];
        REQUIRE(result.find("txid") != result.end());
    }


    //build a block now
    REQUIRE(GenerateBlock());


    //lets check our accounts now
    {
        double dBalance = 0.0;

        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"]        = PIN;
        params["session"]    = SESSION1;
        params["name"]       = strAccount1;

        /* Invoke the API */
        ret = APICall("tokens/get/account", params);

        REQUIRE(ret.find("result") != ret.end());
        REQUIRE(ret["result"]["balance"].get<double>() == dBalance);
    }


    {
        double dBalance = 0.0;

        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"]        = PIN;
        params["session"]    = SESSION1;
        params["name"]       = strAccount2;

        /* Invoke the API */
        ret = APICall("tokens/get/account", params);

        REQUIRE(ret.find("result") != ret.end());
        REQUIRE(ret["result"]["balance"].get<double>() == dBalance);
    }


    {
        double dBalance = 0.0;

        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"]        = PIN;
        params["session"]    = SESSION1;
        params["name"]       = strAccount3;

        /* Invoke the API */
        ret = APICall("tokens/get/account", params);

        REQUIRE(ret.find("result") != ret.end());
        REQUIRE(ret["result"]["balance"].get<double>() == dBalance);
    }


    {
        double dBalance = 0.0;

        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"]        = PIN;
        params["session"]    = SESSION1;
        params["name"]       = strAccount4;

        /* Invoke the API */
        ret = APICall("tokens/get/account", params);

        REQUIRE(ret.find("result") != ret.end());
        REQUIRE(ret["result"]["balance"].get<double>() == dBalance);
    }


    {
        double dBalance = 0.0;

        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"]        = PIN;
        params["session"]    = SESSION1;
        params["name"]       = strAccount5;

        /* Invoke the API */
        ret = APICall("tokens/get/account", params);

        REQUIRE(ret.find("result") != ret.end());
        REQUIRE(ret["result"]["balance"].get<double>() == dBalance);
    }


    {
        double dBalance = 0.0;

        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"]        = PIN;
        params["session"]    = SESSION1;
        params["name"]       = strAccount6;

        /* Invoke the API */
        ret = APICall("tokens/get/account", params);

        REQUIRE(ret.find("result") != ret.end());
        REQUIRE(ret["result"]["balance"].get<double>() == dBalance);
    }



    /* Test success case with multiple recipients */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"] = PIN;
        params["session"] = SESSION1;
        params["name"]    = strToken1;

        /* create json array with 50 recipients */
        json::json jsonRecipients = json::json::array();
        {
            json::json jsonRecipient;
            jsonRecipient["amount"] = "10000";
            jsonRecipient["name_to"] = strAccount1;
            jsonRecipients.push_back(jsonRecipient);
        }


        {
            json::json jsonRecipient;
            jsonRecipient["amount"] = "29845";
            jsonRecipient["name_to"] = strAccount2;
            jsonRecipients.push_back(jsonRecipient);
        }

        params["recipients"] = jsonRecipients;

        /* Invoke the API */
        ret = APICall("tokens/debit/token", params);

        REQUIRE(ret.find("result") != ret.end());
        result = ret["result"];
        REQUIRE(result.find("txid") != result.end());

        //build a block now
        REQUIRE(GenerateBlock());

        //get our txid for a credit.
        std::string strTXID = result["txid"];

        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"] = PIN;
        params["session"] = SESSION1;
        params["txid"]   = strTXID;

        /* Invoke the API */
        ret = APICall("tokens/credit/account", params);

        REQUIRE(ret.find("result") != ret.end());
        result = ret["result"];
        REQUIRE(result.find("txid") != result.end());
    }


    //lets check our accounts now
    double dBalance1 = 10000;
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"]        = PIN;
        params["session"]    = SESSION1;
        params["name"]       = strAccount1;

        /* Invoke the API */
        ret = APICall("tokens/get/account", params);

        REQUIRE(ret.find("result") != ret.end());
        result = ret["result"];

        REQUIRE(result.find("unconfirmed") != result.end());
        REQUIRE(result["unconfirmed"].get<double>() == dBalance1);
    }


    double dBalance2 = 29845;
    {


        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"]        = PIN;
        params["session"]    = SESSION1;
        params["name"]       = strAccount2;

        /* Invoke the API */
        ret = APICall("tokens/get/account", params);

        REQUIRE(ret.find("result") != ret.end());
        result = ret["result"];

        REQUIRE(result.find("unconfirmed") != result.end());
        REQUIRE(result["unconfirmed"].get<double>() == dBalance2);
    }


    //build a block now
    REQUIRE(GenerateBlock());



    //lets check our accounts now
    {

        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"]        = PIN;
        params["session"]    = SESSION1;
        params["name"]       = strAccount1;

        /* Invoke the API */
        ret = APICall("tokens/get/account", params);

        REQUIRE(ret.find("result") != ret.end());
        REQUIRE(ret["result"]["balance"].get<double>() == dBalance1);
    }


    {

        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"]        = PIN;
        params["session"]    = SESSION1;
        params["name"]       = strAccount2;

        /* Invoke the API */
        ret = APICall("tokens/get/account", params);

        REQUIRE(ret.find("result") != ret.end());
        REQUIRE(ret["result"]["balance"].get<double>() == dBalance2);
    }





    /* Test success case with multiple recipients */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"] = PIN;
        params["session"] = SESSION1;
        params["name"]    = strToken2;

        /* create json array with 50 recipients */
        json::json jsonRecipients = json::json::array();
        {
            json::json jsonRecipient;
            jsonRecipient["amount"] = "184943.333";
            jsonRecipient["name_to"] = strAccount3;
            jsonRecipients.push_back(jsonRecipient);
        }


        {
            json::json jsonRecipient;
            jsonRecipient["amount"] = "83828.777";
            jsonRecipient["name_to"] = strAccount4;
            jsonRecipients.push_back(jsonRecipient);
        }

        params["recipients"] = jsonRecipients;

        /* Invoke the API */
        ret = APICall("tokens/debit/token", params);

        REQUIRE(ret.find("result") != ret.end());
        result = ret["result"];
        REQUIRE(result.find("txid") != result.end());

        //build a block now
        REQUIRE(GenerateBlock());

        //get our txid for a credit.
        std::string strTXID = result["txid"];

        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"] = PIN;
        params["session"] = SESSION1;
        params["txid"]   = strTXID;

        /* Invoke the API */
        ret = APICall("tokens/credit/account", params);

        REQUIRE(ret.find("result") != ret.end());
        result = ret["result"];
        REQUIRE(result.find("txid") != result.end());
    }


    //lets check our accounts now
    double dBalance3 = 184943.333;
    {


        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"]        = PIN;
        params["session"]    = SESSION1;
        params["name"]       = strAccount3;

        /* Invoke the API */
        ret = APICall("tokens/get/account", params);

        REQUIRE(ret.find("result") != ret.end());
        result = ret["result"];

        REQUIRE(result.find("unconfirmed") != result.end());
        REQUIRE(result["unconfirmed"].get<double>() == dBalance3);
    }


    double dBalance4 = 83828.777;
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"]        = PIN;
        params["session"]    = SESSION1;
        params["name"]       = strAccount4;

        /* Invoke the API */
        ret = APICall("tokens/get/account", params);

        REQUIRE(ret.find("result") != ret.end());
        result = ret["result"];

        REQUIRE(result.find("unconfirmed") != result.end());
        REQUIRE(result["unconfirmed"].get<double>() == dBalance4);
    }


    //build a block now
    REQUIRE(GenerateBlock());



    //lets check our accounts now
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"]        = PIN;
        params["session"]    = SESSION1;
        params["name"]       = strAccount3;

        /* Invoke the API */
        ret = APICall("tokens/get/account", params);

        REQUIRE(ret.find("result") != ret.end());
        REQUIRE(ret["result"]["balance"].get<double>() == dBalance3);
    }


    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"]        = PIN;
        params["session"]    = SESSION1;
        params["name"]       = strAccount4;

        /* Invoke the API */
        ret = APICall("tokens/get/account", params);

        REQUIRE(ret.find("result") != ret.end());
        REQUIRE(ret["result"]["balance"].get<double>() == dBalance4);
    }



    /* Test success case with multiple recipients */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"] = PIN;
        params["session"] = SESSION1;
        params["name"]    = strToken3;

        /* create json array with 50 recipients */
        json::json jsonRecipients = json::json::array();
        {
            json::json jsonRecipient;
            jsonRecipient["amount"] = "33377.888";
            jsonRecipient["name_to"] = strAccount5;
            jsonRecipients.push_back(jsonRecipient);
        }


        {
            json::json jsonRecipient;
            jsonRecipient["amount"] = "83338.777";
            jsonRecipient["name_to"] = strAccount6;
            jsonRecipients.push_back(jsonRecipient);
        }

        params["recipients"] = jsonRecipients;

        /* Invoke the API */
        ret = APICall("tokens/debit/token", params);

        REQUIRE(ret.find("result") != ret.end());
        result = ret["result"];
        REQUIRE(result.find("txid") != result.end());

        //build a block now
        REQUIRE(GenerateBlock());

        //get our txid for a credit.
        std::string strTXID = result["txid"];

        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"] = PIN;
        params["session"] = SESSION1;
        params["txid"]   = strTXID;

        /* Invoke the API */
        ret = APICall("tokens/credit/account", params);

        REQUIRE(ret.find("result") != ret.end());
        result = ret["result"];
        REQUIRE(result.find("txid") != result.end());
    }


    //lets check our accounts now
    double dBalance5 = 33377.888;
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"]        = PIN;
        params["session"]    = SESSION1;
        params["name"]       = strAccount5;

        /* Invoke the API */
        ret = APICall("tokens/get/account", params);

        REQUIRE(ret.find("result") != ret.end());
        result = ret["result"];

        REQUIRE(result.find("unconfirmed") != result.end());
        REQUIRE(result["unconfirmed"].get<double>() == dBalance5);
    }


    double dBalance6 = 83338.777;
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"]        = PIN;
        params["session"]    = SESSION1;
        params["name"]       = strAccount6;

        /* Invoke the API */
        ret = APICall("tokens/get/account", params);

        REQUIRE(ret.find("result") != ret.end());
        result = ret["result"];

        REQUIRE(result.find("unconfirmed") != result.end());
        REQUIRE(result["unconfirmed"].get<double>() == dBalance6);
    }


    //build a block now
    REQUIRE(GenerateBlock());



    //lets check our accounts now
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"]        = PIN;
        params["session"]    = SESSION1;
        params["name"]       = strAccount5;

        /* Invoke the API */
        ret = APICall("tokens/get/account", params);

        REQUIRE(ret.find("result") != ret.end());
        REQUIRE(ret["result"]["balance"].get<double>() == dBalance5);
    }


    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"]        = PIN;
        params["session"]    = SESSION1;
        params["name"]       = strAccount6;

        /* Invoke the API */
        ret = APICall("tokens/get/account", params);

        REQUIRE(ret.find("result") != ret.end());
        REQUIRE(ret["result"]["balance"].get<double>() == dBalance6);
    }



    /* Create a new account. */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"]        = PIN;
        params["session"]    = SESSION1;
        params["name"]       = strAny1;
        params["token_name"] = strToken1;

        /* Invoke the API */
        ret = APICall("tokens/create/account", params);

        REQUIRE(ret.find("result") != ret.end());
        result = ret["result"];
        REQUIRE(result.find("txid") != result.end());
    }


    /* Create a new account. */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"]        = PIN;
        params["session"]    = SESSION1;
        params["name"]       = strAny2;
        params["token_name"] = strToken2;

        /* Invoke the API */
        ret = APICall("tokens/create/account", params);

        REQUIRE(ret.find("result") != ret.end());
        result = ret["result"];
        REQUIRE(result.find("txid") != result.end());
    }


    /* Create a new account. */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"]        = PIN;
        params["session"]    = SESSION1;
        params["name"]       = strAny3;
        params["token_name"] = strToken3;

        /* Invoke the API */
        ret = APICall("tokens/create/account", params);

        REQUIRE(ret.find("result") != ret.end());
        result = ret["result"];
        REQUIRE(result.find("txid") != result.end());
    }


    //build a block now
    REQUIRE(GenerateBlock());


    //lets check our accounts now
    {
        double dBalance = 0.0;

        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"]        = PIN;
        params["session"]    = SESSION1;
        params["name"]       = strAny1;

        /* Invoke the API */
        ret = APICall("tokens/get/account", params);

        REQUIRE(ret.find("result") != ret.end());
        REQUIRE(ret["result"]["balance"].get<double>() == dBalance);
    }


    {
        double dBalance = 0.0;

        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"]        = PIN;
        params["session"]    = SESSION1;
        params["name"]       = strAny2;

        /* Invoke the API */
        ret = APICall("tokens/get/account", params);

        REQUIRE(ret.find("result") != ret.end());
        REQUIRE(ret["result"]["balance"].get<double>() == dBalance);
    }


    {
        double dBalance = 0.0;

        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"]        = PIN;
        params["session"]    = SESSION1;
        params["name"]       = strAny3;

        /* Invoke the API */
        ret = APICall("tokens/get/account", params);

        REQUIRE(ret.find("result") != ret.end());
        REQUIRE(ret["result"]["balance"].get<double>() == dBalance);
    }


    /* Finally we can do a debit/any */
    double dBalanceAny1 = dBalance1 + dBalance2;
    double dBalanceAny2 = dBalance3 + dBalance4;
    double dBalanceAny3 = dBalance5 + dBalance6;

    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"] = PIN;
        params["session"] = SESSION1;

        /* create json array with 50 recipients */
        json::json jsonRecipients = json::json::array();
        {
            json::json jsonRecipient;
            jsonRecipient["amount"] = dBalanceAny1;
            jsonRecipient["name_to"] = strAny1;
            jsonRecipients.push_back(jsonRecipient);
        }


        {
            json::json jsonRecipient;
            jsonRecipient["amount"] = dBalanceAny2;
            jsonRecipient["name_to"] = strAny2;
            jsonRecipients.push_back(jsonRecipient);
        }


        {
            json::json jsonRecipient;
            jsonRecipient["amount"] = dBalanceAny3;
            jsonRecipient["name_to"] = strAny3;
            jsonRecipients.push_back(jsonRecipient);
        }

        params["recipients"] = jsonRecipients;

        /* Invoke the API */
        ret = APICall("tokens/debit/any", params);

        REQUIRE(ret.find("result") != ret.end());
        result = ret["result"];
        REQUIRE(result.find("txid") != result.end());

        //build a block now
        REQUIRE(GenerateBlock());

        //get our txid for a credit.
        std::string strTXID = result["txid"];

        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"] = PIN;
        params["session"] = SESSION1;
        params["txid"]   = strTXID;

        /* Invoke the API */
        ret = APICall("tokens/credit/account", params);

        REQUIRE(ret.find("result") != ret.end());
        result = ret["result"];
        REQUIRE(result.find("txid") != result.end());
    }


    //lets check our accounts now

    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"]        = PIN;
        params["session"]    = SESSION1;
        params["name"]       = strAny1;

        /* Invoke the API */
        ret = APICall("tokens/get/account", params);

        REQUIRE(ret.find("result") != ret.end());
        REQUIRE(ret["result"]["unconfirmed"].get<double>() * nFigures1 == dBalanceAny1 * nFigures1);
    }



    {


        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"]        = PIN;
        params["session"]    = SESSION1;
        params["name"]       = strAny2;

        /* Invoke the API */
        ret = APICall("tokens/get/account", params);

        REQUIRE(ret.find("result") != ret.end());
        REQUIRE(ret["result"]["unconfirmed"].get<double>() * nFigures2 == dBalanceAny2 * nFigures2);
    }


    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"]        = PIN;
        params["session"]    = SESSION1;
        params["name"]       = strAny3;

        /* Invoke the API */
        ret = APICall("tokens/get/account", params);

        REQUIRE(ret.find("result") != ret.end());
        REQUIRE(ret["result"]["unconfirmed"].get<double>() * nFigures3 == dBalanceAny3 * nFigures3);
    }


    //build a block now
    REQUIRE(GenerateBlock());


    //lets check our accounts now
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"]        = PIN;
        params["session"]    = SESSION1;
        params["name"]       = strAny1;

        /* Invoke the API */
        ret = APICall("tokens/get/account", params);

        REQUIRE(ret.find("result") != ret.end());
        REQUIRE(ret["result"]["balance"].get<double>() * nFigures1 == dBalanceAny1 * nFigures1);
    }


    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"]        = PIN;
        params["session"]    = SESSION1;
        params["name"]       = strAny2;

        /* Invoke the API */
        ret = APICall("tokens/get/account", params);

        REQUIRE(ret.find("result") != ret.end());
        REQUIRE(ret["result"]["balance"].get<double>() * nFigures2 == dBalanceAny2 * nFigures2);
    }


    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"]        = PIN;
        params["session"]    = SESSION1;
        params["name"]       = strAny3;

        /* Invoke the API */
        ret = APICall("tokens/get/account", params);

        REQUIRE(ret.find("result") != ret.end());
        REQUIRE(ret["result"]["balance"].get<double>() * nFigures3 == dBalanceAny3 * nFigures3);
    }
}



TEST_CASE( "Test Tokens API - debit all", "[tokens]")
{
    /* Declare variables shared across test cases */
    json::json params;
    json::json ret;
    json::json result;
    json::json error;

    //track our accounts
    std::string strToken = "token";
    std::string strAccount1 = "account1";
    std::string strAccount2 = "account2";
    std::string strAccount3 = "account3";
    std::string strAccount4 = "account4";
    std::string strAccount5 = "account5";
    std::string strAccount6 = "account6";

    /* Ensure user is created and logged in for testing */
    InitializeUser(USERNAME1, PASSWORD, PIN, GENESIS1, SESSION1);


    /* Create a new token. */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"]      = PIN;
        params["session"]  = SESSION1;
        params["name"]     = strToken;
        params["supply"]   = "1000000";
        params["decimals"] = "4";

        /* Invoke the API */
        ret = APICall("tokens/create/token", params);

        REQUIRE(ret.find("result") != ret.end());
        result = ret["result"];
        REQUIRE(result.find("txid") != result.end());
    }


    /* Create a new account. */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"]        = PIN;
        params["session"]    = SESSION1;
        params["name"]       = strAccount1;
        params["token_name"] = strToken;

        /* Invoke the API */
        ret = APICall("tokens/create/account", params);

        REQUIRE(ret.find("result") != ret.end());
        result = ret["result"];
        REQUIRE(result.find("txid") != result.end());
    }


    /* Create a new account. */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"]        = PIN;
        params["session"]    = SESSION1;
        params["name"]       = strAccount2;
        params["token_name"] = strToken;

        /* Invoke the API */
        ret = APICall("tokens/create/account", params);

        REQUIRE(ret.find("result") != ret.end());
        result = ret["result"];
        REQUIRE(result.find("txid") != result.end());
    }


    /* Create a new account. */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"]        = PIN;
        params["session"]    = SESSION1;
        params["name"]       = strAccount3;
        params["token_name"] = strToken;

        /* Invoke the API */
        ret = APICall("tokens/create/account", params);

        REQUIRE(ret.find("result") != ret.end());
        result = ret["result"];
        REQUIRE(result.find("txid") != result.end());
    }


    /* Create a new account. */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"]        = PIN;
        params["session"]    = SESSION1;
        params["name"]       = strAccount4;
        params["token_name"] = strToken;

        /* Invoke the API */
        ret = APICall("tokens/create/account", params);

        REQUIRE(ret.find("result") != ret.end());
        result = ret["result"];
        REQUIRE(result.find("txid") != result.end());
    }


    /* Create a new account. */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"]        = PIN;
        params["session"]    = SESSION1;
        params["name"]       = strAccount5;
        params["token_name"] = strToken;

        /* Invoke the API */
        ret = APICall("tokens/create/account", params);

        REQUIRE(ret.find("result") != ret.end());
        result = ret["result"];
        REQUIRE(result.find("txid") != result.end());
    }


    /* Create a new account. */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"]        = PIN;
        params["session"]    = SESSION1;
        params["name"]       = strAccount6;
        params["token_name"] = strToken;

        /* Invoke the API */
        ret = APICall("tokens/create/account", params);

        REQUIRE(ret.find("result") != ret.end());
        result = ret["result"];
        REQUIRE(result.find("txid") != result.end());
    }

    //build a block now
    REQUIRE(GenerateBlock());


    /* Test success case by name_to */
    uint64_t nBalance = 0;
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"] = PIN;
        params["session"] = SESSION1;
        params["name"]   = strToken;

        uint64_t nAmount = LLC::GetRand(1000);
        nBalance += nAmount;
        params["amount"] = debug::safe_printstr(nAmount);
        params["name_to"] = strAccount1;

        /* Invoke the API */
        ret = APICall("tokens/debit/token", params);

        REQUIRE(ret.find("result") != ret.end());
        result = ret["result"];
        REQUIRE(result.find("txid") != result.end());

        //build a block now
        REQUIRE(GenerateBlock());

        //now build our credit
        std::string strTXID = result["txid"];

        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"] = PIN;
        params["session"] = SESSION1;
        params["txid"]   = strTXID;

        /* Invoke the API */
        ret = APICall("tokens/credit/account", params);

        REQUIRE(ret.find("result") != ret.end());
        result = ret["result"];
        REQUIRE(result.find("txid") != result.end());
     }


 /* Test success case by name_to */
 {
     /* Build the parameters to pass to the API */
     params.clear();
     params["pin"] = PIN;
     params["session"] = SESSION1;
     params["name"]   = strToken;

     uint64_t nAmount = LLC::GetRand(1000);
     nBalance += nAmount;
     params["amount"] = debug::safe_printstr(nAmount);
     params["name_to"] = strAccount2;

     /* Invoke the API */
     ret = APICall("tokens/debit/token", params);

     REQUIRE(ret.find("result") != ret.end());
     result = ret["result"];
     REQUIRE(result.find("txid") != result.end());

     //build a block now
     REQUIRE(GenerateBlock());

     //now build our credit
     std::string strTXID = result["txid"];

     /* Build the parameters to pass to the API */
     params.clear();
     params["pin"] = PIN;
     params["session"] = SESSION1;
     params["txid"]   = strTXID;

     /* Invoke the API */
     ret = APICall("tokens/credit/account", params);

     REQUIRE(ret.find("result") != ret.end());
     result = ret["result"];
     REQUIRE(result.find("txid") != result.end());
  }


  /* Test success case by name_to */
  {
      /* Build the parameters to pass to the API */
      params.clear();
      params["pin"] = PIN;
      params["session"] = SESSION1;
      params["name"]   = strToken;

      uint64_t nAmount = LLC::GetRand(1000);
      nBalance += nAmount;
      params["amount"] = debug::safe_printstr(nAmount);
      params["name_to"] = strAccount3;

      /* Invoke the API */
      ret = APICall("tokens/debit/token", params);

      REQUIRE(ret.find("result") != ret.end());
      result = ret["result"];
      REQUIRE(result.find("txid") != result.end());

      //build a block now
      REQUIRE(GenerateBlock());

      //now build our credit
      std::string strTXID = result["txid"];

      /* Build the parameters to pass to the API */
      params.clear();
      params["pin"] = PIN;
      params["session"] = SESSION1;
      params["txid"]   = strTXID;

      /* Invoke the API */
      ret = APICall("tokens/credit/account", params);

      REQUIRE(ret.find("result") != ret.end());
      result = ret["result"];
      REQUIRE(result.find("txid") != result.end());
   }


   /* Test success case by name_to */
   {
       /* Build the parameters to pass to the API */
       params.clear();
       params["pin"] = PIN;
       params["session"] = SESSION1;
       params["name"]   = strToken;

       uint64_t nAmount = LLC::GetRand(1000);
       nBalance += nAmount;
       params["amount"] = debug::safe_printstr(nAmount);
       params["name_to"] = strAccount4;

       /* Invoke the API */
       ret = APICall("tokens/debit/token", params);

       REQUIRE(ret.find("result") != ret.end());
       result = ret["result"];
       REQUIRE(result.find("txid") != result.end());

       //build a block now
       REQUIRE(GenerateBlock());

       //now build our credit
       std::string strTXID = result["txid"];

       /* Build the parameters to pass to the API */
       params.clear();
       params["pin"] = PIN;
       params["session"] = SESSION1;
       params["txid"]   = strTXID;

       /* Invoke the API */
       ret = APICall("tokens/credit/account", params);

       REQUIRE(ret.find("result") != ret.end());
       result = ret["result"];
       REQUIRE(result.find("txid") != result.end());
    }


    /* Test success case by name_to */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"] = PIN;
        params["session"] = SESSION1;
        params["name"]   = strToken;

        uint64_t nAmount = LLC::GetRand(1000);
        nBalance += nAmount;
        params["amount"] = debug::safe_printstr(nAmount);
        params["name_to"] = strAccount5;

        /* Invoke the API */
        ret = APICall("tokens/debit/token", params);

        REQUIRE(ret.find("result") != ret.end());
        result = ret["result"];
        REQUIRE(result.find("txid") != result.end());

        //build a block now
        REQUIRE(GenerateBlock());

        //now build our credit
        std::string strTXID = result["txid"];

        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"] = PIN;
        params["session"] = SESSION1;
        params["txid"]   = strTXID;

        /* Invoke the API */
        ret = APICall("tokens/credit/account", params);

        REQUIRE(ret.find("result") != ret.end());
        result = ret["result"];
        REQUIRE(result.find("txid") != result.end());
     }



    /* Test success case by name_to */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"] = PIN;
        params["session"] = SESSION1;
        params["name"]   = strToken;

        uint64_t nAmount = LLC::GetRand(1000);
        nBalance += nAmount;
        params["amount"] = debug::safe_printstr(nAmount);
        params["name_to"] = strAccount5;

        /* Invoke the API */
        ret = APICall("tokens/debit/token", params);

        REQUIRE(ret.find("result") != ret.end());
        result = ret["result"];
        REQUIRE(result.find("txid") != result.end());

        //build a block now
        REQUIRE(GenerateBlock());

        //now build our credit
        std::string strTXID = result["txid"];

        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"] = PIN;
        params["session"] = SESSION1;
        params["txid"]   = strTXID;

        /* Invoke the API */
        ret = APICall("tokens/credit/account", params);

        REQUIRE(ret.find("result") != ret.end());
        result = ret["result"];
        REQUIRE(result.find("txid") != result.end());
     }

     {
         /* Build the parameters to pass to the API */
         params.clear();
         params["pin"] = PIN;
         params["session"] = SESSION1;
         params["token_name"]   = strToken;

         /* Invoke the API */
         ret = APICall("finance/list/accounts", params);

         REQUIRE(ret.find("result") != ret.end());
     }


     //let's try to fail here by exceeding our balance
     {
         /* Build the parameters to pass to the API */
         params.clear();
         params["pin"]        = PIN;
         params["session"]    = SESSION1;
         params["token_name"] = strToken;
         params["name_to"]    = strAccount6;
         params["amount"]     = debug::safe_printstr(nBalance + 10);

         /* Invoke the API */
         ret = APICall("tokens/debit/all", params);

         /* Check response is an error and validate error code */
         REQUIRE(ret.find("error") != ret.end());
         REQUIRE(ret["error"]["code"].get<int32_t>() == -69);
     }


     {
         /* Build the parameters to pass to the API */
         params.clear();
         params["pin"]        = PIN;
         params["session"]    = SESSION1;
         params["token_name"] = strToken;
         params["name_to"]    = strAccount6;
         params["amount"]     = debug::safe_printstr(nBalance);

         /* Invoke the API */
         ret = APICall("tokens/debit/all", params);

         REQUIRE(ret.find("result") != ret.end());
         result = ret["result"];
         REQUIRE(result.find("txid") != result.end());

         //build a block now
         REQUIRE(GenerateBlock());

         //now build our credit
         std::string strTXID = result["txid"];

         /* Build the parameters to pass to the API */
         params.clear();
         params["pin"] = PIN;
         params["session"] = SESSION1;
         params["txid"]   = strTXID;

         /* Invoke the API */
         ret = APICall("tokens/credit/account", params);

         REQUIRE(ret.find("result") != ret.end());
         result = ret["result"];
         REQUIRE(result.find("txid") != result.end());

         /* Build the parameters to pass to the API */
         params.clear();
         params["pin"] = PIN;
         params["session"] = SESSION1;
         params["name"]   = strAccount6;

         /* Invoke the API */
         ret = APICall("finance/get/account", params);

         REQUIRE(ret.find("result") != ret.end());
         REQUIRE(ret["result"]["unconfirmed"].get<double>() == nBalance);

         //build a block now
         REQUIRE(GenerateBlock());
     }


     {
         /* Build the parameters to pass to the API */
         params.clear();
         params["pin"] = PIN;
         params["session"] = SESSION1;
         params["name"]   = strAccount6;

         /* Invoke the API */
         ret = APICall("finance/get/account", params);

         REQUIRE(ret.find("result") != ret.end());
         REQUIRE(ret["result"]["balance"].get<double>() == nBalance);
     }


     {
         /* Build the parameters to pass to the API */
         params.clear();
         params["pin"] = PIN;
         params["session"] = SESSION1;
         params["name"]   = strAccount1;

         /* Invoke the API */
         ret = APICall("finance/get/account", params);

         REQUIRE(ret.find("result") != ret.end());
         REQUIRE(ret["result"]["balance"].get<double>() == 0);
     }

     {
         /* Build the parameters to pass to the API */
         params.clear();
         params["pin"] = PIN;
         params["session"] = SESSION1;
         params["name"]   = strAccount2;

         /* Invoke the API */
         ret = APICall("finance/get/account", params);

         REQUIRE(ret.find("result") != ret.end());
         REQUIRE(ret["result"]["balance"].get<double>() == 0);
     }

     {
         /* Build the parameters to pass to the API */
         params.clear();
         params["pin"] = PIN;
         params["session"] = SESSION1;
         params["name"]   = strAccount3;

         /* Invoke the API */
         ret = APICall("finance/get/account", params);

         REQUIRE(ret.find("result") != ret.end());
         REQUIRE(ret["result"]["balance"].get<double>() == 0);
     }

     {
         /* Build the parameters to pass to the API */
         params.clear();
         params["pin"] = PIN;
         params["session"] = SESSION1;
         params["name"]   = strAccount4;

         /* Invoke the API */
         ret = APICall("finance/get/account", params);

         REQUIRE(ret.find("result") != ret.end());
         REQUIRE(ret["result"]["balance"].get<double>() == 0);
     }


     {
         /* Build the parameters to pass to the API */
         params.clear();
         params["pin"] = PIN;
         params["session"] = SESSION1;
         params["name"]   = strAccount5;

         /* Invoke the API */
         ret = APICall("finance/get/account", params);

         REQUIRE(ret.find("result") != ret.end());
         REQUIRE(ret["result"]["balance"].get<double>() == 0);
     }
}

TEST_CASE( "Test Tokens API - credit account", "[tokens]")
{
    /* Declare variables shared across test cases */
    json::json params;
    json::json ret;
    json::json result;
    json::json error;

    /* Generate random token name */
    std::string strToken = "TOKEN" + std::to_string(LLC::GetRand());
    TAO::Register::Address hashToken = TAO::Register::Address(TAO::Register::Address::TOKEN);

    std::string strAccount = "ACCOUNT" + std::to_string(LLC::GetRand());
    TAO::Register::Address hashAccount = TAO::Register::Address(TAO::Register::Address::ACCOUNT);

    std::string strTXID;

    /* Ensure user is created and logged in for testing */
    InitializeUser(USERNAME1, PASSWORD, PIN, GENESIS1, SESSION1);

    /* create token to create account for  */
    {
        //create the transaction object
        TAO::Ledger::Transaction tx;
        tx.hashGenesis = GENESIS1;
        tx.nSequence   = 0;
        tx.nTimestamp  = runtime::timestamp();

        //create object
        TAO::Register::Object token = TAO::Register::CreateToken(hashToken, 10000, 2);

        //payload
        tx[0] << uint8_t(TAO::Operation::OP::CREATE) << hashToken << uint8_t(TAO::Register::REGISTER::OBJECT) << token.GetState();

        //create name
        tx[1] = TAO::API::Names::CreateName(GENESIS1, strToken, "", hashToken);

        //generate the prestates and poststates
        REQUIRE(tx.Build());

        //verify the prestates and poststates
        REQUIRE(tx.Verify());

        //commit to disk
        REQUIRE(Execute(tx[0], TAO::Ledger::FLAGS::BLOCK));

        //commit to disk
        REQUIRE(Execute(tx[1], TAO::Ledger::FLAGS::BLOCK));

    }

    /* create account to debit */
    {
        //create the transaction object
        TAO::Ledger::Transaction tx;
        tx.hashGenesis = GENESIS1;
        tx.nSequence   = 1;
        tx.nTimestamp  = runtime::timestamp();

        //create object
        TAO::Register::Object account = TAO::Register::CreateAccount(hashToken);

        //create name
        TAO::Register::Object name = TAO::Register::CreateName("", strToken, hashAccount);

        //payload
        tx[0] << uint8_t(TAO::Operation::OP::CREATE) << hashAccount << uint8_t(TAO::Register::REGISTER::OBJECT) << account.GetState();

        //create name
        tx[1] = TAO::API::Names::CreateName(GENESIS1, strAccount, "", hashAccount);

        //generate the prestates and poststates
        REQUIRE(tx.Build());

        //verify the prestates and poststates
        REQUIRE(tx.Verify());

        //commit to disk
        REQUIRE(Execute(tx[0], TAO::Ledger::FLAGS::BLOCK));

        //commit to disk
        REQUIRE(Execute(tx[1], TAO::Ledger::FLAGS::BLOCK));
    }


    /* Debit the token and credit the account so that we have some funds to debit */
    {
        TAO::Ledger::Transaction tx;
        tx.hashGenesis = GENESIS1;
        tx.nSequence   = 2;
        tx.nTimestamp  = runtime::timestamp();


        //payload
        tx[0] << uint8_t(TAO::Operation::OP::DEBIT) << hashToken << hashAccount << uint64_t(1000) << uint64_t(0);

        //generate the prestates and poststates
        REQUIRE(tx.Build());

        //verify the prestates and poststates
        REQUIRE(tx.Verify());

        //write transaction
        REQUIRE(LLD::Ledger->WriteTx(tx.GetHash(), tx));
        REQUIRE(LLD::Ledger->IndexBlock(tx.GetHash(), TAO::Ledger::ChainState::Genesis()));

        //commit to disk
        REQUIRE(Execute(tx[0], TAO::Ledger::FLAGS::BLOCK));

        strTXID = tx.GetHash().GetHex();
    }


    /* Test fail with missing txid */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"] = PIN;
        params["session"] = SESSION1;

        /* Invoke the API */
        ret = APICall("tokens/credit/account", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -50);
    }

    /* Test fail with invalid txid */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"]        = PIN;
        params["session"]    = SESSION1;
        params["txid"]       = RandomTxid().ToString();

        /* Invoke the API */
        ret = APICall("tokens/credit/account", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -40);
    }

    /* Test fail with invalid type noun */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"]     = PIN;
        params["session"] = SESSION1;
        params["txid"]    = strTXID;

        /* Invoke the API */
        ret = APICall("tokens/credit/token", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -49);
    }

    /* Test success case */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"]     = PIN;
        params["session"] = SESSION1;
        params["txid"]    = strTXID;

        /* Invoke the API */
        ret = APICall("tokens/credit/account", params);

        /* Check the result */
        REQUIRE(ret.find("result") != ret.end());
        result = ret["result"];
        REQUIRE(result.find("txid") != result.end());
    }


    //test failure for missing address/name
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"]     = PIN;
        params["session"] = SESSION1;

        /* Invoke the API */
        ret = APICall("tokens/burn", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -33);
    }


    //test failure for missing type
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"]     = PIN;
        params["session"] = SESSION1;
        params["name"]    = strAccount;
        params["amount"]  = "100";

        /* Invoke the API */
        ret = APICall("tokens/burn", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -118);
    }


    //test failure for invalid type
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"]     = PIN;
        params["session"] = SESSION1;
        params["name"]    = strAccount;
        params["amount"]  = "100";

        /* Invoke the API */
        ret = APICall("tokens/burn/account", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -36);
    }


    //test failure for object not account
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"]     = PIN;
        params["session"] = SESSION1;
        params["name"]    = strToken;
        params["amount"]  = "100";

        /* Invoke the API */
        ret = APICall("tokens/burn/token", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -65);
    }


    /* Test success case */
    std::string strBurnID;
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"]     = PIN;
        params["session"] = SESSION1;
        params["name"]    = strAccount;
        params["amount"]  = "10";

        /* Invoke the API */
        ret = APICall("tokens/burn/token", params);

        /* Check the result */
        REQUIRE(ret.find("result") != ret.end());
        result = ret["result"];
        REQUIRE(result.find("txid") != result.end());

        strBurnID = result["txid"];
    }


    /* Test failure crediting with Unexpected type. */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"]     = PIN;
        params["session"] = SESSION1;
        params["txid"]    = strBurnID;

        /* Invoke the API */
        ret = APICall("tokens/credit/account", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -49);
    }


    /* Test failure crediting account. */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"]     = PIN;
        params["session"] = SESSION1;
        params["txid"]    = strBurnID;

        /* Invoke the API */
        ret = APICall("tokens/credit/token", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -32);
    }
}

TEST_CASE( "Test Tokens API - get account", "[tokens]")
{
    /* Declare variables shared across test cases */
    json::json params;
    json::json ret;
    json::json result;
    json::json error;

    /* Generate random token name */
    std::string strToken = "TOKEN" +std::to_string(LLC::GetRand());
    TAO::Register::Address hashToken = TAO::Register::Address(TAO::Register::Address::TOKEN);
    std::string strAccount = "ACCOUNT" +std::to_string(LLC::GetRand());
    TAO::Register::Address hashAccount = TAO::Register::Address(TAO::Register::Address::ACCOUNT);

    std::string strTXID;

    /* Ensure user is created and logged in for testing */
    InitializeUser(USERNAME1, PASSWORD, PIN, GENESIS1, SESSION1);

    /* create token to create account for  */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"] = PIN;
        params["session"] = SESSION1;
        params["name"] = strToken;
        params["supply"] = "10000";
        params["decimals"] = "2";

        /* Invoke the API */
        ret = APICall("tokens/create/token", params);

        /* Check that the result is as we expect it to be */
        REQUIRE(ret.find("result") != ret.end());
        result = ret["result"];

        REQUIRE(result.find("txid") != result.end());
        REQUIRE(result.find("address") != result.end());
        hashToken.SetBase58( result["address"].get<std::string>());
    }

    /* tokens/create/account by token address success */
    {

        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"] = PIN;
        params["session"] = SESSION1;
        params["name"] = strAccount;
        params["token"] = hashToken.ToString();

        /* Invoke the API */
        ret = APICall("tokens/create/account", params);

        /* Check that the result is as we expect it to be */
        REQUIRE(ret.find("result") != ret.end());
        result = ret["result"];

        REQUIRE(result.find("txid") != result.end());
        REQUIRE(result.find("address") != result.end());
        hashAccount.SetBase58( result["address"].get<std::string>());
    }

    /* Test fail with missing name / address */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"] = PIN;
        params["session"] = SESSION1;

        /* Invoke the API */
        ret = APICall("tokens/get/account", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -33);
    }

    /* Test fail with invalid name */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"] = PIN;
        params["session"] = SESSION1;
        params["name"] = "asdfsdfsdf";

        /* Invoke the API */
        ret = APICall("tokens/get/account", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -101);
    }

    /* Test fail with invalid address */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"] = PIN;
        params["session"] = SESSION1;
        params["address"] = TAO::Register::Address(TAO::Register::Address::ACCOUNT).ToString();

        /* Invoke the API */
        ret = APICall("tokens/get/account", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -13);
    }

    /* Test successful get by name  */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"] = PIN;
        params["session"] = SESSION1;
        params["name"] = strAccount;

        /* Invoke the API */
        ret = APICall("tokens/get/account", params);

        /* Check that the result is as we expect it to be */
        REQUIRE(ret.find("result") != ret.end());
        result = ret["result"];

        REQUIRE(result.find("owner") != result.end());
        REQUIRE(result.find("created") != result.end());
        REQUIRE(result.find("modified") != result.end());
        REQUIRE(result.find("name") != result.end());
        REQUIRE(result.find("address") != result.end());
        //REQUIRE(result.find("token_name") != result.end());
        REQUIRE(result.find("token") != result.end());
        REQUIRE(result.find("balance") != result.end());
    }

    /* Test successful get by address  */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"] = PIN;
        params["session"] = SESSION1;
        params["address"] = hashAccount.ToString();

        /* Invoke the API */
        ret = APICall("tokens/get/account", params);

        /* Check that the result is as we expect it to be */
        REQUIRE(ret.find("result") != ret.end());
        result = ret["result"];

        REQUIRE(result.find("owner") != result.end());
        REQUIRE(result.find("created") != result.end());
        REQUIRE(result.find("modified") != result.end());
        REQUIRE(result.find("name") != result.end());
        REQUIRE(result.find("address") != result.end());
        //REQUIRE(result.find("token_name") != result.end());
        REQUIRE(result.find("token") != result.end());
        REQUIRE(result.find("balance") != result.end());

    }

}
