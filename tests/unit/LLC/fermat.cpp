/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/types/uint1024.h>
#include <LLC/types/bignum.h>
#include <LLC/include/random.h>
#include <LLC/prime/fermat.h>
#include <openssl/bn.h>
#include <unit/catch2/catch.hpp>

/* Used after Miller-Rabin and Divisor tests to verify primality. */
LLC::CBigNum FermatTest2(const LLC::CBigNum& bnPrime)
{
    LLC::CAutoBN_CTX pctx;
    LLC::CBigNum bnExp = bnPrime - 1;

    LLC::CBigNum bnResult;
    LLC::CBigNum bnBase(2);
    BN_mod_exp(bnResult.getBN(), bnBase.getBN(), bnExp.getBN(), bnPrime.getBN(), pctx);

    return bnResult;
}


uint1024_t FermatTest(const uint1024_t &p)
{
    uint1024_t r;
    uint32_t e[32];
    uint32_t table[WINDOW_SIZE * 32];


    uint32_t *rr = (uint32_t *)r.begin();
    uint32_t *pp = (uint32_t *)p.begin();

    sub_ui<32>(e, pp, 1);

    pow2m<32>(rr, e, pp, table);
    //pow2m<32>(rr, e, pp);

    return r;
}


TEST_CASE("Fermat Tests", "[LLC]")
{

    uint1024_t hashNumber = uint1024_t("0x010009f035e34e85a13fe2c51d56d96781ace0b2df31fecff9ff09094e7772db452d335fe59dfaab61a6bafcf399a5705e98a9b2e1b368e37d267f76693388ffe8255177a734eb77ceac385f0a994288f24bc2526d4c53499aaf270232eb9d31f6ee6c78627bbd490ac899c5a814d861acafd17f51882e68dc01f7330db013cc");
    uint64_t nonce = uint64_t(5190024797402611181);

    uint1024_t bn1 = hashNumber + nonce;
    LLC::CBigNum bn2(bn1); 

    REQUIRE(FermatTest(bn1).GetHex() == FermatTest2(bn2).getuint1024().GetHex());

    for(uint32_t i = 0; i < 1000; ++i)
    {
        bn1 = LLC::GetRand1024();
        bn1 |= 1; //make odd

        uint32_t mask = 0x80000000;
        // mask off highest order bit
        if(bn1.get(31) & mask)
            bn1 ^= (uint1024_t(mask) << 992);

        bn2 = LLC::CBigNum(bn1);

        //REQUIRE(bn1.bits() < 1024);

        REQUIRE(FermatTest(bn1).GetHex() == FermatTest2(bn2).getuint1024().GetHex());
    }



}
