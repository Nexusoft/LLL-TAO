/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/types/contracts/exchange.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /**  Contracts::Exchange
     *
     *  This contract allows the recipient a window of time to claim the contract, after which the recipient will no longer
     *  be able to claim the funds, and they will return back to the sender.
     *
     **/
    namespace Contracts::Exchange
    {
        /* So we can save a lot of typing. */
        using OP          = TAO::Operation::OP;
        using PLACEHOLDER = TAO::Operation::PLACEHOLDER;

        /* Build a binary stream to check conditions against. */
        const std::vector<std::vector<uint8_t>> Token =
        {
            //Version 1 contract.
            {
                OP::GROUP,
                OP::CALLER::OPERATIONS, OP::CONTAINS,    PLACEHOLDER::_1, OP::TYPES::BYTES,
                OP::AND,
                OP::CALLER::PRESTATE::VALUE, OP::EQUALS, PLACEHOLDER::_2, OP::TYPES::UINT256_T,
                OP::UNGROUP,
                OP::OR,
                OP::GROUP,
                OP::CALLER::GENESIS, OP::EQUALS, OP::CONTRACT::GENESIS,
                OP::UNGROUP
            },

            //Version 2 contract.
            {
                OP::GROUP,
                OP::CALLER::OPERATIONS, OP::CONTAINS, PLACEHOLDER::_1, OP::TYPES::BYTES,
                OP::UNGROUP,
                OP::OR,
                OP::GROUP,
                OP::CALLER::GENESIS, OP::EQUALS, OP::CONTRACT::GENESIS,
                OP::UNGROUP
            }
        };


        /* Build a binary stream to check conditions against. */
        const std::vector<uint8_t> Asset =
        {
        };
    }
}
