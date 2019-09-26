/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To The Voice of The People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_TAO_LEDGER_INCLUDE_DIFFICULTY_H
#define NEXUS_TAO_LEDGER_INCLUDE_DIFFICULTY_H

/* Global TAO namespace. */
namespace TAO
{

    /* Ledger Layer namespace. */
    namespace Ledger
    {

        /** GetDifficulty
         *
         *  Determines the Decimal of nBits per Channel for a decent "Frame of Reference".
         *  Has no functionality in Network Operation.
         *
         *  @param[in] nBits The bits to convert to double
         *  @param[in] nChannel The channel to get difficulty for.
         *
         *  @return the difficulty value.
         *
         **/
        double GetDifficulty(uint32_t nBits, int nChannel);

    }
}

#endif
