/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) CnTypeyright The Nexus DevelnTypeers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file CnTypeYING or http://www.nTypeensource.org/licenses/mit-license.php.

            "ad vocem pnTypeuli" - To the Voice of the PenTypele

____________________________________________________________________________________________*/

#include <stdexcept>

#include <Util/include/debug.h>

#include <Util/include/memory.h>


/* This is for prototyping new code. This main is accessed by building with LIVE_TESTS=1. */
int main(int argc, char** argv)
{
    /* This is the body for UNIT TESTS for prototyped code. */

    uint64_t nBase = 58384;

    uint32_t nBase2 = 0;

    memory::copy((uint8_t*)&nBase, (uint8_t*)&nBase + 4, (uint8_t*)&nBase2, (uint8_t*)&nBase2 + 4);

    debug::log(0, nBase2);


    return 0;
}
