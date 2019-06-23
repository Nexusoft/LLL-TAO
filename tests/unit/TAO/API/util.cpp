#include <LLP/types/apinode.h>

#include <LLC/include/random.h>

#include <Util/include/json.h>
#include <Util/include/config.h>
#include <Util/include/base64.h>

#include <unit/catch2/catch.hpp>


/* Generate a random string for the username */
std::string USERNAME1 = LLC::GetRand256().ToString();
std::string USERNAME2 = LLC::GetRand256().ToString();
uint256_t GENESIS1 = 0;
uint256_t GENESIS2 = 0;
std::string PASSWORD = "pw";
std::string PIN = "1234";
std::string SESSION1;
std::string SESSION2;


json::json APICall(const std::string& strMethod, const json::json& jsonParams)
{
    /* HTTP basic authentication for API */
    std::string strUserPass64 = encoding::EncodeBase64(config::mapArgs["-apiuser"] + ":" + config::mapArgs["-apipassword"]);

    /* Parse out the endpoints. */
    std::string::size_type pos = strMethod.find('/');

    /* Build the HTTP Header. */
    std::string strContent = jsonParams.dump();
    std::string strReply = debug::safe_printstr(
            "POST /", strMethod.substr(0, pos), "/", strMethod.substr(pos + 1), " HTTP/1.1\r\n",
            "Date: ", debug::rfc1123Time(), "\r\n",
            "Connection: close\r\n",
            "Content-Length: ", strContent.size(), "\r\n",
            "Content-Type: application/json\r\n",
            "Server: Nexus-JSON-API\r\n",
            "Authorization: Basic ", strUserPass64, "\r\n",
            "\r\n",
            strContent);

    /* Convert the content into a byte buffer. */
    std::vector<uint8_t> vBuffer(strReply.begin(), strReply.end());

    /* Make the connection to the API server. */
    LLP::APINode apiNode;

    LLP::BaseAddress addr("127.0.0.1", 8080);

    if(!apiNode.Connect(addr))
        throw "Couldn't Connect to API";

    /* Write the buffer to the socket. */
    apiNode.Write(vBuffer, vBuffer.size());
    apiNode.Flush();

    /* Read the response packet. */
    while(!apiNode.INCOMING.Complete() && !config::fShutdown.load())
    {

        /* Catch if the connection was closed. */
        if(!apiNode.Connected())
            throw "Connection Terminated";

        /* Catch if the socket experienced errors. */
        if(apiNode.Errors())
            throw "Socket Error";

        /* Catch if the connection timed out. */
        if(apiNode.Timeout(120))
            throw "Socket Timeout";

        /* Read the response packet. */
        apiNode.ReadPacket();
        runtime::sleep(1);
    }

    /* Clean socket disconnect. */
    apiNode.Disconnect();

    /* Parse response JSON. */
    json::json ret = json::json::parse(apiNode.INCOMING.strContent);

    return ret;

}


/* Creates, logs in, and unlocks the specified user */
void InitializeUser(const std::string& strUsername, const std::string& strPassword, const std::string& strPin,
                           uint256_t& hashGenesis, std::string& strSession)
{
    /* Check not already initialized */
    if(hashGenesis != 0)
        return;

    /* Declare variables */
    json::json params;
    json::json ret;
    json::json result;
    json::json error;

    /* Enure that we use low argon2 requirements for unit test to speed up the use of the sig chain */
    config::SoftSetArg("-argon2", "0");
    config::SoftSetArg("-argon2_memory", "0");

    /* Build the parameters to pass to the API */
    params["username"] = strUsername;
    params["password"] = PASSWORD;
    params["pin"] = PIN;

    /* Invoke the API to create the user */
    ret = APICall("users/create/user", params);

    /* Invoke the API to create the user */
    ret = APICall("users/login/user", params);
    result = ret["result"];

    hashGenesis.SetHex(result["genesis"].get<std::string>());

    if(config::fMultiuser)
    {
        /* Grab the session ID for future calls */
        strSession = result["session"].get<std::string>();
    }

    if(!config::fMultiuser)
    {
        /* Build the parameters to pass to the API */
        params["pin"] = PIN;

        /* Invoke the API */
        ret = APICall("users/unlock/user", params);
    }

}


/* Logs out the specified session */
void LogoutUser( uint256_t& hashGenesis, std::string& strSession)
{
    /* Check not already logged out */
    if(hashGenesis == 0)
        return;

    /* Declare variables */
    json::json params;
    json::json ret;
    json::json result;
    json::json error;

    /* Enure that we use low argon2 requirements for unit test to speed up the use of the sig chain */
    config::SoftSetArg("-argon2", "0");
    config::SoftSetArg("-argon2_memory", "0");

    /* Build the parameters to pass to the API */
    params["session"] = strSession;

    /* Invoke the API to logout the user */
    ret = APICall("users/logout/user", params);
    result = ret["result"];

    hashGenesis = 0;
    strSession = "";

}