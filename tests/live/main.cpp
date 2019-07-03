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

#include <list>



/* This is for prototyping new code. This main is accessed by building with LIVE_TESTS=1. */
int main(int argc, char** argv)
{


    uint256_t hashTest = LLC::GetRand256();


    TAO::Ledger::Genesis genesis = TAO::Ledger::Genesis(hashTest);

    debug::log(0, "Genesis ", genesis.ToString());

    debug::log(0, "Hash: ", hashTest.ToString());

    TAO::Register::Address addr = TAO::Register::Address(TAO::Register::Address::NAME);

    debug::log(0, "Hash: ", addr.ToString());

    printf("BYTE: %x\n", addr.GetType());

    debug::log(0, "Valid: ", addr.IsValid() ? "Yes" : "No");

    TAO::Register::Address name = TAO::Register::Address("colin2", TAO::Register::Address::NAMESPACE);

    debug::log(0, "Hash: ", name.ToString());


    return 0;
}
