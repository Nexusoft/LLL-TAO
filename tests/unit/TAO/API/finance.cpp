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

#include <TAO/Register/types/address.h>


TEST_CASE( "Test Finance API - create acccount", "[finance/create/account]")
{
    /* Declare variables shared across test cases */
    json::json params;
    json::json ret;
    json::json result;
    json::json error;

    std::string strAccount = "ACCOUNT" +std::to_string(LLC::GetRand());
    
    /* Ensure user is created and logged in for testing */
    InitializeUser(USERNAME1, PASSWORD, PIN, GENESIS1, SESSION1);

    /* finance/create/account fail with missing pin (only relevant for multiuser)*/
    if(config::fMultiuser.load())
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["session"] = SESSION1;

        /* Invoke the API */
        ret = APICall("finance/create/account", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -129);
    }

    /* finance/create/account fail with missing session (only relevant for multiuser)*/
    if(config::fMultiuser.load())
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"] = PIN;

        /* Invoke the API */
        ret = APICall("finance/create/account", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -12);
    }

    /* finance/create/account success */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"] = PIN;
        params["session"] = SESSION1;
        params["name"] = strAccount;

        /* Invoke the API */
        ret = APICall("finance/create/account", params);

        /* Check that the result is as we expect it to be */
        REQUIRE(ret.find("result") != ret.end());
        result = ret["result"];

        REQUIRE(result.find("txid") != result.end());
        REQUIRE(result.find("address") != result.end());
    }
}


TEST_CASE( "Test Finance API - get acccount", "[finance/get/account]")
{
    /* Declare variables shared across test cases */
    json::json params;
    json::json ret;
    json::json result;
    json::json error;

    std::string strAccount = "ACCOUNT" +std::to_string(LLC::GetRand());
    TAO::Register::Address hashAccount;
    
    /* Ensure user is created and logged in for testing */
    InitializeUser(USERNAME1, PASSWORD, PIN, GENESIS1, SESSION1);

    /* create an account to list */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"] = PIN;
        params["session"] = SESSION1;
        params["name"] = strAccount;

        /* Invoke the API */
        ret = APICall("finance/create/account", params);

        /* Check that the result is as we expect it to be */
        REQUIRE(ret.find("result") != ret.end());
        result = ret["result"];

        REQUIRE(result.find("txid") != result.end());
        REQUIRE(result.find("address") != result.end());

        hashAccount.SetBase58(result["address"].get<std::string>());
    }

    /* Failure case with missing name / address */
    {
        /* Invoke the API */
        params.clear();
        ret = APICall("finance/get/account", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -33);
    }

    /* Failure case with invalid name */
    {
       /* Build the parameters to pass to the API */
        params.clear();
        params["session"] = SESSION1;
        params["name"] = "notanaccount";

        /* Invoke the API */
        ret = APICall("finance/get/account", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -101);
    }

    /* Failure case with invalid address */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["session"] = SESSION1;
        params["address"] = TAO::Register::Address(TAO::Register::Address::ACCOUNT).ToString();

        /* Invoke the API */
        ret = APICall("finance/get/account", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -13);
    }

    /* Successful get by name */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["session"] = SESSION1;
        params["name"] = strAccount;

        /* Invoke the API */
        ret = APICall("finance/get/account", params);

        /* Check that the result is as we expect it to be */
        REQUIRE(ret.find("result") != ret.end());
        result = ret["result"];

        REQUIRE(result.find("name") != result.end());
        REQUIRE(result.find("address") != result.end());
        REQUIRE(result.find("token") != result.end());
        REQUIRE(result.find("balance") != result.end());
    }

    /* Successful get by address */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["session"] = SESSION1;
        params["address"] = hashAccount.ToString();

        /* Invoke the API */
        ret = APICall("finance/get/account", params);

        /* Check that the result is as we expect it to be */
        REQUIRE(ret.find("result") != ret.end());
        result = ret["result"];

        REQUIRE(result.find("owner") != result.end());
        REQUIRE(result.find("created") != result.end());
        REQUIRE(result.find("modified") != result.end());
        REQUIRE(result.find("name") != result.end());
        REQUIRE(result.find("address") != result.end());
        REQUIRE(result.find("token") != result.end());
        REQUIRE(result.find("balance") != result.end());
    }
}


