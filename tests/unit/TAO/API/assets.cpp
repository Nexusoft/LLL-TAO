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

#include <Util/include/convert.h>
#include <Util/include/base64.h>

/* Declare asset names / hashes in global scope so that we can reuse them for the update/get */
std::string strBasicAsset = "ASSET" +std::to_string(LLC::GetRand());
uint256_t hashBasicAsset = 0;
std::string strRawAsset = "ASSET" +std::to_string(LLC::GetRand());
uint256_t hashRawAsset = 0;
std::string strJSONAsset = "ASSET" +std::to_string(LLC::GetRand());
uint256_t hashJSONAsset = 0;

uint512_t hashTransfer = 0;

TEST_CASE( "Test Assets API - create asset - basic", "[assets/create/asset]")
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
        params["format"] = "basic";

        /* Invoke the API */
        ret = APICall("assets/create/asset", params);

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
        params["format"] = "basic";

        /* Invoke the API */
        ret = APICall("assets/create/asset", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -12);
    }

    /* fail with unsupported format */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["session"] = SESSION1;
        params["pin"] = PIN;
        params["format"] = "notaformat";

        /* Invoke the API */
        ret = APICall("assets/create/asset", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -29);
      
    }

    /* fail with missing fields */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["session"] = SESSION1;
        params["pin"] = PIN;
        params["format"] = "basic";

        /* Invoke the API */
        ret = APICall("assets/create/asset", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -20);
    }

    /* fail with non string field */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["session"] = SESSION1;
        params["pin"] = PIN;
        params["format"] = "basic";
        params["field1"] = 1234;

        /* Invoke the API */
        ret = APICall("assets/create/asset", params);

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
        params["format"] = "basic";
        params["name"] = strBasicAsset;
        
        params["field1"] = "data1";

        /* Invoke the API */
        ret = APICall("assets/create/asset", params);

        /* Check response and validate fields */
        REQUIRE(ret.find("result") != ret.end());
        result = ret["result"];

        REQUIRE(result.find("txid") != result.end());
        REQUIRE(result.find("address") != result.end());

        /* Grab the asset hash for later use */
        hashBasicAsset.SetHex(result["address"].get<std::string>());
    }
}

TEST_CASE( "Test Assets API - create asset - raw", "[assets/create/asset]")
{
    /* Declare variables shared across test cases */
    json::json params;
    json::json ret;
    json::json result;
    json::json error;

    /* Ensure user is created and logged in for testing */
    InitializeUser(USERNAME1, PASSWORD, PIN, GENESIS1, SESSION1);

    /* NOTE: Most of the failure cases are covered by the basic format test case above, so won't be repeated here */

    /* fail with missing data */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["session"] = SESSION1;
        params["pin"] = PIN;
        params["format"] = "raw";

        /* Invoke the API */
        ret = APICall("assets/create/asset", params);

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
        params["format"] = "raw";
        params["data"] = 12345;

        /* Invoke the API */
        ret = APICall("assets/create/asset", params);

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
        params["format"] = "raw";
        params["name"] = strRawAsset;
        
        params["data"] = "rawdata";

        /* Invoke the API */
        ret = APICall("assets/create/asset", params);

        /* Check response and validate fields */
        REQUIRE(ret.find("result") != ret.end());
        result = ret["result"];

        REQUIRE(result.find("txid") != result.end());
        REQUIRE(result.find("address") != result.end());

        /* Grab the asset hash for later use */
        hashRawAsset.SetHex(result["address"].get<std::string>());
    }
}

