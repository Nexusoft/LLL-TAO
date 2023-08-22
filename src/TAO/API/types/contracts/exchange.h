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
    /** Contracts::Exchange
     *
     *  This creates a limited order that can be executed and exchanged.
     *
     **/
    namespace Contracts::Exchange
    {

        /** Token
         *
         *  An exchange contract that exchanges two tokens for one another.
         *
         **/
        extern const std::vector<std::vector<uint8_t>> Token;


        /** Asset
         *
         *  An exchange contract that exchanges a tokens for an asset.
         *
         **/
        extern const std::vector<uint8_t> Asset;

    }
}
