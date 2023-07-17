/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/types/commands/invoices.h>

#include <TAO/API/include/check.h>
#include <TAO/API/include/extract.h>
#include <TAO/API/include/filter.h>
#include <TAO/API/include/get.h>

#include <TAO/Operation/include/enum.h>

#include <TAO/Ledger/include/timelocks.h>

#include <LLD/include/global.h>


/* Global TAO namespace. */
namespace TAO::API
{
    /* Returns a status for the invoice (outstanding/paid/cancelled) */
    std::string Invoices::get_status(const TAO::Register::State& state, const uint256_t& hashRecipient)
    {
        /* If the Invoice currently has no owner then we know it is outstanding */
        if(state.hashOwner.GetType() == TAO::Ledger::GENESIS::SYSTEM)
            return "OUTSTANDING";

        /* If register is owned by recipient, then it is paid. */
        if(state.hashOwner == hashRecipient)
            return "PAID";

        /* Otherwise treat as cancelled. */
        return "CANCELLED";
    }


    /* Looks up the transaction ID and Contract ID for the transfer transaction that needs to be paid */
    bool Invoices::find_invoice(const uint256_t& hashRecipient, const TAO::Register::Address& hashInvoice,
                          uint512_t &hashTx, uint32_t &nContract)
    {
        /* Get all registers that have been transferred to the recipient but not yet paid (claimed) */
        std::vector<std::tuple<TAO::Operation::Contract, uint32_t, uint256_t>> vUnclaimed;
        GetUnclaimed(hashRecipient, vUnclaimed);

        /* search the vector of unclaimed to see if this invoice is in there */
        const auto& itt = std::find_if(vUnclaimed.begin(), vUnclaimed.end(),
                            [&](const std::tuple<TAO::Operation::Contract, uint32_t, uint256_t>& tUnclaimed)
                            {
                                return std::get<2>(tUnclaimed) == hashInvoice;
                            });

        /* If found set the txid from the contract */
        if(itt != vUnclaimed.end())
        {
            hashTx = std::get<0>(*itt).Hash();
            nContract = std::get<1>(*itt);

            return true;
        }

        /* If we haven't found the transaction then return false */
        return false;
    }
}
