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

#include <TAO/API/types/names.h>

TEST_CASE( "Test Tokens API - create token", "[tokens/create/token]")
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
        params["session"] = SESSION1;

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

TEST_CASE( "Test Tokens API - debit token", "[tokens/debit/token]")
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
        params["address_to"] = TAO::Register::Address(TAO::Register::Address::ACCOUNT).ToString();
        params["address"] = TAO::Register::Address(TAO::Register::Address::TOKEN).ToString();

        /* Invoke the API */
        ret = APICall("tokens/debit/token", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -122);
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


TEST_CASE( "Test Tokens API - credit token", "[tokens/credit/token]")
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
        params["txid"] = LLC::GetRand256().GetHex();

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


TEST_CASE( "Test Tokens API - get token", "[tokens/get/token]")
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
        REQUIRE(ret["error"]["code"].get<int32_t>() == -122);
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

TEST_CASE( "Test Tokens API - create account", "[tokens/create/account]")
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

    /* tokens/create/account fail with missing token name / address*/
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"] = PIN;
        params["session"] = SESSION1;

        /* Invoke the API */
        ret = APICall("tokens/create/account", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -37);
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
}


TEST_CASE( "Test Tokens API - debit account", "[tokens/debit/account]")
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
    }



    /* Test fail with missing amount */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"] = PIN;
        params["session"] = SESSION1;

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
        REQUIRE(ret["error"]["code"].get<int32_t>() == -122);
    }

    /* Test fail with missing name_to / address_to */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"] = PIN;
        params["session"] = SESSION1;
        params["address"] = hashAccount.ToString();
        params["amount"] = "100";

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
        params["amount"] = "100";
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
        params["amount"] = "100";
        params["address_to"] = hashToken.GetHex();

        /* Invoke the API */
        ret = APICall("tokens/debit/account", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -69);
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

    /* Test success case by name_to */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"] = PIN;
        params["session"] = SESSION1;
        params["name"] = strAccount;
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
}

TEST_CASE( "Test Tokens API - credit account", "[tokens/credit/account]")
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
        params["pin"] = PIN;
        params["session"] = SESSION1;
        params["txid"] = LLC::GetRand256().GetHex();

        /* Invoke the API */
        ret = APICall("tokens/credit/account", params);

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
        ret = APICall("tokens/credit/account", params);

        /* Check the result */
        REQUIRE(ret.find("result") != ret.end());
        result = ret["result"];
        REQUIRE(result.find("txid") != result.end());
    }

}

TEST_CASE( "Test Tokens API - get account", "[tokens/get/account]")
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
        REQUIRE(ret["error"]["code"].get<int32_t>() == -122);
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
        REQUIRE(result.find("token_name") != result.end());
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
        REQUIRE(result.find("token_name") != result.end());
        REQUIRE(result.find("token") != result.end());
        REQUIRE(result.find("balance") != result.end());

    }

}
