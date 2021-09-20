/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#include <vector>

/* Forward declarations. */
namespace TAO::Operation { class Contract; }


/* Global TAO namespace. */
namespace TAO::API::Contracts
{

    /** Verify
     *
     *  Verify that a given contract is wel formed and the binary data matches pattern.
     *
     *  @param[in] rContract The contract we are checking against.
     *
     *  @return true if contract matches, false otherwise.
     *
     **/
    bool Verify(const std::vector<uint8_t> vByteCode, const TAO::Operation::Contract& rContract);
}
