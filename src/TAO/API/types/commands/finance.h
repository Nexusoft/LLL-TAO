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
namespace TAO::API
{
    /** Finance
     *
     *  Finance API Class.
     *  Manages the function pointers for all Finance commands.
     *
     **/
    class Finance : public Derived<Finance>
    {
    public:

        /** Default Constructor. **/
        Finance()
        : Derived<Finance>()
        {
        }


        /** Initialize.
         *
         *  Sets the function pointers for this API.
         *
         **/
        void Initialize() final;


        /** Name
         *
         *  Returns the name of this API.
         *
         **/
        static std::string Name()
        {
            return "finance";
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
        encoding::json Burn(const encoding::json& jParams, const bool fHelp);


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
        encoding::json Create(const encoding::json& jParams, const bool fHelp);


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
        encoding::json Credit(const encoding::json& jParams, const bool fHelp);


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
        encoding::json Debit(const encoding::json& jParams, const bool fHelp);


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
        encoding::json Get(const encoding::json& jParams, const bool fHelp);



        /** GetBalances
         *
         *  Get balances for a particular token type including immature, stake, and unclaimed.
         *
         *  @param[in] jParams The parameters from the API call.
         *  @param[in] fHelp Trigger for help data.
         *
         *  @return The return object in JSON.
         *
         **/
        encoding::json GetBalances(const encoding::json& jParams, const bool fHelp);


        /** GetStakeInfo
         *
         *  Get staking metrics for a trust account.
         *
         *  @param[in] jParams The parameters from the API call.
         *  @param[in] fHelp Trigger for help data.
         *
         *  @return The return object in JSON.
         *
         **/
        encoding::json GetStakeInfo(const encoding::json& jParams, const bool fHelp);


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
        encoding::json List(const encoding::json& jParams, const bool fHelp);


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
        encoding::json ListTransactions(const encoding::json& jParams, const bool fHelp);


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
        encoding::json MigrateAccounts(const encoding::json& jParams, const bool fHelp);


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
        encoding::json SetStake(const encoding::json& jParams, const bool fHelp);

    };
}
