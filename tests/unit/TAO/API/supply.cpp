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

#include <TAO/Operation/include/enum.h>
#include <TAO/Operation/include/execute.h>

#include <TAO/Register/include/create.h>

#include <TAO/API/types/names.h>
#include <TAO/API/include/global.h>

#include <Util/include/convert.h>
#include <Util/include/base64.h>

/* Declare item names / hashes in global scope so that we can reuse them for the update/get */
std::string strItem = "ITEM" +std::to_string(LLC::GetRand());
TAO::Register::Address hashItem ;
uint512_t hashItemTransfer = 0;



TEST_CASE( "Test Supply API - create item", "[supply/create/item]")
{
    /* Declare variables shared across test cases */
    json::json params;
    json::json ret;
    json::json result;
    json::json error;

    /* Ensure user is created and logged in for testing */
    InitializeUser(USERNAME1, PASSWORD, PIN, GENESIS1, SESSION1);

    /*  fail with missing pin (only relevant for multiuser)*/
    if(config::fMultiuser.load())
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["session"] = SESSION1;

        /* Invoke the API */
        ret = APICall("supply/create/item", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -129);
    }

    /* fail with missing session (only relevant for multiuser)*/
    if(config::fMultiuser.load())
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"] = PIN;

        /* Invoke the API */
        ret = APICall("supply/create/item", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -12);
    }

    /* fail with missing data */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["session"] = SESSION1;
        params["pin"] = PIN;

        /* Invoke the API */
        ret = APICall("supply/create/item", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -18);
    }

    /* fail with data not in string format */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["session"] = SESSION1;
        params["pin"] = PIN;
        params["data"] = 12345;

        /* Invoke the API */
        ret = APICall("supply/create/item", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -19);
    }

    /* success case */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["session"] = SESSION1;
        params["pin"] = PIN;
        params["name"] = strItem;

        params["data"] = "theitemdata";

        /* Invoke the API */
        ret = APICall("supply/create/item", params);

        /* Check response and validate fields */
        REQUIRE(ret.find("result") != ret.end());
        result = ret["result"];

        REQUIRE(result.find("txid") != result.end());
        REQUIRE(result.find("address") != result.end());

        /* Grab the item hash for later use */
        hashItem.SetBase58(result["address"].get<std::string>());
    }
}


TEST_CASE( "Test Supply API - get item", "[supply/get/item]")
{
    /* Declare variables shared across test cases */
    json::json params;
    json::json ret;
    json::json result;
    json::json error;

    /* Ensure user is created and logged in for testing */
    InitializeUser(USERNAME1, PASSWORD, PIN, GENESIS1, SESSION1);

    /* fail with missing name / address */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["session"] = SESSION1;
        params["pin"] = PIN;

        /* Invoke the API */
        ret = APICall("supply/get/item", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -33);
    }

    /* fail with invalid name  */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["session"] = SESSION1;
        params["pin"] = PIN;
        params["name"] = "invalid name";

        /* Invoke the API */
        ret = APICall("supply/get/item", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -101);
    }

    /* fail with invalid address  */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["session"] = SESSION1;
        params["pin"] = PIN;
        params["address"] = TAO::Register::Address(TAO::Register::Address::RAW).ToString();

        /* Invoke the API */
        ret = APICall("supply/get/item", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -117);
    }

    /* success case  */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["session"] = SESSION1;
        params["pin"] = PIN;
        params["address"] = hashItem.ToString();

        /* Invoke the API */
        ret = APICall("supply/get/item", params);

        /* Check the result */
        REQUIRE(ret.find("result") != ret.end());
        result = ret["result"];

        /* Check all of the fields */
        REQUIRE(result.find("owner") != result.end());
        REQUIRE(result.find("created") != result.end());
        REQUIRE(result.find("modified") != result.end());
        REQUIRE(result.find("name") != result.end());
        REQUIRE(result.find("address") != result.end());

        /* Check data fields all for this item */
        REQUIRE(result.find("data") != result.end());
        REQUIRE(result["data"].get<std::string>() == "theitemdata");
    }

}


