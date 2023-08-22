/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2023

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/types/contracts/expiring.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /** Contracts::Expiring
     *
     *  This contract allows the recipient a window of time to claim the contract, after which the recipient will no longer
     *  be able to claim the funds, and they will return back to the sender.
     *
     **/
    namespace Contracts::Expiring
    {
        /* So we can save a lot of typing. */
        using OP          = TAO::Operation::OP;
        using PLACEHOLDER = TAO::Operation::PLACEHOLDER;

        /* Build a binary stream to check conditions against. */
        const std::vector<std::vector<uint8_t>> Receiver =
        {
            //version 1 expiring contract
            {
                OP::GROUP,
                OP::CALLER::GENESIS, OP::NOTEQUALS, PLACEHOLDER::_1, OP::TYPES::UINT256_T,
                OP::AND,
                OP::CONTRACT::TIMESTAMP, OP::ADD,   PLACEHOLDER::_2, OP::TYPES::UINT64_T,
                OP::GREATERTHAN, OP::LEDGER::TIMESTAMP,
                OP::UNGROUP,
                OP::OR,
                OP::GROUP,
                OP::CALLER::GENESIS, OP::EQUALS,    PLACEHOLDER::_1, OP::TYPES::UINT256_T,
                OP::AND,
                OP::CONTRACT::TIMESTAMP, OP::ADD,   PLACEHOLDER::_2, OP::TYPES::UINT64_T,
                OP::LESSTHAN, OP::CALLER::TIMESTAMP,
                OP::UNGROUP
            },

            //version 2 expiring contract
            {
                OP::GROUP,
                OP::CALLER::GENESIS, OP::NOTEQUALS, OP::CONTRACT::GENESIS,
                OP::AND,
                OP::CONTRACT::TIMESTAMP, OP::ADD, PLACEHOLDER::_1, OP::TYPES::UINT64_T,
                OP::GREATERTHAN, OP::LEDGER::TIMESTAMP,
                OP::UNGROUP,
                OP::OR,
                OP::GROUP,
                OP::CALLER::GENESIS, OP::EQUALS, OP::CONTRACT::GENESIS,
                OP::AND,
                OP::CONTRACT::TIMESTAMP, OP::ADD, PLACEHOLDER::_1, OP::TYPES::UINT64_T,
                OP::LESSTHAN, OP::LEDGER::TIMESTAMP,
                OP::UNGROUP
            },
        };


        /* Build a binary stream to check conditions against. */
        const std::vector<uint8_t> Sender =
        {
            OP::GROUP,
            OP::CALLER::GENESIS, OP::EQUALS,  PLACEHOLDER::_1, OP::TYPES::UINT256_T,
            OP::AND,
            OP::CONTRACT::TIMESTAMP, OP::ADD, PLACEHOLDER::_2, OP::TYPES::UINT64_T,
            OP::GREATERTHAN, OP::LEDGER::TIMESTAMP,
            OP::UNGROUP,
            OP::OR,
            OP::GROUP,
            OP::CALLER::GENESIS, OP::NOTEQUALS, PLACEHOLDER::_1, OP::TYPES::UINT256_T,
            OP::AND,
            OP::CONTRACT::TIMESTAMP, OP::ADD,   PLACEHOLDER::_2, OP::TYPES::UINT64_T,
            OP::LESSTHAN, OP::CALLER::TIMESTAMP,
            OP::UNGROUP
        };
    }
}