TEST_CASE( "Test Assets API - create asset - json", "[assets/create/asset]")
{
    /* Declare variables shared across test cases */
    json::json params;
    json::json ret;
    json::json result;
    json::json error;

    /* Ensure user is created and logged in for testing */
    InitializeUser(USERNAME1, PASSWORD, PIN, GENESIS1, SESSION1);

    /* fail with missing json */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["session"] = SESSION1;
        params["pin"] = PIN;
        params["format"] = "JSON";

        /* Invoke the API */
        ret = APICall("assets/create/asset", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -21);
    }

    /* fail with JSON not an array */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["session"] = SESSION1;
        params["pin"] = PIN;
        params["format"] = "JSON";
        params["json"] = "notvalid";

        /* Invoke the API */
        ret = APICall("assets/create/asset", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -22);
    }

    /* fail with JSON definition empty */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["session"] = SESSION1;
        params["pin"] = PIN;
        params["format"] = "JSON";
        params["json"] = json::json::array();

        /* Invoke the API */
        ret = APICall("assets/create/asset", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -28);
    }

    /* test JSON definition  */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["session"] = SESSION1;
        params["pin"] = PIN;
        params["format"] = "JSON";

        json::json jsonAssetDefinition = json::json::array();

        json::json jsonFieldDefinition;

        
        /* Test missing name*/
        {
            jsonFieldDefinition.clear();
            jsonAssetDefinition.clear();

            jsonFieldDefinition["test"] = "asdf";

            jsonAssetDefinition.push_back(jsonFieldDefinition);

            params["json"] = jsonAssetDefinition;

            /* Invoke the API */
            ret = APICall("assets/create/asset", params);

            /* Check response is an error and validate error code */
            REQUIRE(ret.find("error") != ret.end());
            REQUIRE(ret["error"]["code"].get<int32_t>() == -22);
        }

        /* Test missing type*/
        {
            jsonFieldDefinition.clear();
            jsonAssetDefinition.clear();

            jsonFieldDefinition["name"] = "field1";

            jsonAssetDefinition.push_back(jsonFieldDefinition);

            params["json"] = jsonAssetDefinition;

            /* Invoke the API */
            ret = APICall("assets/create/asset", params);

            /* Check response is an error and validate error code */
            REQUIRE(ret.find("error") != ret.end());
            REQUIRE(ret["error"]["code"].get<int32_t>() == -23);
        }

        /* Test invalid type */
        {
            jsonFieldDefinition.clear();
            jsonAssetDefinition.clear();

            jsonFieldDefinition["name"] = "field1";
            jsonFieldDefinition["value"] = "data1";
            jsonFieldDefinition["mutable"] = "true";
            jsonFieldDefinition["type"] = "notatype";

            jsonAssetDefinition.push_back(jsonFieldDefinition);

            params["json"] = jsonAssetDefinition;

            /* Invoke the API */
            ret = APICall("assets/create/asset", params);

            /* Check response is an error and validate error code */
            REQUIRE(ret.find("error") != ret.end());
            REQUIRE(ret["error"]["code"].get<int32_t>() == -154);
        }

        /* Test missing value */
        {
            jsonFieldDefinition.clear();
            jsonAssetDefinition.clear();

            jsonFieldDefinition["name"] = "field1";
            jsonFieldDefinition["type"] = "string";

            jsonAssetDefinition.push_back(jsonFieldDefinition);

            params["json"] = jsonAssetDefinition;

            /* Invoke the API */
            ret = APICall("assets/create/asset", params);

            /* Check response is an error and validate error code */
            REQUIRE(ret.find("error") != ret.end());
            REQUIRE(ret["error"]["code"].get<int32_t>() == -24);
        }

        /* Test missing mutable */
        {
            jsonFieldDefinition.clear();
            jsonAssetDefinition.clear();

            jsonFieldDefinition["name"] = "field1";
            jsonFieldDefinition["type"] = "string";
            jsonFieldDefinition["value"] = "data1";

            jsonAssetDefinition.push_back(jsonFieldDefinition);

            params["json"] = jsonAssetDefinition;

            /* Invoke the API */
            ret = APICall("assets/create/asset", params);

            /* Check response is an error and validate error code */
            REQUIRE(ret.find("error") != ret.end());
            REQUIRE(ret["error"]["code"].get<int32_t>() == -25);
        }

        /* Maxlength too small */
        {
            jsonFieldDefinition.clear();
            jsonAssetDefinition.clear();

            jsonFieldDefinition["name"] = "field1";
            jsonFieldDefinition["type"] = "string";
            jsonFieldDefinition["value"] = "data1";
            jsonFieldDefinition["mutable"] = "true";
            jsonFieldDefinition["maxlength"] = "1";

            jsonAssetDefinition.push_back(jsonFieldDefinition);

            params["json"] = jsonAssetDefinition;

            /* Invoke the API */
            ret = APICall("assets/create/asset", params);

            /* Check response is an error and validate error code */
            REQUIRE(ret.find("error") != ret.end());
            REQUIRE(ret["error"]["code"].get<int32_t>() == -26);
        }
 
    }

    /* Test success case for all field types */
    /* test JSON definition  */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["session"] = SESSION1;
        params["pin"] = PIN;
        params["name"] = strJSONAsset;
        params["format"] = "JSON";

        json::json jsonAssetDefinition = json::json::array();

        json::json jsonFieldDefinition;

        /* Add string */
        jsonFieldDefinition["name"] = "stringfield";
        jsonFieldDefinition["type"] = "string";
        jsonFieldDefinition["value"] = "stringvalue";
        jsonFieldDefinition["mutable"] = "true";
        jsonFieldDefinition["maxlength"] = "20";
        jsonAssetDefinition.push_back(jsonFieldDefinition);

        /* Add bytes */
        jsonFieldDefinition["name"] = "bytesfield";
        jsonFieldDefinition["type"] = "bytes";
        jsonFieldDefinition["value"] = encoding::EncodeBase64("bytesfield");
        jsonFieldDefinition["mutable"] = "true";
        jsonFieldDefinition["maxlength"] = "20";
        jsonAssetDefinition.push_back(jsonFieldDefinition);

        /* Add uint8 */
        jsonFieldDefinition["name"] = "uint8field";
        jsonFieldDefinition["type"] = "uint8";
        jsonFieldDefinition["value"] = "1";
        jsonFieldDefinition["mutable"] = "false";
        jsonAssetDefinition.push_back(jsonFieldDefinition);

        /* Add uint16 */
        jsonFieldDefinition["name"] = "uint16field";
        jsonFieldDefinition["type"] = "uint16";
        jsonFieldDefinition["value"] = "1";
        jsonFieldDefinition["mutable"] = "false";
        jsonAssetDefinition.push_back(jsonFieldDefinition);

        /* Add uint32 */
        jsonFieldDefinition["name"] = "uint32field";
        jsonFieldDefinition["type"] = "uint32";
        jsonFieldDefinition["value"] = "1";
        jsonFieldDefinition["mutable"] = "false";
        jsonAssetDefinition.push_back(jsonFieldDefinition);

        /* Add uint64 */
        jsonFieldDefinition["name"] = "uint64field";
        jsonFieldDefinition["type"] = "uint64";
        jsonFieldDefinition["value"] = "1";
        jsonFieldDefinition["mutable"] = "false";
        jsonAssetDefinition.push_back(jsonFieldDefinition);

        /* Add uint256 */
        jsonFieldDefinition["name"] = "uint256field";
        jsonFieldDefinition["type"] = "uint256";
        jsonFieldDefinition["value"] = "1";
        jsonFieldDefinition["mutable"] = "false";
        jsonAssetDefinition.push_back(jsonFieldDefinition);

        /* Add uint512 */
        jsonFieldDefinition["name"] = "uint512field";
        jsonFieldDefinition["type"] = "uint512";
        jsonFieldDefinition["value"] = "1";
        jsonFieldDefinition["mutable"] = "false";
        jsonAssetDefinition.push_back(jsonFieldDefinition);

        /* Add uint1024 */
        jsonFieldDefinition["name"] = "uint1024field";
        jsonFieldDefinition["type"] = "uint1024";
        jsonFieldDefinition["value"] = "1";
        jsonFieldDefinition["mutable"] = "false";
        jsonAssetDefinition.push_back(jsonFieldDefinition);

        params["json"] = jsonAssetDefinition;

        /* Invoke the API */
        ret = APICall("assets/create/asset", params);

        /* Check the result */
        REQUIRE(ret.find("result") != ret.end());
        result = ret["result"];

        REQUIRE(result.find("txid") != result.end());
        REQUIRE(result.find("address") != result.end());

        /* Grab the address for later tests */
        hashJSONAsset.SetHex(result["address"].get<std::string>());
    }

}