TEST_CASE( "Test Supply API - update item", "[supply/update/item]")
{
    /* Declare variables shared across test cases */
    json::json params;
    json::json ret;
    json::json result;
    json::json error;

    /* Ensure user is created and logged in for testing */
    InitializeUser(USERNAME1, PASSWORD, PIN, GENESIS1, SESSION1);

    /* fail with missing data  */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["session"] = SESSION1;
        params["pin"] = PIN;
        params["address"] = hashItem.ToString();

        /* Invoke the API */
        ret = APICall("supply/update/item", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -18);
    }

    /* fail with data not a string  */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["session"] = SESSION1;
        params["pin"] = PIN;
        params["address"] = hashItem.ToString();
        params["data"] = 12345;

        /* Invoke the API */
        ret = APICall("supply/update/item", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -19);
    }

    /* fail with missing name / address */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["session"] = SESSION1;
        params["pin"] = PIN;
        params["data"] = "newitemdata";

        /* Invoke the API */
        ret = APICall("supply/update/item", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -33);
    }

    /* fail with missing invalid name  */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["session"] = SESSION1;
        params["pin"] = PIN;
        params["name"] = "invalid name";
        params["data"] = "newitemdata";

        /* Invoke the API */
        ret = APICall("supply/update/item", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -101);
    }

    /* fail with missing invalid address  */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["session"] = SESSION1;
        params["pin"] = PIN;
        params["address"] = TAO::Register::Address(TAO::Register::Address::RAW).ToString();
        params["data"] = "newitemdata";

        /* Invoke the API */
        ret = APICall("supply/update/item", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -117);
    }

    /* Success case */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["session"] = SESSION1;
        params["pin"] = PIN;
        params["address"] = hashItem.ToString();
        params["data"] = "newitemdata";

        /* Invoke the API */
        ret = APICall("supply/update/item", params);

        /* Check the result */
        REQUIRE(ret.find("result") != ret.end());
        result = ret["result"];

        /* Check all of the fields */
        REQUIRE(result.find("txid") != result.end());
        REQUIRE(result.find("address") != result.end());

        /* Retrieve the item and check the value */
        ret = APICall("supply/get/item", params);

        /* Check the result */
        REQUIRE(ret.find("result") != ret.end());
        result = ret["result"];

        /* Check all of the fields */
        REQUIRE(result.find("data") != result.end());
        REQUIRE(result["data"].get<std::string>() == "newitemdata");
    }
}


TEST_CASE( "Test Assets API - transfer item", "[supply/transfer/item]")
{
    /* Declare variables shared across test cases */
    json::json params;
    json::json ret;
    json::json result;
    json::json error;

    /* Ensure user is created and logged in for testing */
    InitializeUser(USERNAME1, PASSWORD, PIN, GENESIS1, SESSION1);
    InitializeUser(USERNAME2, PASSWORD, PIN, GENESIS2, SESSION2);

    /* fail with missing username / destination */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["session"] = SESSION1;
        params["pin"] = PIN;

        /* Invoke the API */
        ret = APICall("supply/transfer/item", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -112);
    }

    /* fail with invalid destination */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["session"] = SESSION1;
        params["pin"] = PIN;
        params["destination"] = LLC::GetRand256().GetHex();

        /* Invoke the API */
        ret = APICall("supply/transfer/item", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -113);
    }

    /* fail with invalid username */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["session"] = SESSION1;
        params["pin"] = PIN;
        params["username"] = "not a user";

        /* Invoke the API */
        ret = APICall("supply/transfer/item", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -113);
    }

    /* fail with missing name / address */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["session"] = SESSION1;
        params["pin"] = PIN;
        params["destination"] = GENESIS2.GetHex();

        /* Invoke the API */
        ret = APICall("supply/transfer/item", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -33);
    }

    /* fail with invalid name  */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["session"] = SESSION1;
        params["pin"] = PIN;
        params["name"] = "invalid name";
        params["destination"] = GENESIS2.GetHex();

        /* Invoke the API */
        ret = APICall("supply/transfer/item", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -101);
    }

    /* fail with invalid address  */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["session"] = SESSION1;
        params["pin"] = PIN;
        params["address"] = TAO::Register::Address(TAO::Register::Address::RAW).ToString();
        params["destination"] = GENESIS2.GetHex();

        /* Invoke the API */
        ret = APICall("supply/transfer/item", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -114);
    }

    /* Success case */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["session"] = SESSION1;
        params["pin"] = PIN;
        params["address"] = hashItem.ToString();
        params["destination"] = GENESIS2.GetHex();

        /* Invoke the API */
        ret = APICall("supply/transfer/item", params);
         REQUIRE(ret.find("result") != ret.end());
        result = ret["result"];

        /* Check all of the fields */
        REQUIRE(result.find("txid") != result.end());

        /* Grab the transfer txid so that we can use it for a claim */
        hashItemTransfer.SetHex(result["txid"].get<std::string>());

    }

}

