/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLP/include/global.h>
#include <LLP/include/manager.h>

#include <Legacy/rpc/types/rpc.h>

#include <Util/include/json.h>
#include <Util/include/convert.h>

/* Global TAO namespace. */
namespace Legacy
{
    /* Initialize the list of commands. */
    void RPC::Initialize()
    {
        mapFunctions["echo"] = TAO::API::Function(std::bind(&RPC::Echo, this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["help"] = TAO::API::Function(std::bind(&RPC::Help, this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["stop"] = TAO::API::Function(std::bind(&RPC::Stop, this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["getinfo"] = TAO::API::Function(std::bind(&RPC::GetInfo, this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["getmininginfo"] = TAO::API::Function(std::bind(&RPC::GetMiningInfo, this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["getconnectioncount"] = TAO::API::Function(std::bind(&RPC::GetConnectionCount, this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["getnewaddress"] = TAO::API::Function(std::bind(&RPC::GetNewAddress, this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["getaccountaddress"] = TAO::API::Function(std::bind(&RPC::GetAccountAddress, this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["setaccount"] = TAO::API::Function(std::bind(&RPC::SetAccount, this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["getaccount"] = TAO::API::Function(std::bind(&RPC::GetAccount, this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["getaddressesbyaccount"] = TAO::API::Function(std::bind(&RPC::GetAddressesByAccount, this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["sendtoaddress"] = TAO::API::Function(std::bind(&RPC::SendToAddress, this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["signmessage"] = TAO::API::Function(std::bind(&RPC::SignMessage, this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["verifymessage"] = TAO::API::Function(std::bind(&RPC::VerifyMessage, this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["getreceivedbyaddress"] = TAO::API::Function(std::bind(&RPC::GetReceivedByAddress, this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["getreceivedbyaccount"] = TAO::API::Function(std::bind(&RPC::GetReceivedByAccount, this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["getbalance"] = TAO::API::Function(std::bind(&RPC::GetBalance, this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["move"] = TAO::API::Function(std::bind(&RPC::MoveCmd, this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["addmultisigaddress"] = TAO::API::Function(std::bind(&RPC::AddMultisigAddress, this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["sendfrom"] = TAO::API::Function(std::bind(&RPC::SendFrom, this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["sendmany"] = TAO::API::Function(std::bind(&RPC::SendMany, this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["listreceivedbyaddress"] = TAO::API::Function(std::bind(&RPC::ListReceivedByAddress, this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["listreceivedbyaccount"] = TAO::API::Function(std::bind(&RPC::ListReceivedByAccount, this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["listtransactions"] = TAO::API::Function(std::bind(&RPC::ListTransactions, this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["listaddresses"] = TAO::API::Function(std::bind(&RPC::ListAddresses, this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["listaccounts"] = TAO::API::Function(std::bind(&RPC::ListAccounts, this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["listsinceblock"] = TAO::API::Function(std::bind(&RPC::ListSinceBlock, this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["getglobaltransaction"] = TAO::API::Function(std::bind(&RPC::GetGlobalTransaction, this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["gettransaction"] = TAO::API::Function(std::bind(&RPC::GetTransaction, this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["getrawtransaction"] = TAO::API::Function(std::bind(&RPC::GetRawTransaction, this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["sendrawtransaction"] = TAO::API::Function(std::bind(&RPC::SendRawTransaction, this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["validateaddress"] = TAO::API::Function(std::bind(&RPC::ValidateAddress, this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["makekeypair"] = TAO::API::Function(std::bind(&RPC::MakeKeyPair, this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["unspentbalance"] = TAO::API::Function(std::bind(&RPC::UnspentBalance, this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["listunspent"] = TAO::API::Function(std::bind(&RPC::ListUnspent, this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["getpeerinfo"] = TAO::API::Function(std::bind(&RPC::GetPeerInfo, this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["getnetworkhashps"] = TAO::API::Function(std::bind(&RPC::GetNetworkHashps, this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["getnetworkpps"] = TAO::API::Function(std::bind(&RPC::GetNetworkPps, this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["getnetworktrustkeys"] = TAO::API::Function(std::bind(&RPC::GetNetworkTrustKeys, this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["getblockcount"] = TAO::API::Function(std::bind(&RPC::GetBlockCount, this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["getblocknumber"] = TAO::API::Function(std::bind(&RPC::GetBlockNumber, this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["getdifficulty"] = TAO::API::Function(std::bind(&RPC::GetDifficulty, this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["getsupplyrates"] = TAO::API::Function(std::bind(&RPC::GetSupplyRates, this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["getmoneysupply"] = TAO::API::Function(std::bind(&RPC::GetMoneySupply, this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["getblockhash"] = TAO::API::Function(std::bind(&RPC::GetBlockHash, this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["isorphan"] = TAO::API::Function(std::bind(&RPC::IsOrphan, this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["getblock"] = TAO::API::Function(std::bind(&RPC::GetBlock, this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["backupwallet"] = TAO::API::Function(std::bind(&RPC::BackupWallet, this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["keypoolrefill"] = TAO::API::Function(std::bind(&RPC::KeypoolRefill, this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["settxfee"]      = TAO::API::Function(std::bind(&RPC::SetTxFee, this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["walletpassphrase"] = TAO::API::Function(std::bind(&RPC::WalletPassphrase, this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["encryptwallet"] = TAO::API::Function(std::bind(&RPC::EncryptWallet, this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["walletpassphrasechange"] = TAO::API::Function(std::bind(&RPC::WalletPassphraseChange, this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["walletlock"] = TAO::API::Function(std::bind(&RPC::WalletLock, this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["checkwallet"] = TAO::API::Function(std::bind(&RPC::CheckWallet, this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["listtrustkeys"] = TAO::API::Function(std::bind(&RPC::ListTrustKeys, this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["repairwallet"] = TAO::API::Function(std::bind(&RPC::RepairWallet, this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["rescan"] = TAO::API::Function(std::bind(&RPC::Rescan, this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["importprivkey"] = TAO::API::Function(std::bind(&RPC::ImportPrivKey, this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["dumpprivkey"] = TAO::API::Function(std::bind(&RPC::DumpPrivKey, this, std::placeholders::_1, std::placeholders::_2));
        //mapFunctions["importkeys"] = TAO::API::Function(std::bind(&RPC::ImportKeys, this, std::placeholders::_1, std::placeholders::_2));
        //mapFunctions["exportkeys"] = TAO::API::Function(std::bind(&RPC::ExportKeys, this, std::placeholders::_1, std::placeholders::_2));

    }



    /* Allows derived API's to check the values in the parameters array for the method being called.
    *  The return json contains the sanitized parameter values, which derived implementations might convert to the correct type
    *  for the method being called */
    encoding::json RPC::SanitizeParams(const std::string& strMethod, const encoding::json& jsonParams)
    {
        encoding::json jsonSanitizedParams = jsonParams;

        int n = jsonSanitizedParams.size();

        //
        // Special case non-string parameter types
        //
        if (strMethod == "setgenerate"            && n > 0) convert::StringValueTo<bool>(jsonSanitizedParams[0]);
        if (strMethod == "dumprichlist"           && n > 0) convert::StringValueTo<int>(jsonSanitizedParams[0]);
        if (strMethod == "setgenerate"            && n > 1) convert::StringValueTo<uint64_t>(jsonSanitizedParams[1]);
        if (strMethod == "sendtoaddress"          && n > 1) convert::StringValueTo<double>(jsonSanitizedParams[1]);
        if (strMethod == "settxfee"               && n > 0) convert::StringValueTo<double>(jsonSanitizedParams[0]);
        if (strMethod == "getreceivedbyaddress"   && n > 1) convert::StringValueTo<uint64_t>(jsonSanitizedParams[1]);
        if (strMethod == "getreceivedbyaccount"   && n > 1) convert::StringValueTo<uint64_t>(jsonSanitizedParams[1]);
        if (strMethod == "listreceivedbyaddress"  && n > 0) convert::StringValueTo<uint64_t>(jsonSanitizedParams[0]);
        if (strMethod == "listreceivedbyaddress"  && n > 1) convert::StringValueTo<bool>(jsonSanitizedParams[1]);
        if (strMethod == "listreceivedbyaccount"  && n > 0) convert::StringValueTo<uint64_t>(jsonSanitizedParams[0]);
        if (strMethod == "listreceivedbyaccount"  && n > 1) convert::StringValueTo<bool>(jsonSanitizedParams[1]);
        if (strMethod == "getbalance"             && n > 1) convert::StringValueTo<uint64_t>(jsonSanitizedParams[1]);
        if (strMethod == "getblockhash"           && n > 0) convert::StringValueTo<uint64_t>(jsonSanitizedParams[0]);
        if (strMethod == "getblock"               && n > 1) convert::StringValueTo<bool>(jsonSanitizedParams[1]);
        if (strMethod == "move"                   && n > 2) convert::StringValueTo<double>(jsonSanitizedParams[2]);
        if (strMethod == "move"                   && n > 3) convert::StringValueTo<uint64_t>(jsonSanitizedParams[3]);
        if (strMethod == "sendfrom"               && n > 2) convert::StringValueTo<double>(jsonSanitizedParams[2]);
        if (strMethod == "sendfrom"               && n > 3) convert::StringValueTo<uint64_t>(jsonSanitizedParams[3]);
        if (strMethod == "listtransactions"       && n > 1) convert::StringValueTo<uint64_t>(jsonSanitizedParams[1]);
        if (strMethod == "listtransactions"       && n > 2) convert::StringValueTo<uint64_t>(jsonSanitizedParams[2]);
        if (strMethod == "totaltransactions"      && n > 0) convert::StringValueTo<bool>(jsonSanitizedParams[0]);
        if (strMethod == "gettransactions"        && n > 1) convert::StringValueTo<int>(jsonSanitizedParams[1]);
        if (strMethod == "gettransactions"        && n > 2) convert::StringValueTo<bool>(jsonSanitizedParams[2]);
        if (strMethod == "listaddresses"          && n > 0) convert::StringValueTo<uint64_t>(jsonSanitizedParams[0]);
        if (strMethod == "listaccounts"           && n > 0) convert::StringValueTo<uint64_t>(jsonSanitizedParams[0]);
        if (strMethod == "listunspent"            && n > 0) convert::StringValueTo<uint64_t>(jsonSanitizedParams[0]);
        if (strMethod == "listunspent"            && n > 1) convert::StringValueTo<uint64_t>(jsonSanitizedParams[1]);
        if (strMethod == "walletpassphrase"       && n > 1) convert::StringValueTo<uint64_t>(jsonSanitizedParams[1]);
        if (strMethod == "walletpassphrase"       && n > 2) convert::StringValueTo<bool>(jsonSanitizedParams[2]);
        if (strMethod == "listsinceblock"         && n > 1) convert::StringValueTo<uint64_t>(jsonSanitizedParams[1]);
        if (strMethod == "sendmany"                && n > 2) convert::StringValueTo<uint64_t>(jsonSanitizedParams[2]);
        if (strMethod == "addmultisigaddress"      && n > 0) convert::StringValueTo<uint64_t>(jsonSanitizedParams[0]);


        return jsonSanitizedParams;
    }


    /* Test method to echo back the parameters passed by the caller */
    encoding::json RPC::Echo(const encoding::json& jsonParams, const bool fHelp)
    {
        if(fHelp || jsonParams.size() == 0)
            return std::string(
                "echo [param]...[param]"
                " - Test function that echo's back the parameters supplied in the call.");

        if(jsonParams.size() == 0)
            throw TAO::API::Exception(-11, "Not enough parameters");

        encoding::json ret;
        ret["echo"] = jsonParams;

        return ret;
    }


    /* Returns help list.  Iterates through all functions in mapFunctions and calls each one with fHelp=true */
    encoding::json RPC::Help(const encoding::json& jsonParams, const bool fHelp)
    {
        encoding::json ret;

        if(fHelp || jsonParams.size() > 1)
            return std::string(
                "help [command] - List commands, or get help for a command.");

        std::string strCommand = "";

        if(!jsonParams.is_null() && jsonParams.size() > 0)
            strCommand = jsonParams.at(0);

        if(strCommand.length() > 0)
        {
            if(mapFunctions.find(strCommand) == mapFunctions.end())
                throw TAO::API::Exception(-32601, debug::safe_printstr("Method not found: ", strCommand));
            else
                ret = mapFunctions[strCommand].Execute(jsonParams, true).get<std::string>();
        }
        else
        {
            // iterate through all registered commands and build help list to return
            std::string strHelp = "";
            for(auto& pairFunctionEntry : mapFunctions)
                strHelp += pairFunctionEntry.second.Execute(jsonParams, true).get<std::string>() + "\n";

            ret = strHelp;
        }

        return ret;
    }

    uint32_t RPC::GetTotalConnectionCount()
    {
        uint32_t nConnections = 0;
        if(LLP::TRITIUM_SERVER)
            nConnections += LLP::TRITIUM_SERVER->GetConnectionCount();

        return nConnections;
    }
}