TEST_CASE( "Test Assets API - get asset", "[assets/get/asset]")
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
        ret = APICall("assets/get/asset", params);

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

        /* Invoke the API */
        ret = APICall("assets/get/asset", params);

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
        params["address"] = LLC::GetRand256().GetHex();

        /* Invoke the API */
        ret = APICall("assets/get/asset", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -34);
    }

    /* success case  */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["session"] = SESSION1;
        params["pin"] = PIN;
        params["address"] = hashJSONAsset.GetHex();

        /* Invoke the API */
        ret = APICall("assets/get/asset", params);

        /* Check the result */
        REQUIRE(ret.find("result") != ret.end());
        result = ret["result"];

        /* Check all of the fields */
        REQUIRE(result.find("owner") != result.end());
        REQUIRE(result.find("created") != result.end());
        REQUIRE(result.find("modified") != result.end());
        REQUIRE(result.find("name") != result.end());
        REQUIRE(result.find("address") != result.end());

        /* Check data fields all for this asset */
        REQUIRE(result.find("stringfield") != result.end());
        REQUIRE(result.find("bytesfield") != result.end());
        REQUIRE(result.find("uint8field") != result.end());
        REQUIRE(result.find("uint16field") != result.end());
        REQUIRE(result.find("uint32field") != result.end());
        REQUIRE(result.find("uint64field") != result.end());
        REQUIRE(result.find("uint256field") != result.end());
        REQUIRE(result.find("uint512field") != result.end());
        REQUIRE(result.find("uint1024field") != result.end());
    }

}