TEST_CASE( "Test Assets API - claim item", "[supply/claim/item]")
{
    /* Declare variables shared across test cases */
    json::json params;
    json::json ret;
    json::json result;
    json::json error;

    /* Ensure user is created and logged in for testing */
    InitializeUser(USERNAME1, PASSWORD, PIN, GENESIS1, SESSION1);
    InitializeUser(USERNAME2, PASSWORD, PIN, GENESIS2, SESSION2);

    /* fail with missing txid */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["session"] = SESSION2;
        params["pin"] = PIN;

        /* Invoke the API */
        ret = APICall("supply/claim/item", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -50);
    }

    /* fail with invalid txid */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["session"] = SESSION2;
        params["pin"] = PIN;
        params["txid"] = LLC::GetRand512().GetHex();

        /* Invoke the API */
        ret = APICall("supply/claim/item", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -99);
    }

    /* Success case */
    {
        hashItem = TAO::Register::Address(TAO::Register::Address::APPEND);

        {
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = GENESIS1;
            tx.nSequence   = 0;
            tx.nTimestamp  = runtime::timestamp();

            DataStream ssData(SER_REGISTER, 1);

            /* Then the raw data */
            ssData << std::string("itemdata");

            /* Submit the payload object. */
            tx[0] << (uint8_t)TAO::Operation::OP::CREATE << hashItem << (uint8_t)TAO::Register::REGISTER::APPEND << ssData.Bytes();

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //verify the prestates and poststates
            REQUIRE(tx.Verify());

            REQUIRE(LLD::Ledger->WriteTx(tx.GetHash(), tx));

            //commit to disk
            REQUIRE(Execute(tx[0], TAO::Ledger::FLAGS::BLOCK));
        }
        {
            /* Need to create an item that is persisted to the DB */
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = GENESIS1;
            tx.nSequence   = 2;
            tx.nTimestamp  = runtime::timestamp();
            tx.hashNextTx  = TAO::Ledger::STATE::HEAD;

            //payload
            tx[0] << uint8_t(TAO::Operation::OP::TRANSFER) << hashItem << GENESIS2 << uint8_t(TAO::Operation::TRANSFER::CLAIM);

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //verify the prestates and poststates
            REQUIRE(tx.Verify());

            //write transaction
            REQUIRE(LLD::Ledger->WriteTx(tx.GetHash(), tx));

            //commit to disk
            REQUIRE(Execute(tx[0], TAO::Ledger::FLAGS::BLOCK));


            hashItemTransfer = tx.GetHash();
        }

        /* Build the parameters to pass to the API */
        params.clear();
        params["session"] = SESSION2;
        params["pin"] = PIN;
        params["txid"] = hashItemTransfer.GetHex();

        /* Invoke the API */
        ret = APICall("supply/claim/item", params);
        REQUIRE(ret.find("result") != ret.end());
        result = ret["result"];

        /* Check all of the fields */
        REQUIRE(result.find("txid") != result.end());


    }

}

TEST_CASE( "Test Assets API - list item history", "[supply/list/item/history]")
{
    /* Declare variables shared across test cases */
    json::json params;
    json::json ret;
    json::json result;
    json::json error;

    /* Ensure user is created and logged in for testing */
    InitializeUser(USERNAME1, PASSWORD, PIN, GENESIS1, SESSION1);
    InitializeUser(USERNAME2, PASSWORD, PIN, GENESIS2, SESSION2);

    /* fail with missing name / address */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["session"] = SESSION2;

        /* Invoke the API */
        ret = APICall("supply/list/item/history", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -33);
    }

    /* fail with invalid name  */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["session"] = SESSION2;
        params["name"] = "not an item";

        /* Invoke the API */
        ret = APICall("supply/list/item/history", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -101);
    }

    /* fail with invalid name  */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["session"] = SESSION2;
        params["address"] = TAO::Register::Address(TAO::Register::Address::RAW).ToString();

        /* Invoke the API */
        ret = APICall("supply/list/item/history", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -106);
    }

    /* Add an update to the item so that we have each of the create/update/transfer/claim operations in the history */
    {
        params.clear();
        params["session"] = SESSION2;
        params["pin"] = PIN;
        params["address"] = hashItem.ToString();
        params["data"] = "newitemdata";

        /* Invoke the API */
        ret = APICall("supply/update/item", params);

        /* Check the result */
        REQUIRE(ret.find("result") != ret.end());
        result = ret["result"];
    }

    /* success case by address */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["session"] = SESSION2;
        params["address"] = hashItem.ToString();

        /* Invoke the API */
        ret = APICall("supply/list/item/history", params);

        /* Check response  */
        REQUIRE(ret.find("result") != ret.end());
        result = ret["result"];

        /* history should be an array */
        REQUIRE(result.is_array());

        /* Check all of the fields */
        for( const auto& entry : result)
        {
            REQUIRE(entry.find("type") != entry.end());
            std::string strType = entry["type"].get<std::string>();
            REQUIRE((strType == "CREATE" || strType == "MODIFY" || strType == "TRANSFER" || strType == "CLAIM"));
            REQUIRE(entry.find("owner") != entry.end());
            REQUIRE(entry.find("modified") != entry.end());
            REQUIRE(entry.find("checksum") != entry.end());
            REQUIRE(entry.find("address") != entry.end());
            REQUIRE(entry.find("data") != entry.end());

            /* Check the data at each point */
            std::string strData = entry["data"].get<std::string>();

            if(strType == "CREATE" || strType == "TRANSFER" || strType == "CLAIM")
            {
                REQUIRE(strData == "itemdata");
            }
            else if(strType == "MODIFY")
            {
                REQUIRE(strData == "newitemdata");
            }
        }

    }

}
