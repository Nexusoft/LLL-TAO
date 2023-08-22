/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2023

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#include <TAO/Operation/include/enum.h>

#include <vector>

/* Forward declarations. */
namespace TAO::Operation { class Contract; }

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

        /** Receiver
         *
         *  An expiring contract that expires the receiver's ability to claim after a given time period.
         *
         **/
        extern const std::vector<std::vector<uint8_t>> Receiver;


        /** Sender
         *
         *  An expiring contract that expires the senders's ability to claim after a given time period.
         *
         **/
        extern const std::vector<uint8_t> Sender;

    }
}
