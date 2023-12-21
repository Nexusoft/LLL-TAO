/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2023

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/types/uint1024.h>
#include <LLC/types/bignum.h>
#include <LLC/prime/fermat.h>

#include <Util/include/debug.h>

#include <openssl/bn.h>

#include <Util/include/runtime.h>

int main(int argc, char** argv)
{
    uint1024_t hashProof = uint1024_t("0x62be93f239bcff29f4e8b73996db69f052ee5bf3c88f13271e7b03cae96d1d74f0d7c3fcb928760127702ca8012ea9581bdb4526e65498b6d345fbb832d695d48b6eb4e1b6027fd110bdd402696fb283196314c8cd8466c19ffb498bfb71dac66b777d6c90e62fe58eb583dca1cbc66beb339bb1221550b5808cc15eaa1ae329");

    uint64_t nNonce = 16059175321220995000;

    uint1024_t hashPrime = hashProof + nNonce;

    LLC::CAutoBN_CTX pctx;

    LLC::CBigNum bnPrime(hashPrime);
    LLC::CBigNum bnBase(2);
    LLC::CBigNum bnExp = bnPrime - 1;


    runtime::stopwatch tElapsed1;
    tElapsed1.start();
    LLC::CBigNum bnResult;
    BN_mod_exp_mont(bnResult.getBN(), bnBase.getBN(), bnExp.getBN(), bnPrime.getBN(), pctx, NULL);
    tElapsed1.stop();

    runtime::stopwatch tElapsed2;
    tElapsed2.start();
    uint1024_t hashFermat = LLC::fermat_prime(hashPrime);
    tElapsed2.stop();

    //return bnResult.getuint1024();

    debug::log(0, "Prime for bignum is ", bnResult.getuint32(), " in ", tElapsed1.ElapsedMicroseconds());
    debug::log(0, "Prime for fermat is ", hashFermat.getuint32(), " in ", tElapsed2.ElapsedMicroseconds());


    uint32_t nBits = 0x7b2f3cb2;
    LLC::CBigNum bnTarget;
    bnTarget.SetCompact(nBits);

    uint1024_t hashTarget;
    hashTarget.SetCompact(nBits);
    debug::log(0, "Compact is ", bnTarget.getuint1024().ToString());
    debug::log(0, "Compact is ", hashTarget.ToString());

    return 0;
}
