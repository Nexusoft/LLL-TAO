/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/include/rpc.h>
#include <Util/include/json.h>
#include <LLP/include/global.h>

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        /* Create the list of commands. */
        TAO::API::RPC* RPCCommands;

        void RPC::Initialize()
        {

            mapFunctions["echo"] = Function(std::bind(&RPC::Echo, this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["help"] = Function(std::bind(&RPC::Help, this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["getinfo"] = Function(std::bind(&RPC::GetInfo, this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["getmininginfo"] = Function(std::bind(&RPC::GetMiningInfo, this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["getconnectioncount"] = Function(std::bind(&RPC::GetConnectionCount, this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["getnewaddress"] = Function(std::bind(&RPC::GetNewAddress, this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["getaccountaddress"] = Function(std::bind(&RPC::GetAccountAddress, this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["setaccount"] = Function(std::bind(&RPC::SetAccount, this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["getaccount"] = Function(std::bind(&RPC::GetAccount, this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["getaddressesbyaccount"] = Function(std::bind(&RPC::GetAddressesByAccount, this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["signmessage"] = Function(std::bind(&RPC::SignMessage, this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["verifymessage"] = Function(std::bind(&RPC::VerifyMessage, this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["getreceivedbyaddress"] = Function(std::bind(&RPC::GetReceivedByAddress, this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["getreceivedbyaccount"] = Function(std::bind(&RPC::GetReceivedByAccount, this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["getbalance"] = Function(std::bind(&RPC::GetBalance, this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["movecmd"] = Function(std::bind(&RPC::MoveCmd, this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["addmultisigaddress"] = Function(std::bind(&RPC::AddMultisigAddress, this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["listreceivedbyaddress"] = Function(std::bind(&RPC::ListReceivedByAddress, this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["listreceivedbyaccount"] = Function(std::bind(&RPC::ListReceivedByAccount, this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["listtransactions"] = Function(std::bind(&RPC::ListTransactions, this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["listaddresses"] = Function(std::bind(&RPC::ListAddresses, this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["listaccounts"] = Function(std::bind(&RPC::ListAccounts, this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["listsinceblock"] = Function(std::bind(&RPC::ListSinceBlock, this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["gettransaction"] = Function(std::bind(&RPC::GetTransaction, this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["getrawtransaction"] = Function(std::bind(&RPC::GetRawTransaction, this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["sendrawtransaction"] = Function(std::bind(&RPC::SendRawTransaction, this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["validateaddress"] = Function(std::bind(&RPC::ValidateAddress, this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["makekeypair"] = Function(std::bind(&RPC::MakeKeyPair, this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["unspentbalance"] = Function(std::bind(&RPC::UnspentBalance, this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["listunspent"] = Function(std::bind(&RPC::ListUnspent, this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["reset"] = Function(std::bind(&RPC::Reset, this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["getpeerinfo"] = Function(std::bind(&RPC::GetPeerInfo, this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["getnetworkhashps"] = Function(std::bind(&RPC::GetNetworkHashps, this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["getnetworkpps"] = Function(std::bind(&RPC::GetNetworkPps, this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["getnetworktrustkeys"] = Function(std::bind(&RPC::GetNetworkTrustKeys, this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["getblockcount"] = Function(std::bind(&RPC::GetBlockCount, this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["getblocknumber"] = Function(std::bind(&RPC::GetBlockNumber, this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["getdifficulty"] = Function(std::bind(&RPC::GetDifficulty, this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["getsupplyrates"] = Function(std::bind(&RPC::GetSupplyRates, this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["getmoneysupply"] = Function(std::bind(&RPC::GetMoneySupply, this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["backupwallet"] = Function(std::bind(&RPC::BackupWallet, this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["checkwallet"] = Function(std::bind(&RPC::CheckWallet, this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["listtrustkeys"] = Function(std::bind(&RPC::ListTrustKeys, this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["repairwallet"] = Function(std::bind(&RPC::RepairWallet, this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["rescan"] = Function(std::bind(&RPC::Rescan, this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["importprivkey"] = Function(std::bind(&RPC::ImportPrivKey, this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["dumpprivkey"] = Function(std::bind(&RPC::DumpPrivKey, this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["importkeys"] = Function(std::bind(&RPC::ImportKeys, this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["exportkeys"] = Function(std::bind(&RPC::ExportKeys, this, std::placeholders::_1, std::placeholders::_2));

        }


        /* Test method to echo back the parameters passed by the caller */
        json::json RPC::Echo(const json::json& jsonParams, bool fHelp)
        {
            if (fHelp || jsonParams.size() == 0)
                return std::string(
                    "echo [param]...[param]"
                    " - Test function that echo's back the parameters supplied in the call.");

            if(jsonParams.size() == 0)
                throw APIException(-11, "Not enough parameters");

            json::json ret;
            ret["echo"] = jsonParams;

            return ret;
        }


        /* Returns help list.  Iterates through all functions in mapFunctions and calls each one with fHelp=true */
        json::json RPC::Help(const json::json& jsonParams, bool fHelp)
        {
            json::json ret;

            if (fHelp || jsonParams.size() > 1)
                return std::string(
                    "help [command]"
                    " - List commands, or get help for a command.");

            std::string strCommand = "";

            if(!jsonParams.is_null() && jsonParams.size() > 0 )
                strCommand = jsonParams.at(0);

            if( strCommand.length() > 0)
            {
                if( mapFunctions.find(strCommand) == mapFunctions.end())
                    throw APIException(-32601, debug::strprintf("Method not found: %s", strCommand.c_str()));
                else
                {
                    ret = mapFunctions[strCommand].Execute(jsonParams, true).get<std::string>();
                }
            }
            else
            {
                // iterate through all registered commands and build help list to return
                std::string strHelp = "";
                for(auto& pairFunctionEntry : mapFunctions)
                {
                    strHelp += pairFunctionEntry.second.Execute(jsonParams, true).get<std::string>() + "\n";
                }

                ret = strHelp;
            }

            return ret;
        }

        uint32_t RPC::GetTotalConnectionCount()
        {
            uint32_t nConnections = 0;

            if(LLP::TRITIUM_SERVER  && LLP::TRITIUM_SERVER->pAddressManager)
                nConnections += LLP::TRITIUM_SERVER->pAddressManager->GetInfoCount(LLP::ConnectState::CONNECTED);

            if(LLP::LEGACY_SERVER && LLP::LEGACY_SERVER->pAddressManager)
                nConnections += LLP::LEGACY_SERVER->pAddressManager->GetInfoCount(LLP::ConnectState::CONNECTED);

            return nConnections;
        }
    }
}
