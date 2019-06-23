#include <LLP/types/apinode.h>

#include <Util/include/json.h>
#include <Util/include/config.h>
#include <Util/include/base64.h>

#include <unit/catch2/catch.hpp>

json::json APICall(std::string strMethod, json::json jsonParams)
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