TEST_CASE( "Test Assets API - update asset", "[assets/update/asset]")
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
        ret = APICall("assets/update/asset", params);

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

        /* Invoke the API */
        ret = APICall("assets/update/asset", params);

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
        params["address"] = LLC::GetRand256().GetHex();

        /* Invoke the API */
        ret = APICall("assets/update/asset", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -34);
    }

    /* fail with updating a raw asset  */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["session"] = SESSION1;
        params["pin"] = PIN;
        params["address"] = hashRawAsset.GetHex();

        /* Invoke the API */
        ret = APICall("assets/update/asset", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -155);
    }

    /* fail updating an unknown field */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["session"] = SESSION1;
        params["pin"] = PIN;
        params["address"] = hashJSONAsset.GetHex();
        params["notafield"] = "xxxx";

        /* Invoke the API */
        ret = APICall("assets/update/asset", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -156);
    }

    /* fail updating a non-mutable field */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["session"] = SESSION1;
        params["pin"] = PIN;
        params["address"] = hashJSONAsset.GetHex();
        params["uint8field"] = "0";

        /* Invoke the API */
        ret = APICall("assets/update/asset", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -157);
    }

    /* fail value longer than max length */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["session"] = SESSION1;
        params["pin"] = PIN;
        params["address"] = hashJSONAsset.GetHex();
        params["stringfield"] = "0123456789012345678901234567890";

        /* Invoke the API */
        ret = APICall("assets/update/asset", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -158);
    }

    /* fail value not a string */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["session"] = SESSION1;
        params["pin"] = PIN;
        params["address"] = hashJSONAsset.GetHex();
        params["stringfield"] = 1234;

        /* Invoke the API */
        ret = APICall("assets/update/asset", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -159);
    }

    /* Success case */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["session"] = SESSION1;
        params["pin"] = PIN;
        params["address"] = hashJSONAsset.GetHex();
        params["stringfield"] = "newstringdata";

        /* Invoke the API */
        ret = APICall("assets/update/asset", params);

        /* Check the result */
        REQUIRE(ret.find("result") != ret.end());
        result = ret["result"];

        /* Check all of the fields */
        REQUIRE(result.find("txid") != result.end());
        REQUIRE(result.find("address") != result.end());

        /* Retrieve the asset and check the value */
        ret = APICall("assets/get/asset", params);

        /* Check the result */
        REQUIRE(ret.find("result") != ret.end());
        result = ret["result"];

        /* Check all of the fields */
        REQUIRE(result.find("stringfield") != result.end());
        REQUIRE(result["stringfield"].get<std::string>() == "newstringdata");
    }
}


