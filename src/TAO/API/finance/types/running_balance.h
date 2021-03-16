/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2021

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once

#include <LLC/types/uint1024.h>
#include <TAO/Register/types/address.h>

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        /* Used as a helper for keeping the running balances of accounts as we debit them, when debiting multiple accounts. */
        class RunningBalance
        {
        public:

            /** Register address of the account **/
            TAO::Register::Address hashAddress;


            /** The current balance **/
            uint64_t nBalance;


            /** Sum of balances of the optimal number of contracts below it in the accounts list **/
            uint64_t nSumBalances;


            /** Default Constructor. **/
            RunningBalance();


            /** Constructor from values. **/
            RunningBalance(const TAO::Register::Address& hashAddressIn, const uint64_t nBalanceIn);


            /** Copy constructor. **/
            RunningBalance(const RunningBalance& balance);


            /** Move constructor. **/
            RunningBalance(RunningBalance&& balance) noexcept;


            /** Copy assignment. **/
            RunningBalance& operator=(const RunningBalance& balance);


            /** Move assignment. **/
            RunningBalance& operator=(RunningBalance&& balance) noexcept;


            /** Default Destructor. **/
            ~RunningBalance();

        };


        /** CalcRunningBalances
         *
         *  Calculates the running balances for a target number of contracts
         *
         *  @param[in] vRunningBalances Vector of running balances to update.
         *  @param[in] nTarget The target number of contracts to sum the balances for
         *
         *  @return The total balance of all of the accounts.
         *
         **/
        void CalcRunningBalances(std::vector<RunningBalance>& vRunningBalances, const uint8_t nTarget);


        /** GetTotalBalance
         *
         *  Calculates the balance available from all accounts used in the running balances
         *
         *  @param[in] vRunningBalances Vector of running balances to sum from.
         *
         *  @return The total balance of all of the accounts.
         *
         **/
        uint64_t GetTotalBalance(const std::vector<RunningBalance>& vRunningBalances);
    }
}
