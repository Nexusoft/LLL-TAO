/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2021

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/finance/types/running_balance.h>

#include <Util/include/debug.h>

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        /* Default Constructor. */
        RunningBalance::RunningBalance()
        : hashAddress  ( )
        , nBalance     (0)
        , nSumBalances (0)
        {
        }


        /* Constructor from values. */
        RunningBalance::RunningBalance(const TAO::Register::Address& hashAddressIn, const uint64_t nBalanceIn)
        : hashAddress  (hashAddressIn)
        , nBalance     (nBalanceIn)
        , nSumBalances (0)
        {
        }


        /* Copy constructor. */
        RunningBalance::RunningBalance(const RunningBalance& balance)
        : hashAddress  (balance.hashAddress)
        , nBalance     (balance.nBalance)
        , nSumBalances (balance.nSumBalances)
        {
        }


        /* Move constructor. */
        RunningBalance::RunningBalance(RunningBalance&& balance) noexcept
        : hashAddress  (std::move(balance.hashAddress))
        , nBalance     (std::move(balance.nBalance))
        , nSumBalances (std::move(balance.nSumBalances))
        {

        }


        /* Copy assignment. */
        RunningBalance& RunningBalance::operator=(const RunningBalance& balance)
        {
            hashAddress  = balance.hashAddress;
            nBalance     = balance.nBalance;
            nSumBalances = balance.nSumBalances;

            return *this;
        }


        /* Move assignment. */
        RunningBalance& RunningBalance::operator=(RunningBalance&& balance) noexcept
        {
            hashAddress  = std::move(balance.hashAddress);
            nBalance     = std::move(balance.nBalance);
            nSumBalances = std::move(balance.nSumBalances);

            return *this;
        }


        /* Default Destructor. */
        RunningBalance::~RunningBalance()
        {
        }
        

        /* Calculates the running balances for a target number of contracts */
        void CalcRunningBalances(std::vector<RunningBalance>& vRunningBalances, const uint8_t nTarget)
        {            
            /* For each account, sum the balance of the X accounts below it, where X is the target number of contracts to create. */
            for(int nIndex=0; nIndex < vRunningBalances.size(); nIndex++)
            {
                /* The sum of the target number of balances */
                uint64_t nSumBalances = 0;

                /* Iterate backwards through nTarget accounts and sum the balances*/
                for(int nSumIndex=nIndex; nSumIndex >= 0 && nIndex - nSumIndex < nTarget; nSumIndex--)
                    nSumBalances += vRunningBalances.at(nSumIndex).nBalance;

                /* Update the sum X balance for this account */
                vRunningBalances.at(nIndex).nSumBalances = nSumBalances;

            }

            /* Log calculations to help debug */
            debug::log(4, FUNCTION, "Calculating running balances for target: ", (int)nTarget );            
            for(int nIndex=0; nIndex < vRunningBalances.size(); nIndex++)
                debug::log(4, FUNCTION, nIndex, " - Address: ", 
                    vRunningBalances.at(nIndex).hashAddress.ToString(), 
                    " Balance: ", 
                    vRunningBalances.at(nIndex).nBalance,
                    " Sum(", nTarget,"): ", vRunningBalances.at(nIndex).nSumBalances);
        }


        /* Calculates the balance available from all accounts used in the running balances */
        uint64_t GetTotalBalance(const std::vector<RunningBalance>& vRunningBalances)
        {
            /* Total balance across all accounts to return */
            uint64_t nBalance = 0;

            for(size_t nIndex=0; nIndex < vRunningBalances.size(); nIndex++)
            {
                /* Update the total balance */
                nBalance += vRunningBalances.at(nIndex).nBalance;
            }

            /* Return the total balance */
            return nBalance;
        }
    }
}