TEST_CASE( "Test Assets API - transfer asset", "[assets/transfer/asset]")
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
        ret = APICall("assets/transfer/asset", params);

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
        ret = APICall("assets/transfer/asset", params);

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
        ret = APICall("assets/transfer/asset", params);

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
        ret = APICall("assets/transfer/asset", params);

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
        ret = APICall("assets/transfer/asset", params);

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
        params["address"] = LLC::GetRand256().GetHex();
        params["destination"] = GENESIS2.GetHex();

        /* Invoke the API */
        ret = APICall("assets/transfer/asset", params);

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
        params["address"] = hashBasicAsset.GetHex();
        params["destination"] = GENESIS2.GetHex();

        /* Invoke the API */
        ret = APICall("assets/transfer/asset", params);
         REQUIRE(ret.find("result") != ret.end());
        result = ret["result"];

        /* Check all of the fields */
        REQUIRE(result.find("txid") != result.end());
        
        /* Grab the transfer txid so that we can use it for a claim */
        hashTransfer.SetHex(result["txid"].get<std::string>());

    }

}

TEST_CASE( "Test Assets API - claim asset", "[assets/claim/asset]")
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
        ret = APICall("assets/claim/asset", params);

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
        ret = APICall("assets/claim/asset", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -99);
    }

    /* Success case */
    {
        uint256_t hashAsset = LLC::GetRand256();

        {
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = GENESIS1;
            tx.nSequence   = 1;
            tx.nTimestamp  = runtime::timestamp();

            //create object
            TAO::Register::Object asset = TAO::Register::CreateAsset();

            // add some data
            asset << std::string("data") << uint8_t(TAO::Register::TYPES::STRING) << std::string("somedata");
            

            //payload
            tx[0] << uint8_t(TAO::Operation::OP::CREATE) << hashAsset << uint8_t(TAO::Register::REGISTER::OBJECT) << asset.GetState();

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //verify the prestates and poststates
            REQUIRE(tx.Verify());

            REQUIRE(LLD::Ledger->WriteTx(tx.GetHash(), tx));

            //commit to disk
            REQUIRE(Execute(tx[0], TAO::Ledger::FLAGS::BLOCK));
        }
        {
            /* Need to create an asset that is persisted to the DB */
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = GENESIS1;
            tx.nSequence   = 2;
            tx.nTimestamp  = runtime::timestamp();
            tx.hashNextTx  = TAO::Ledger::STATE::HEAD;

            //payload
            tx[0] << uint8_t(TAO::Operation::OP::TRANSFER) << hashAsset << GENESIS2 << uint8_t(TAO::Operation::TRANSFER::CLAIM);

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //verify the prestates and poststates
            REQUIRE(tx.Verify());

            //write transaction
            REQUIRE(LLD::Ledger->WriteTx(tx.GetHash(), tx));

            //commit to disk
            REQUIRE(Execute(tx[0], TAO::Ledger::FLAGS::BLOCK));
            

            hashTransfer = tx.GetHash();
        }

        /* Build the parameters to pass to the API */
        params.clear();
        params["session"] = SESSION2;
        params["pin"] = PIN;
        params["txid"] = hashTransfer.GetHex();

        /* Invoke the API */
        ret = APICall("assets/claim/asset", params);
         REQUIRE(ret.find("result") != ret.end());
        result = ret["result"];

        /* Check all of the fields */
        REQUIRE(result.find("txid") != result.end());
        

    }

}