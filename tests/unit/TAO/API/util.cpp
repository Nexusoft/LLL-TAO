#include <LLP/impl/apinode.h>

#include <LLC/include/random.h>

#include <LLD/include/global.h>

#include <TAO/Ledger/types/transaction.h>
#include <TAO/Ledger/include/create.h>
#include <TAO/Ledger/include/constants.h>
#include <TAO/Ledger/include/process.h>

#include <Util/include/json.h>
#include <Util/include/config.h>
#include <Util/include/base64.h>

#include <unit/catch2/catch.hpp>


/* Generate a random string for the username */
std::string USERNAME1 = "USERNAME" +std::to_string(LLC::GetRand());
std::string USERNAME2 = "USERNAME" +std::to_string(LLC::GetRand());
uint256_t GENESIS1 = 0;
uint256_t GENESIS2 = 0;
std::string PASSWORD = "password";
std::string PIN = "1234";
std::string SESSION1;
std::string SESSION2;


/* Generate a random txid for use in unit tests.*/
uint512_t RandomTxid(const uint8_t nType)
{
    uint512_t hashTx = LLC::GetRand512();
    hashTx.SetType(nType);

    return hashTx;
}


/* Generate a block and process for private mode. */
static uint256_t hashGenesis = 0;
bool GenerateBlock()
{
    if(!config::mapArgs.count("-generate"))
        return debug::error("no generate parameters");

    /* Get the account. */
    memory::encrypted_ptr<TAO::Ledger::SignatureChain> user =
        new TAO::Ledger::SignatureChain("generate", config::GetArg("-generate", "").c_str());

    /* Get the genesis ID. */
    if(hashGenesis == 0)
        hashGenesis = user->Genesis();

    /* Check for duplicates in ledger db. */
    TAO::Ledger::Transaction txPrev;
    if(LLD::Ledger->HasGenesis(hashGenesis))
    {
        /* Get the last transaction. */
        uint512_t hashLast;
        if(!LLD::Ledger->ReadLast(hashGenesis, hashLast))
            return debug::error(FUNCTION, "No previous transaction found... closing");

        /* Get previous transaction */
        if(!LLD::Ledger->ReadTx(hashLast, txPrev))
            return debug::error(FUNCTION, "No previous transaction found... closing");

        /* Genesis Transaction. */
        TAO::Ledger::Transaction tx;
        tx.NextHash(user->Generate(txPrev.nSequence + 1, "1234"), txPrev.nNextType);

        /* Check for consistency. */
        if(txPrev.hashNext != tx.hashNext)
            return debug::error(FUNCTION, "Invalid credentials... closing");
    }

    /* Create the block object. */
    runtime::timer TIMER;
    TIMER.Start();

    TAO::Ledger::TritiumBlock block;
    if(!TAO::Ledger::CreateBlock(user, "1234", 3, block))
        return debug::error(FUNCTION, "Failed to create block");

    /* Get the secret from new key. */
    std::vector<uint8_t> vBytes = user->Generate(block.producer.nSequence, "1234").GetBytes();
    LLC::CSecret vchSecret(vBytes.begin(), vBytes.end());

    /* Switch based on signature type. */
    switch(block.producer.nKeyType)
    {
        /* Support for the FALCON signature scheeme. */
        case TAO::Ledger::SIGNATURE::FALCON:
        {
            /* Create the FL Key object. */
            LLC::FLKey key;

            /* Set the secret parameter. */
            if(!key.SetSecret(vchSecret))
                return debug::error(FUNCTION, "Failed to create block");

            /* Generate the signature. */
            if(!block.GenerateSignature(key))
                return debug::error(FUNCTION, "Failed to create signature");

            break;
        }

        /* Support for the BRAINPOOL signature scheme. */
        case TAO::Ledger::SIGNATURE::BRAINPOOL:
        {
            /* Create EC Key object. */
            LLC::ECKey key = LLC::ECKey(LLC::BRAINPOOL_P512_T1, 64);

            /* Set the secret parameter. */
            if(!key.SetSecret(vchSecret, true))
                return debug::error(FUNCTION, "Failed to create block");

            /* Generate the signature. */
            if(!block.GenerateSignature(key))
                return debug::error(FUNCTION, "Failed to create signature");

            break;
        }
    }

    /* Debug output. */
    debug::log(0, FUNCTION, "Private Block CREATED in ", TIMER.ElapsedMilliseconds(), " ms");

    /* Verify the block object. */
    uint8_t nStatus = 0;
    TAO::Ledger::Process(block, nStatus);

    /* Check the statues. */
    return (nStatus & TAO::Ledger::PROCESS::ACCEPTED);
}


encoding::json APICall(const std::string& strMethod, const encoding::json& jsonParams)
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
        if(apiNode.Timeout(3000))
            throw "Socket Timeout";

        /* Read the response packet. */
        apiNode.ReadPacket();
        runtime::sleep(1);
    }

    /* Clean socket disconnect. */
    apiNode.Disconnect();

    /* Parse response JSON. */
    encoding::json ret = encoding::json::parse(apiNode.INCOMING.strContent);

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
    encoding::json params;
    encoding::json ret;
    encoding::json result;
    encoding::json error;

    /* Enure that we use low argon2 requirements for unit test to speed up the use of the sig chain */
    config::SoftSetArg("-argon2", "0");
    config::SoftSetArg("-argon2_memory", "0");

    /* Build the parameters to pass to the API */
    params["username"] = strUsername;
    params["password"] = PASSWORD;
    params["pin"] = PIN;

    /* Invoke the API to create the user */
    ret = APICall("users/create/user", params);
    result = ret["result"];

    hashGenesis.SetHex(result["genesis"].get<std::string>());

    uint512_t txid;
    txid.SetHex(result["hash"].get<std::string>());

    /* Write the genesis to disk so that we have it for later use */
    LLD::Ledger->WriteGenesis(hashGenesis, txid);

    /* Invoke the API to create the user */
    ret = APICall("users/login/user", params);

    result = ret["result"];

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
    encoding::json params;
    encoding::json ret;
    encoding::json result;
    encoding::json error;

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
