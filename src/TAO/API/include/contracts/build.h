/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#include <vector>

#include <TAO/API/types/contracts/params.h>

#include <TAO/Operation/types/contract.h>

/* Global TAO namespace. */
namespace TAO::API::Contracts
{
    /** Build
     *
     *  Build that a given contract with variadic parameters based on placeholder binary templates.
     *
     *  @param[in] vByteCode The binary template to build off of.
     *  @param[in] rContract The contract we are checking against.
     *  @param[in] rArgs The variadic template pack
     *
     *  @return true if contract matches, false otherwise.
     *
     **/
    template<class... Args>
    bool Build(const std::vector<uint8_t> vByteCode, TAO::Operation::Contract &rContract, Args&&... rArgs)
    {
        /* Reset our conditional stream each time this is called. */
        rContract.Clear(TAO::Operation::Contract::CONDITIONS);

        /* Unpack our parameter pack and place expected parameters over the PLACEHOLDERS. */
        TAO::API::Contracts::Params ssParams;
        ((ssParams << rArgs), ...);

        /* Loop through our byte-code to generate our contract. */
        uint32_t nMax = 0;
        for(uint64_t nPos = 0; nPos < vByteCode.size(); ++nPos)
        {
            /* Get our current operation code. */
            const uint8_t nCode = vByteCode[nPos];

            /* Check for valid placeholder. */
            if(TAO::Operation::PLACEHOLDER::Valid(nCode))
            {
                /* Get placeholder index. */
                const uint32_t nIndex = (nCode - 1);

                /* Check our placeholder. */
                if(!ssParams.check(nIndex, vByteCode[++nPos]))
                    return debug::error(FUNCTION, "parameter type mismatch");

                /* Bind parameter to placeholder. */
                ssParams.bind(rContract, nIndex);
                nMax = std::max(nIndex, nMax);

                continue;
            }

            /* Add the instructions. */
            rContract <= nCode;
        }

        /* Check that we don't have too many parameters passed into variadic template. */
        if(nMax < ssParams.max())
            return debug::error(FUNCTION, "too many parameters for binary template");

        return true;
    }
}
