/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include "apicall.h"

#include <LLC/include/random.h>

#include <unit/catch2/catch.hpp>

std::string SESSION;

TEST_CASE( "Test Users API", "[API/users]")
{
    /* Declare variables shared across test cases */
    std::string strUsername;
    json::json params;
    json::json ret;
    json::json result;
    json::json error;
    
    /* Enure that we use low argon2 requirements for unit test to speed up the use of the sig chain */
    config::SoftSetArg("-argon2", "0");
    config::SoftSetArg("-argon2_memory", "0");

    /* Test creating a new user */
    {
        /* Generate a random string for the username */
        strUsername = LLC::GetRand256().ToString();

        /* Build the parameters to pass to the API */
        params["username"] = strUsername;
        params["password"] = "pw";
        params["pin"] = "1234";

        /* Invoke the API */
        ret = APICall("users/create/user", params);

        /* Check that the result is as we expect it to be */
        REQUIRE(ret.find("result") != ret.end());
        result = ret["result"];

        REQUIRE(result.find("genesis") != result.end());
        REQUIRE(result.find("hash") != result.end());
        REQUIRE(result.find("nexthash") != result.end());
        REQUIRE(result.find("prevhash") != result.end());
        REQUIRE(result.find("pubkey") != result.end());
        REQUIRE(result.find("signature") != result.end());
        REQUIRE(result.find("sequence") != result.end());
        REQUIRE(result.find("timestamp") != result.end());
        REQUIRE(result.find("version") != result.end());
    }

    /* Test failing if user already exists */
    {
        /* Params are already populated from the previous call so just invoke the API again */
        ret = APICall("users/create/user", params);

        /* Check that the result is an error */
        REQUIRE(ret.find("error") != ret.end());
    }

    /* Test login failure */
    {
        /* Build the parameters to pass to the API */
        params["username"] = strUsername;
        params["password"] = "wrongpass";
        params["pin"] = "1234";

        /* Invoke the API */
        ret = APICall("users/login/user", params);

        /* Check that the result is as we expect it to be */
        REQUIRE(ret.find("error") != ret.end());

    }

    /* Test login success */
    {
        /* Build the parameters to pass to the API */
        params["username"] = strUsername;
        params["password"] = "pw";
        params["pin"] = "1234";

        /* Invoke the API */
        ret = APICall("users/login/user", params);

        /* Check that the result is as we expect it to be */
        REQUIRE(ret.find("result") != ret.end());
        result = ret["result"];

        REQUIRE(result.find("genesis") != result.end());

        if(config::fMultiuser)
        {
            REQUIRE(result.find("session") != result.end());

            /* Grab the session ID for future calls */
            SESSION = result["session"].get<std::string>();
        }
    }

    /* Test unlock fail wrong pin */
    if(!config::fMultiuser)
    {
        /* Build the parameters to pass to the API */
        params["pin"] = "5678";

        /* Invoke the API */
        ret = APICall("users/unlock/user", params);

        /* Check that the result is as we expect it to be */
        REQUIRE(ret.find("error") != ret.end());
    }

    /* Test unlock */
    if(!config::fMultiuser)
    {
        /* Build the parameters to pass to the API */
        params["pin"] = "1234";

        /* Invoke the API */
        ret = APICall("users/unlock/user", params);

        /* Check that the result is as we expect it to be */
        REQUIRE(ret.find("result") != ret.end());
        result = ret["result"];

        REQUIRE(result.get<bool>() == true);
    }


    /* Test lock */
    if(!config::fMultiuser)
    {
        /* Build the parameters to pass to the API */
        params["pin"] = "1234";

        /* Invoke the API */
        ret = APICall("users/lock/user", params);

        /* Check that the result is as we expect it to be */
        REQUIRE(ret.find("result") != ret.end());
        result = ret["result"];

        REQUIRE(result.get<bool>() == true);
    }


    /* Test logout success */
    {
        /* Build the parameters to pass to the API */
        params["session"] = SESSION;

        /* Invoke the API */
        ret = APICall("users/logout/user", params);

        /* Check that the result is as we expect it to be */
        REQUIRE(ret.find("result") != ret.end());
        result = ret["result"];

        REQUIRE(result.get<bool>() == true);

    }
}