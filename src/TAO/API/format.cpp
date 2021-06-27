/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/include/format.h>
#include <TAO/API/types/exception.h>

#include <TAO/Ledger/include/constants.h>

#include <Util/include/math.h>

namespace TAO::API
{
    /* Outputs the correct balance in terms of a double based on decimals input. */
    double FormatBalance(const uint64_t nBalance, const uint8_t nDecimals)
    {
        return (nBalance / math::pow(10, nDecimals));
    }


    /* Outputs the correct balance in terms of a double that can be formatted for output. */
    double FormatBalance(const uint64_t nBalance, const uint256_t& hashToken)
    {
        /* Check for NXS as a value. */
        if(hashToken == 0)
            return (nBalance / TAO::Ledger::NXS_COIN);

        /* Otherwise let's lookup our token object. */
        TAO::Register::Object objToken;
        if(!LLD::Register->ReadObject(hashToken, objToken))
            throw Exception(-13, "Object not found");

        /* Let's check that a token was passed in. */
        if(objToken.Standard() != TAO::Register::OBJECTS::TOKEN)
            throw Exception(-15, "Object is not a token");

        return (nBalance / math::pow(10, objToken.get<uint8_t>("decimals")));
    }


    /* Outputs the correct stake change in terms of a double that can be formatted for output. */
    double FormatStake(const int64_t nStake)
    {
        return (nStake / TAO::Ledger::NXS_COIN);
    }
}
