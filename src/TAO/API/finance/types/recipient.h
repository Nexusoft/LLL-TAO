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

        /* Helper class to describe a recipient of a debit */
        class Recipient
        {
        public:

            /** The recieving address **/
            TAO::Register::Address hashAddress;


            /** The amount to send **/
            uint64_t nAmount;


            /** Reference to include in the debit **/
            uint64_t nReference;


            /** Default Constructor. **/
            Recipient();


            /** Constructor from values. **/
            Recipient(const TAO::Register::Address& hashAddressIn, const uint64_t nAmountIn, const uint64_t nReferenceIn);


            /** Copy constructor. **/
            Recipient(const Recipient& recipient);


            /** Move constructor. **/
            Recipient(Recipient&& recipient) noexcept;


            /** Copy assignment. **/
            Recipient& operator=(const Recipient& recipient);


            /** Move assignment. **/
            Recipient& operator=(Recipient&& recipient) noexcept;


            /** Default Destructor. **/
            ~Recipient();

        };


        /** CalcRecipients
         *
         *  Calculates the running balances for an optimal number of contracts
         *
         *  @param[in] vRecipients Vector of running balances to update.
         *  @param[in] nTarget The target number of contracts to sum the balances for
         *
         *  @return The total balance of all of the accounts.
         *
         **/
        void CalcRecipients(std::vector<Recipient>& vRecipients, const uint8_t nTarget);


        /** GetTotalBalance
         *
         *  Calculates the balance available from all of the running balances
         *
         *  @param[in] vRecipients Vector of running balances to sum from.
         *
         *  @return The total balance of all of the accounts.
         *
         **/
        uint64_t GetTotalBalance(const std::vector<Recipient>& vRecipients);
    }
}
