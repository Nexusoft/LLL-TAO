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


    /** build_contract
     *
     *  Overload for varadaic templates.
     *
     *  @param[out] s The stream being written to.
     *  @param[in] head The object being written to stream.
     *
     **/
    template<class Head>
    bool build_contract(const std::vector<uint8_t> vByteCode, TAO::Operation::Contract &rContract, uint16_t &nCount, uint64_t &nPos, Head&& head)
    {
        /* Loop through our byte-code to generate our contract. */
        for(; nPos < vByteCode.size(); ++nPos)
        {
            /* Get our current operation code. */
            const uint8_t nCode = vByteCode[nPos];

            /* Check for valid placeholder. */
            if(TAO::Operation::PLACEHOLDER::Valid(nCode))
            {
                /* Check our placeholder value. */
                if(nCode != ++nCount)
                    return debug::error("Parameter placeholder mismatch ", uint16_t(nCode),  " expecting ", nCount);

                /* Add our data-type instruction on placeholder. */
                const uint8_t nType = vByteCode[++nPos];

                /* Check our serialization sizes. */
                const Head tHead =
                    std::forward<Head>(head);

                /* Add our contract level data. */
                rContract <= nType <= tHead;

                debug::log(0, "Adding parameter ", std::hex, uint32_t(nType), " size ", nCount, " expect ", uint32_t(nCode));

                return true;
            }

            /* Add the instructions. */
            rContract <= nCode;
        }

        return true;
    }


    /** build_contract
     *
     *  Handle for variadic template pack
     *
     *  @param[out] rContract The stream being written to.
     *  @param[out] nPos The current binary position for binary template.
     *  @param[in] head The object being written to stream.
     *  @param[in] tail The variadic parameters.
     *
     **/
    template<class Head, class... Tail>
    bool build_contract(const std::vector<uint8_t> vByteCode, TAO::Operation::Contract &rContract, uint64_t &nPos, Head&& head, Tail&&... tail)
    {
        //rContract <= std::forward<Head>(head);
        uint16_t nCount = 0;
        if(!build_contract(vByteCode, rContract, nCount, nPos, std::forward<Head>(head)))
            return debug::error("Failed to build head placeholder");

        return build_contract(vByteCode, rContract, nCount, nPos, std::forward<Tail>(tail)...);
    }
}
