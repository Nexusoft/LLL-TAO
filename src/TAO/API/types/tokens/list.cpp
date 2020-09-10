/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/types/finance.h>
#include <TAO/API/types/objects.h>
#include <TAO/API/include/global.h>

#include <TAO/API/include/utils.h>
#include <TAO/API/include/json.h>

#include <TAO/Ledger/types/sigchain.h>

#include <TAO/Operation/include/enum.h>

#include <TAO/Register/types/object.h>

#include <Util/include/debug.h>


/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        /* Get a list of accounts owned by a signature chain. */
        json::json Tokens::ListAccounts(const json::json& params, bool fHelp)
        {
            /* JSON return value. */
            json::json ret;// = json::json::array();

            /* Get the session to be used for this API call */
            Session& session = users->GetSession(params);

            /* Get the account. */
            const memory::encrypted_ptr<TAO::Ledger::SignatureChain>& user = session.GetAccount();
            if(!user)
                throw APIException(-10, "Invalid session ID");

            /* Check for paged parameter. */
            uint32_t nPage = 0;
            if(params.find("page") != params.end())
                nPage = std::stoul(params["page"].get<std::string>());

            /* Check for limit parameter. */
            uint32_t nLimit = 100;
            if(params.find("limit") != params.end())
                nLimit = std::stoul(params["limit"].get<std::string>());

            /* Get the list of registers owned by this sig chain */
            std::vector<TAO::Register::Address> vAddresses;
            if(!TAO::API::ListAccounts(user->Genesis(), vAddresses, false, false))
                throw APIException(-74, "No registers found");

            /* Read all the registers to that they are sorted by creation time */
            std::vector<std::pair<TAO::Register::Address, TAO::Register::State>> vAccounts;
            GetRegisters(vAddresses, vAccounts);

            /* Add the register data to the response */
            uint32_t nTotal = 0;
            for(const auto& state : vAccounts)
            {
                /* Double check that it is an object before we cast it */
                if(state.second.nType != TAO::Register::REGISTER::OBJECT)
                    continue;

                /* Cast the state to an Object register */
                TAO::Register::Object object(state.second);

                /* Check that this is a non-standard object type so that we can parse it and check the type*/
                if(object.nType != TAO::Register::REGISTER::OBJECT)
                    continue;

                /* parse object so that the data fields can be accessed */
                if(!object.Parse())
                    throw APIException(-36, "Failed to parse object register");

                /* Check that this is an account */
                uint8_t nStandard = object.Standard();
                if(nStandard != TAO::Register::OBJECTS::ACCOUNT )
                    continue;

                /* Check the account is not a NXS account */
                if(object.get<uint256_t>("token") == 0)
                    continue;

                /* Get the current page. */
                uint32_t nCurrentPage = nTotal / nLimit;

                ++nTotal;

                /* Check the paged data. */
                if(nCurrentPage < nPage)
                    continue;

                if(nCurrentPage > nPage)
                    break;

                if(nTotal - (nPage * nLimit) > nLimit)
                    break;

                /* Convert the object to JSON */
                ret.push_back(TAO::API::ObjectToJSON(params, object, state.first));
            }

            return ret;
        }

        /* Lists all transactions for a given account. */
        json::json Tokens::ListTransactions(const json::json& params, bool fHelp)
        {
            /* The account to list transactions for. */
            TAO::Register::Address hashAccount;

            /* If name is provided then use this to deduce the register address,
             * otherwise try to find the raw hex encoded address. */
            if(params.find("name") != params.end() && !params["name"].get<std::string>().empty())
                hashAccount = Names::ResolveAddress(params, params["name"].get<std::string>());
            else if(params.find("address") != params.end())
                hashAccount.SetBase58(params["address"].get<std::string>());
            else
                throw APIException(-33, "Missing name or address");

            /* the type to list transactions for - should be "token" or "account" */
            std::string strType = params.find("type") != params.end() ? params["type"].get<std::string>() : "";

            /* Get the account object. */
            TAO::Register::Object object;
            if(!LLD::Register->ReadState(hashAccount, object, TAO::Ledger::FLAGS::MEMPOOL))
            {
                if(strType == "account")
                    throw APIException(-13, "Account not found");
                else if(strType == "token")
                    throw APIException(-125, "Token not found");
                else
                    throw APIException(-104, "Object not found");
            }

            /* Parse the object register. */
            if(!object.Parse())
                throw APIException(-14, "Object failed to parse");

            /* Get the object standard. */
            uint8_t nStandard = object.Standard();

            /* Check the object standard. */
            if(nStandard == TAO::Register::OBJECTS::TOKEN
            || nStandard == TAO::Register::OBJECTS::ACCOUNT
            || nStandard == TAO::Register::OBJECTS::TRUST)
            {
                /* If the user requested a particular object type then check it is that type */
                if(strType == "token" && (nStandard == TAO::Register::OBJECTS::ACCOUNT || nStandard == TAO::Register::OBJECTS::TRUST))
                    throw APIException(-123, "Object is not a token");
                else if(strType == "account" && nStandard == TAO::Register::OBJECTS::TOKEN)
                    throw APIException(-65, "Object is not an account");

                /* Check the account is not a NXS account */
                if(strType == "account" && object.get<uint256_t>("token") == 0)
                    throw APIException(-166, "Account is a NXS account.  Please use the finance API for accessing NXS accounts.");

            }
            else
            {
                throw APIException(-124, "Unknown token / account.");
            }

            

            return Objects::ListTransactions(params, fHelp);
        }


        /* Lists all accounts that have been created for a particular token. */
        json::json Tokens::ListTokenAccounts(const json::json& params, bool fHelp)
        {
            /* The return json array */
            json::json jsonRet = json::json::array();

            /* Check they are not using client mode */
            if(config::fClient.load() )
                throw APIException(-300, "API can only be used to lookup data for the currently logged in signature chain when running in client mode");

            /* The token to list accounts for. */
            TAO::Register::Address hashToken;

            /* If name is provided then use this to deduce the register address of the token,
             * otherwise try to find the raw hex encoded address. */
            if(params.find("name") != params.end() && !params["name"].get<std::string>().empty())
                hashToken = Names::ResolveAddress(params, params["name"].get<std::string>());
            else if(params.find("address") != params.end())
                hashToken.SetBase58(params["address"].get<std::string>());
            else
                throw APIException(-33, "Missing name or address");

            /* Get the token  */
            TAO::Register::Object object;
            if(!LLD::Register->ReadState(hashToken, object, TAO::Ledger::FLAGS::LOOKUP))
                throw APIException(-125, "Token not found");

            /* Parse the object register. */
            if(!object.Parse())
                throw APIException(-14, "Object failed to parse");

            /* Get the object standard. */
            uint8_t nStandard = object.Standard();

            /* Check it is a token. */
            if(nStandard != TAO::Register::OBJECTS::TOKEN)
                throw APIException(-123, "Object is not a token");

            /* Check for page parameter. */
            uint32_t nPage = 0;
            if(params.find("page") != params.end())
                nPage = std::stoul(params["page"].get<std::string>());

            /* Get the limit parameter, default to 100. */
            uint32_t nLimit = 100;
            if(params.find("limit") != params.end())
                nLimit = std::stoul(params["limit"].get<std::string>());


            /* The process below for building up the list of token accounts might seem convoluted but there is a reason for the 
               long-winded process.  We can batch read accounts from the register database to obtain all accounts and then filter
               them by the token being filtered on, but unfortunately this by itself does not give us the register address of each
               account.  To get that, we instead use this initial list of accounts to build up a list of the account owners sig
               chain genesis hashes, and then we scan each of those sig chains to find the registers they own.
               This way we can obtain the register address */

            /* The vector of all accounts in the database*/
            std::vector<TAO::Register::Object> vAccounts;

            /* Vector of all account owners and the registers that we are looking for*/
            std::map<uint256_t, std::vector<TAO::Register::Object>> vAccountsByGenesis;

            /* The vector of token accounts for the token being filtered*/
            std::vector<std::pair<TAO::Register::Address, TAO::Register::Object>> vTokenAccounts;

            /* Batch read up to 100,000 */
            if(LLD::Register->BatchRead("account", vAccounts, 100000))
            {
                /* Check that the account belongs to the token being filtered on */
                for(auto& account : vAccounts)
                {
                    /* Parse so we can access the data */
                    account.Parse();

                    /* Only include accounts for the token being searched for */
                    if(account.get<uint256_t>("token") != hashToken)
                        continue;

                    /* Add the account owner and register to our list */
                    vAccountsByGenesis[account.hashOwner].push_back(account);
                }
            }

            /* Now we need to iterate the list of owners and find all of their registers */
            for(const auto& accountsByGenesis : vAccountsByGenesis)
            {
                /* Number of accounts found for this sig chain.  We track this so that once we have found this many, 
                   we can break out rather than searching the entire sig chain history */
                uint16_t nFound = 0;

                /* Get the last transaction. */
                uint512_t hashLast = 0;

                /* Get the last transaction for this genesis.  NOTE that we include the mempool here as there may be registers that
                have been created recently but not yet included in a block*/
                if(!LLD::Ledger->ReadLast(accountsByGenesis.first, hashLast, TAO::Ledger::FLAGS::MEMPOOL))
                    throw APIException(-108, "Failed to read transaction");

                /* The previous hash in the chain */
                uint512_t hashPrev = hashLast;

                 /* Loop until genesis. */
                while(hashPrev != 0)
                {
                    /* Get the transaction from disk. */
                    TAO::Ledger::Transaction tx;
                    if(!LLD::Ledger->ReadTx(hashPrev, tx, TAO::Ledger::FLAGS::MEMPOOL))
                        throw APIException(-108, "Failed to read transaction");

                    /* Set the next last. */
                    hashPrev = !tx.IsFirst() ? tx.hashPrevTx : 0;

                    /* Iterate through all contracts. */
                    for(uint32_t nContract = 0; nContract < tx.Size(); ++nContract)
                    {
                        /* Get the contract output. */
                        const TAO::Operation::Contract& contract = tx[nContract];

                        /* Reset all streams */
                        contract.Reset();

                        /* Seek the contract operation stream to the position of the primitive. */
                        contract.SeekToPrimitive();

                        /* Deserialize the OP. */
                        uint8_t nOP = 0;
                        contract >> nOP;

                        /* The address of the register we are searching for */
                        TAO::Register::Address hashAddress;

                        /* To get th address we check the opcode and then deserialize it from the op stream. Since we are searching 
                           for token accounts, we know that the last operation must be a create, debit, or credit */
                        switch(nOP)
                        {

                            /* These are the register-based operations that prove ownership if encountered before a transfer*/
                            case TAO::Operation::OP::CREATE:
                            case TAO::Operation::OP::DEBIT:
                            {
                                /* Extract the address from the contract. */
                                contract >> hashAddress;

                                break;
                            }


                            /* Check for credits here. */
                            case TAO::Operation::OP::CREDIT:
                            {
                                /* Seek past irrelevant data. */
                                contract.Seek(68);

                                /* The account that is being credited. */
                                contract >> hashAddress;

                                break;
                            }

                            default:
                                continue;

                        }

                        /* Get the register checksum hash from the end of the register stream (last 8 bytes as it is a uint64_t).  
                           This is hash of post-state register which we can then check against the register hashes we are 
                           searching for, for this genesis */
                        contract.Seek(8, TAO::Operation::Contract::REGISTERS, STREAM::END);

                        /* Deserialize the register checksum */
                        uint64_t hashChecksum;
                        contract >>= hashChecksum;

                        /* Check to see if this is one of the registers we are searching for */
                        for(const auto& account : accountsByGenesis.second)
                        {
                            if(account.GetHash() == hashChecksum)
                            {
                                /* Add it to the final list of token accounts to return */
                                vTokenAccounts.push_back(std::make_pair(hashAddress, account));

                                /* Increment the number we have found for this sig chain and break out */
                                nFound++;
                                break;
                            }
                        }
                    }

                    /* If we have found all of the accounts for this user then stop searching their sig chain */
                    if(nFound >= accountsByGenesis.second.size())
                        break;

                }  
            }

            /* Iterate the list and build the response */
            uint32_t nTotal = 0;
            for(const auto& account : vTokenAccounts)
            {
                /* Get the current page. */
                uint32_t nCurrentPage = (nTotal / nLimit) ;

                /* Increment the counter */
                ++nTotal;

                /* Check the paged data. */
                if(nCurrentPage < nPage)
                    continue;

                if(nCurrentPage > nPage)
                    break;

                /* The JSON for this account */
                json::json jsonAccount;
                jsonAccount["owner"]    = account.second.hashOwner.ToString();
                jsonAccount["created"]  = account.second.nCreated;
                jsonAccount["modified"] = account.second.nModified;
                jsonAccount["address"]  = account.first.ToString();

                /* Handle digit conversion. */
                uint8_t nDecimals = GetDecimals(account.second);

                /* Add the balance.  This is the last known confirmed balance, and for this API method does not include any
                   unconfirmed outgoing debits. */
                jsonAccount["balance"] = account.second.get<uint64_t>("balance") / pow(10, nDecimals);

                jsonRet.push_back(jsonAccount);

                if(nTotal - (nPage * nLimit) > nLimit)
                    break;
            }

            return jsonRet;

        }
    }
}
