/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_TAO_API_TYPES_RPC_H
#define NEXUS_TAO_API_TYPES_RPC_H

#include <TAO/API/types/base.h>

namespace TAO::API
{

    /** RPC API Class
     *
     *  Manages the function pointers for all RPC commands.
     *
     **/
    class RPC : public Base
    {
    public:
        RPC() {}

        void Initialize();

        std::string GetName() const
        {
            return "RPC";
        }

        /** RPC API command implementations */

        /** Echo
        *
        *  Test method to echo back the parameters passed by the caller
        *
        *  @param[in] jsonParams Parameters array passed by the caller
        *
        *  @return JSON containing the user supplied parameters array
        *
        **/
        json::json Echo(const json::json& jsonParams, bool fHelp);


        /** Help
        *
        *  Returns help list.  Iterates through all functions in mapFunctions and calls each one with fHelp=true
        *
        *  @param[in] jsonParams Parameters array passed by the caller
        *
        *  @return JSON containing the help list
        *
        **/
        json::json Help( const json::json& jsonParams, bool fHelp);

        json::json GetInfo(const json::json& jsonParams, bool fHelp);
        json::json GetMiningInfo(const json::json& jsonParams, bool fHelp);
        json::json GetConnectionCount(const json::json& jsonParams, bool fHelp);
        json::json GetNewAddress(const json::json& jsonParams, bool fHelp);
        json::json SetAccount(const json::json& jsonParams, bool fHelp);
        json::json GetAccount(const json::json& jsonParams, bool fHelp);
        json::json GetAddressesByAccount(const json::json& jsonParams, bool fHelp);
        json::json SignMessage(const json::json& jsonParams, bool fHelp);
        json::json VerifyMessage(const json::json& jsonParams, bool fHelp);
        json::json GetReceivedByAddress(const json::json& jsonParams, bool fHelp);
        json::json GetReceivedByAccount(const json::json& jsonParams, bool fHelp);
        json::json GetBalance(const json::json& jsonParams, bool fHelp);
        json::json MoveCmd(const json::json& jsonParams, bool fHelp);
        json::json AddMultisigAddress(const json::json& jsonParams, bool fHelp);
        json::json ListReceivedByAddress(const json::json& jsonParams, bool fHelp);
        json::json ListReceivedByAccount(const json::json& jsonParams, bool fHelp);
        json::json ListTransactions(const json::json& jsonParams, bool fHelp);
        json::json ListAddresses(const json::json& jsonParams, bool fHelp);
        json::json ListAccounts(const json::json& jsonParams, bool fHelp);
        json::json ListSinceBlock(const json::json& jsonParams, bool fHelp);
        json::json GetTransaction(const json::json& jsonParams, bool fHelp);
        json::json GetRawTransaction(const json::json& jsonParams, bool fHelp);
        json::json SendRawTransaction(const json::json& jsonParams, bool fHelp);
        json::json ValidateAddress(const json::json& jsonParams, bool fHelp);
        json::json MakeKeyPair(const json::json& jsonParams, bool fHelp);
        json::json UnspentBalance(const json::json& jsonParams, bool fHelp);
        json::json ListUnspent(const json::json& jsonParams, bool fHelp);

        json::json Reset(const json::json& jsonParams, bool fHelp);
        json::json GetPeerInfo(const json::json& jsonParams, bool fHelp);
        json::json GetNetworkHashps(const json::json& jsonParams, bool fHelp);
        json::json GetNetworkPps(const json::json& jsonParams, bool fHelp);
        json::json GetNetworkTrustKeys(const json::json& jsonParams, bool fHelp);
        json::json GetBlockCount(const json::json& jsonParams, bool fHelp);
        json::json GetBlockNumber(const json::json& jsonParams, bool fHelp);
        json::json GetDifficulty(const json::json& jsonParams, bool fHelp);
        json::json GetSupplyRates(const json::json& jsonParams, bool fHelp);
        json::json GetMoneySupply(const json::json& jsonParams, bool fHelp);
        json::json BackupWallet(const json::json& jsonParams, bool fHelp);
        json::json CheckWallet(const json::json& jsonParams, bool fHelp);
        json::json ListTrustKeys(const json::json& jsonParams, bool fHelp);
        json::json RepairWallet(const json::json& jsonParams, bool fHelp);
        json::json Rescan(const json::json& jsonParams, bool fHelp);
        json::json ImportPrivKey(const json::json& jsonParams, bool fHelp);
        json::json DumpPrivKey(const json::json& jsonParams, bool fHelp);
        json::json ImportKeys(const json::json& jsonParams, bool fHelp);
        json::json ExportKeys(const json::json& jsonParams, bool fHelp);

    private:
        uint32_t GetTotalConnectionCount();
    };

    /** The instance of RPC commands. */
    extern RPC* RPCCommands;
}

#endif