TEST_CASE( "Test Finance API - list acccounts", "[finance/list/accounts]")
{
    /* Declare variables shared across test cases */
    json::json params;
    json::json ret;
    json::json result;
    json::json error;

    std::string strAccount = "ACCOUNT" +std::to_string(LLC::GetRand());
    TAO::Register::Address hashAccount;
    
    /* Ensure user is created and logged in for testing */
    InitializeUser(USERNAME1, PASSWORD, PIN, GENESIS1, SESSION1);

    /* create an account to list */
    {
        /* Build the parameters to pass to the API */
        params["pin"] = PIN;
        params["session"] = SESSION1;
        params["name"] = strAccount;

        /* Invoke the API */
        ret = APICall("finance/create/account", params);

        /* Check that the result is as we expect it to be */
        REQUIRE(ret.find("result") != ret.end());
        result = ret["result"];

        REQUIRE(result.find("txid") != result.end());
        REQUIRE(result.find("address") != result.end());

        hashAccount.SetBase58(result["address"].get<std::string>());
    }

    /* Successful get for logged in user*/
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["session"] = SESSION1;

        /* Invoke the API */
        ret = APICall("finance/list/accounts", params);

        /* Check that the result is as we expect it to be */
        REQUIRE(ret.find("result") != ret.end());
        result = ret["result"];

        /* Result must be an array */
        REQUIRE(result.is_array());

        /* Must be at least one entry */
        REQUIRE(result.size() > 0);

        json::json account = result[0];

        REQUIRE(account.find("name") != account.end());
        REQUIRE(account.find("address") != account.end());
        REQUIRE(account.find("token") != account.end());
        REQUIRE(account.find("balance") != account.end());
    }
}


TEST_CASE( "Test Finance API - get stakeinfo", "[finance/get/stakeinfo]")
{
    /* Declare variables shared across test cases */
    json::json params;
    json::json ret;
    json::json result;
    json::json error;

    std::string strAccount = "ACCOUNT" +std::to_string(LLC::GetRand());
    TAO::Register::Address hashAccount ;
    
    /* Ensure user is created and logged in for testing */
    InitializeUser(USERNAME1, PASSWORD, PIN, GENESIS1, SESSION1);


    /* Successful get for logged in user*/
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["session"] = SESSION1;

        /* Invoke the API */
        ret = APICall("finance/get/stakeinfo", params);

        /* Check that the result is as we expect it to be */
        REQUIRE(ret.find("result") != ret.end());
        result = ret["result"];

        REQUIRE(result.find("address") != result.end());
        REQUIRE(result.find("balance") != result.end());
        REQUIRE(result.find("stake") != result.end());
        REQUIRE(result.find("trust") != result.end());
        REQUIRE(result.find("staking") != result.end());
        REQUIRE(result.find("stakerate") != result.end());
        REQUIRE(result.find("trustweight") != result.end());
        REQUIRE(result.find("blockweight") != result.end());
        REQUIRE(result.find("stakeweight") != result.end());
    }
}

TEST_CASE( "Test Finance API - set stake", "[finance/set/stake]")
{
    /* NOTE: We can only test failure cases here because you cannot set the stake until after genesis */
    /* Declare variables shared across test cases */
    json::json params;
    json::json ret;
    json::json result;
    json::json error;

    std::string strAccount = "ACCOUNT" +std::to_string(LLC::GetRand());
    TAO::Register::Address hashAccount ;
    
    /* Ensure user is created and logged in for testing */
    InitializeUser(USERNAME1, PASSWORD, PIN, GENESIS1, SESSION1);

    /* Failure case with insufficient balance */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"] = PIN;
        params["session"] = SESSION1;
        params["amount"] = "1000000";

        /* Invoke the API */
        ret = APICall("finance/set/stake", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -76);
    }
}