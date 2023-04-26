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
#include <TAO/Ledger/include/stake.h>

#include <Util/include/math.h>

namespace TAO::API
{
    /* Outputs the correct balance in terms of a double based on decimals input. */
    double FormatBalance(const uint64_t nBalance, const uint8_t nDecimals)
    {
        return double(nBalance) / math::pow(10, nDecimals);
    }

    /* Outputs the correct balance in terms of a double based on decimals input. */
    double FormatNegative(const int64_t nBalance, const uint8_t nDecimals)
    {
        return double(nBalance) / math::pow(10, nDecimals);
    }


    /* Outputs the correct balance in terms of a double that can be formatted for output. */
    double FormatBalance(const uint64_t nBalance, const uint256_t& hashToken)
    {
        /* Check for NXS as a value. */
        if(hashToken == 0)
            return double(nBalance) / TAO::Ledger::NXS_COIN;

        /* Otherwise let's lookup our token object. */
        TAO::Register::Object oToken;
        if(!LLD::Register->ReadObject(hashToken, oToken, TAO::Ledger::FLAGS::LOOKUP))
            throw Exception(-13, FUNCTION, "Object not found");

        /* Let's check that a token was passed in. */
        if(oToken.Standard() != TAO::Register::OBJECTS::TOKEN)
            throw Exception(-15, FUNCTION, "Object is not a token");

        return double(nBalance) / math::pow(10, oToken.get<uint8_t>("decimals"));
    }

    /* Outputs the correct balance in terms of a double that can be formatted for output. */
    double FormatMint(const int32_t nBalance)
    {
        return double(nBalance) / TAO::Ledger::NXS_COIN;
    }


    /* Outputs the correct stake change in terms of a double that can be formatted for output. */
    double FormatStake(const int64_t nStake)
    {
        return double(nStake) / TAO::Ledger::NXS_COIN;
    }


    /* Outputs the correct stake rate in terms of a double that can be formatted for output. */
    double FormatStakeRate(const uint64_t nTrust)
    {
        /* Flag to detect if genesis tx. */
        const bool fGenesis = (nTrust == 0);

        /* Grab our stake-rate from ledger. */
        const uint64_t nRate = (TAO::Ledger::StakeRate(nTrust, fGenesis) * 100 * TAO::Ledger::NXS_COIN);
        return double(nRate) / TAO::Ledger::NXS_COIN;
    }
}
