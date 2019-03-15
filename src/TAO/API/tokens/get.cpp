/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/include/tokens.h>

#include <TAO/Register/objects/account.h>
#include <TAO/Register/objects/token.h>

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        /* Get the data from a digital asset */
        json::json Tokens::Get(const json::json& params, bool fHelp)
        {
            json::json ret;

            /* Check for identifier parameter. */
            if(params.find("type") == params.end())
                throw APIException(-25, "Missing Type");

            /* Get the Register ID. */
            uint256_t hashRegister = 0;

            /* Check for data parameter. */
            if(params.find("name") != params.end())
            {
                /* Get the address from the name. */
                std::string strName = GetName() + ":" + params["name"].get<std::string>();

                /* Build the address from an SK256 hash of API:NAME. */
                hashRegister = LLC::SK256(std::vector<uint8_t>(strName.begin(), strName.end()));
            }

            /* Otherwise try to find the raw hex encoded address. */
            else if(params.find("address") != params.end())
                hashRegister.SetHex(params["address"].get<std::string>());

            /* Fail if no required parameters supplied. */
            else
                throw APIException(-23, "Missing memory address");

            /* Get the history. */
            TAO::Register::State state;
            if(!LLD::regDB->ReadState(hashRegister, state))
                throw APIException(-24, "No state found");

            /* Build the response JSON. */
            if(params["type"].get<std::string>() == "account")
            {
                /* Create a token object. */
                TAO::Register::Account account;

                /* De-Serialie the account.. */
                state >> account;

                /* Build response. */
                ret["version"]    = account.nVersion;
                ret["identifier"] = account.nIdentifier;
                ret["balance"]    = account.nBalance;
            }
            else if(params["type"].get<std::string>() == "token")
            {
                /* Check for supply parameter. */
                if(params.find("supply") == params.end())
                    throw APIException(-25, "Missing Supply");

                /* Create a token object. */
                TAO::Register::Token token;

                /* De-Serialie the token. */
                state >> token;

                /* Build response. */
                ret["version"]          = token.nVersion;
                ret["identifier"]       = token.nIdentifier;
                ret["maxsupply"]        = token.nMaxSupply;
                ret["currentsupply"]    = token.nCurrentSupply;
            }
            else
                throw APIException(-27, "Unknown object register");

            return ret;
        }
    }
}
