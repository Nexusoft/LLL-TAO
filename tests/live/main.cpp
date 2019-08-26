/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/types/uint1024.h>
#include <LLC/hash/SK.h>
#include <LLC/include/random.h>

#include <LLD/cache/binary_lfu.h>

#include <Util/include/debug.h>
#include <Util/include/base64.h>

#include <openssl/rand.h>

#include <LLC/hash/argon2.h>
#include <LLC/hash/SK.h>
#include <LLC/include/flkey.h>
#include <LLC/types/bignum.h>

#include <Util/include/hex.h>

#include <iostream>

#include <TAO/Register/types/address.h>

#include <TAO/Ledger/types/genesis.h>
#include <TAO/Ledger/types/sigchain.h>
#include <TAO/Ledger/types/transaction.h>

#include <TAO/Ledger/include/ambassador.h>

#include <Legacy/types/address.h>
#include <Legacy/types/transaction.h>

#include <list>
#include <variant>


/* This is for prototyping new code. This main is accessed by building with LIVE_TESTS=1. */
int main(int argc, char** argv)
{
    //debug::log(0, TAO::Register::Address(TAO::Register::Address::RESERVED).ToBase58());
    //debug::log(0, TAO::Register::Address(TAO::Register::Address::RESERVED2).ToBase58());
    debug::log(0, TAO::Register::Address(TAO::Register::Address::LEGACY).ToBase58());
    debug::log(0, TAO::Register::Address(TAO::Register::Address::LEGACY_TESTNET).ToBase58());
    debug::log(0, TAO::Register::Address(TAO::Register::Address::READONLY).ToBase58());
    debug::log(0, TAO::Register::Address(TAO::Register::Address::APPEND).ToBase58());
    debug::log(0, TAO::Register::Address(TAO::Register::Address::RAW).ToBase58());
    debug::log(0, TAO::Register::Address(TAO::Register::Address::OBJECT).ToBase58());
    debug::log(0, TAO::Register::Address(TAO::Register::Address::ACCOUNT).ToBase58());
    debug::log(0, TAO::Register::Address(TAO::Register::Address::TOKEN).ToBase58());
    debug::log(0, TAO::Register::Address(TAO::Register::Address::TRUST).ToBase58());
    debug::log(0, TAO::Register::Address(TAO::Register::Address::NAME).ToBase58());
    debug::log(0, TAO::Register::Address(TAO::Register::Address::NAMESPACE).ToBase58());
    debug::log(0, TAO::Register::Address(TAO::Register::Address::WILDCARD).ToBase58());

    //Legacy::NexusAddress legacy("4iwPaoaCYdrQ5hX88K4TRdrTuRRtGn931qW66spaGgNiWe2dGRp");

    // for(uint8_t n = 0; n<255; n++)
    // {
    //     std::string str = TAO::Register::Address(n).ToBase58();
    //     debug::log(0, str + " - " +std::to_string(n));
    // }

    // TAO::Register::Address tritium;
    // tritium.SetBase58("4iwPaoaCYdrQ5hX88K4TRdrTuRRtGn931qW66spaGgNiWe2dGRp");
    // debug::log(0, tritium.ToBase58());
    // debug::log(0, std::to_string(tritium.GetType()));
  

    return 0;
}
