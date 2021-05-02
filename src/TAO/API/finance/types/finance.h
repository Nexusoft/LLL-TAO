/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once

#include <TAO/API/types/base.h>

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        /** Finance
         *
         *  Finance API Class.
         *  Manages the function pointers for all Finance commands.
         *
         **/
        class Finance : public Base
        {
        public:

            /** Default Constructor. **/
            Finance()
            : Base()
            {
                Initialize();
            }


            /** Initialize.
             *
             *  Sets the function pointers for this API.
             *
             **/
            void Initialize() final;


            /** RewriteURL
             *
             *  Allows derived API's to handle custom/dynamic URL's where the strMethod does not
             *  map directly to a function in the target API.  Insted this method can be overriden to
             *  parse the incoming URL and route to a different/generic method handler,
             *
             *  @param[in] strMethod The name of the method being invoked.
             *  @param[out] jParams The json array of parameters to be modified and passed back.
             *
             *  @return the modified API method URL as a string
             *
             **/
            std::string RewriteURL(const std::string& strMethod, json::json &jParams) final;


            /** GetName
             *
             *  Returns the name of this API.
             *
             **/
            std::string GetName() const final
            {
                return "Finance";
            }


            /** Burn
             *
             *  Burn tokens. This debits the account to send the coins back to the token address, but does so with a contract
             *  condition that always evaluates to false.  Hence the debit can never be credited, burning the tokens.
             *
             *  @param[in] jParams The parameters from the API call.
             *  @param[in] fHelp Trigger for help data.
             *
             *  @return The return object in JSON.
             *
             **/
            json::json Burn(const json::json& jParams, bool fHelp);


            /** Create
             *
             *  Create an account or token object register
             *
             *  @param[in] jParams The parameters from the API call.
             *  @param[in] fHelp Trigger for help data.
             *
             *  @return The return object in JSON.
             *
             **/
            json::json Create(const json::json& jParams, bool fHelp);


            /** Credit
             *
             *  Claim a balance from an account or token object register.
             *
             *  @param[in] jParams The parameters from the API call.
             *  @param[in] fHelp Trigger for help data.
             *
             *  @return The return object in JSON.
             *
             **/
            json::json Credit(const json::json& jParams, bool fHelp);


            /** Debit
             *
             *  Debit tokens from an account or token object register
             *
             *  @param[in] jParams The parameters from the API call.
             *  @param[in] fHelp Trigger for help data.
             *
             *  @return The return object in JSON.
             *
             **/
            json::json Debit(const json::json& jParams, bool fHelp);


            /** Get
             *
             *  Get an account or token object register
             *
             *  @param[in] jParams The parameters from the API call.
             *  @param[in] fHelp Trigger for help data.
             *
             *  @return The return object in JSON.
             *
             **/
            json::json Get(const json::json& jParams, bool fHelp);



            /** GetBalances
             *
             *  Get a summary of balance information across all accounts belonging to the currently logged in signature chain
             *  for a particular token type
             *
             *  @param[in] jParams The parameters from the API call.
             *  @param[in] fHelp Trigger for help data.
             *
             *  @return The return object in JSON.
             *
             **/
            json::json GetBalances(const json::json& jParams, bool fHelp);


            /** ListBalances
             *
             *  Get a summary of balance information across all accounts belonging to the currently logged in signature chain
             *
             *  @param[in] jParams The parameters from the API call.
             *  @param[in] fHelp Trigger for help data.
             *
             *  @return The return object in JSON.
             *
             **/
            json::json ListBalances(const json::json& jParams, bool fHelp);


            /** Info
             *
             *  Get staking metrics for a trust account.
             *
             *  @param[in] jParams The parameters from the API call.
             *  @param[in] fHelp Trigger for help data.
             *
             *  @return The return object in JSON.
             *
             **/
            json::json Info(const json::json& jParams, bool fHelp);


            /** List
             *
             *  List all NXS accounts
             *
             *  @param[in] jParams The parameters from the API call.
             *  @param[in] fHelp Trigger for help data.
             *
             *  @return The return object in JSON.
             *
             **/
            json::json List(const json::json& jParams, bool fHelp);


            /** ListTransactions
             *
             *  Lists all transactions for a given account
             *
             *  @param[in] jParams The parameters from the API call.
             *  @param[in] fHelp Trigger for help data.
             *
             *  @return The return object in JSON.
             *
             **/
            json::json ListTransactions(const json::json& jParams, bool fHelp);


            /** MigrateAccounts
             *
             *  Migrate all Legacy wallet accounts to corresponding accounts in the signature chain.
             *
             *  @param[in] jParams The parameters from the API call.
             *  @param[in] fHelp Trigger for help data.
             *
             *  @return The return object in JSON.
             *
             **/
            json::json MigrateAccounts(const json::json& jParams, bool fHelp);


            /** Stake
             *
             *  Set the stake amount for trust account (add/remove stake).
             *
             *  @param[in] jParams The parameters from the API call.
             *  @param[in] fHelp Trigger for help data.
             *
             *  @return The return object in JSON.
             *
             **/
            json::json Stake(const json::json& jParams, bool fHelp);


            /** TrustAccounts
             *
             *  Lists all trust accounts in the chain
             *
             *  @param[in] jParams The parameters from the API call.
             *  @param[in] fHelp Trigger for help data.
             *
             *  @return The return object in JSON.
             *
             **/
            json::json TrustAccounts(const json::json& jParams, bool fHelp);

        };
    }
}
