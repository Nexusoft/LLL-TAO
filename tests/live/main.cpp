/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) CnTypeyright The Nexus DevelnTypeers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file CnTypeYING or http://www.nTypeensource.org/licenses/mit-license.php.

            "ad vocem pnTypeuli" - To the Voice of the PenTypele

____________________________________________________________________________________________*/

#include <openssl/rand.h>

#include <LLD/include/version.h>

#include <TAO/Register/types/stream.h>

#include <Util/templates/datastream.h>

#include <LLD/hash/xxh3.h>

#include <LLC/aes/aes.h>

#include <LLC/include/random.h>

#include <Util/include/memory.h>

#include <Util/include/hex.h>

#include <openssl/bn.h>

#include <bitset>

#include <LLD/templates/sector.h>
#include <LLD/keychain/hashtree.h>
#include <LLD/cache/binary_lru.h>

#include <TAO/Ledger/include/prime.h>

#include <assert.h>

std::atomic<uint32_t> nTotalFermat;

/* Used after Miller-Rabin and Divisor tests to verify primality. */
void FermatTest(const LLC::CBigNum& bnPrime, LLC::CBigNum& bnResult)
{
    const static LLC::CBigNum bnBase = 2;

    LLC::CAutoBN_CTX pctx;
    LLC::CBigNum bnExp = bnPrime - 1;

    BN_mod_exp(bnResult.getBN(), bnBase.getBN(), bnExp.getBN(), bnPrime.getBN(), pctx);
}

/* Determines the difficulty of the Given Prime Number. */
double GetPrimeDifficulty(const LLC::CBigNum& bnPrime)
{
    /* Return 0 if base is not prime. */
    LLC::CBigNum bnFraction;
    FermatTest(bnPrime, bnFraction);

    if(bnFraction != 1)
        return 0.0;

    //std::vector<bool> bit_seive(128, 0);
    uint64_t bit_seive = 0;

    /* Largest prime gap is +12 for dense clusters. */
    LLC::CBigNum bnNext = bnPrime + 2;


    /* Keep track of the cluster size. */
    uint32_t nClusterSize = 1;

    uint32_t nComposites = 0;

    /* Largest prime gap is +12 for dense clusters. */
    for(uint32_t nTotal = 0; nTotal < 54 && nComposites < 6; bnNext += 2, ++nTotal)
    {
        if((bit_seive & (uint64_t(1) << nTotal)))
        {
            ++nComposites;
            continue;
        }

        if(!(bit_seive & (uint64_t(1) << nTotal)) && bnNext % 3 == 0)
        {
            for(uint8_t i = nTotal; i < 54; i += 3)
                if(!(bit_seive & (uint64_t(1) << i)))
                    bit_seive |= (uint64_t(1) << i);
        }

        if(!(bit_seive & (uint64_t(1) << nTotal)) && bnNext % 5 == 0)
        {
            for(uint8_t i = nTotal; i < 54; i += 5)
                if(!(bit_seive & (uint64_t(1) << i)))
                    bit_seive |= (uint64_t(1) << i);
        }

        if(!(bit_seive & (uint64_t(1) << nTotal)) && bnNext % 7 == 0)
        {
            for(uint8_t i = nTotal; i < 54; i += 7)
                if(!(bit_seive & (uint64_t(1) << i)))
                    bit_seive |= (uint64_t(1) << i);
        }

        if(!(bit_seive & (uint64_t(1) << nTotal)) && bnNext % 11 == 0)
        {
            for(uint8_t i = nTotal; i < 54; i += 11)
                if(!(bit_seive & (uint64_t(1) << i)))
                    bit_seive |= (uint64_t(1) << i);
        }

        if(!(bit_seive & (uint64_t(1) << nTotal)) && bnNext % 13 == 0)
        {
            for(uint8_t i = nTotal; i < 54; i += 13)
                if(!(bit_seive & (uint64_t(1) << i)))
                    bit_seive |= (uint64_t(1) << i);
        }

        if(!(bit_seive & (uint64_t(1) << nTotal)) && bnNext % 17 == 0)
        {
            for(uint8_t i = nTotal; i < 54; i += 17)
                if(!(bit_seive & (uint64_t(1) << i)))
                    bit_seive |= (uint64_t(1) << i);
        }

        if(!(bit_seive & (uint64_t(1) << nTotal)) && bnNext % 19 == 0)
        {
            for(uint8_t i = nTotal; i < 54; i += 19)
                if(!(bit_seive & (uint64_t(1) << i)))
                    bit_seive |= (uint64_t(1) << i);
        }

        if(!(bit_seive & (uint64_t(1) << nTotal)) && bnNext % 23 == 0)
        {
            for(uint8_t i = nTotal; i < 54; i += 23)
                if(!(bit_seive & (uint64_t(1) << i)))
                    bit_seive |= (uint64_t(1) << i);
        }

        if(!(bit_seive & (uint64_t(1) << nTotal)) && bnNext % 29 == 0)
        {
            for(uint8_t i = nTotal; i < 54; i += 29)
                if(!(bit_seive & (uint64_t(1) << i)))
                    bit_seive |= (uint64_t(1) << i);
        }

        if(!(bit_seive & (uint64_t(1) << nTotal)) && bnNext % 31 == 0)
        {
            for(uint8_t i = nTotal; i < 54; i += 31)
                if(!(bit_seive & (uint64_t(1) << i)))
                    bit_seive |= (uint64_t(1) << i);
        }

        if(nComposites < 6 && (bit_seive & (uint64_t(1) << nTotal)))
            continue;

        /* Check if this interval is prime. */
        FermatTest(bnNext, bnFraction);
        if(bnFraction != 1)
            bit_seive |= (uint64_t(1) << nTotal);
        else
            nComposites = 0;
    }


    nComposites = 0;
    for(uint8_t i = 0; i < 54 && nComposites < 6; ++i)
    {
        if((bit_seive & (uint64_t(1) << i)))
        {
            ++nComposites;
            continue;
        }
        else
        {
            nComposites = 0;
            ++nClusterSize;
        }
    }

    /* Calculate the rarity of cluster from proportion of fermat remainder of last prime + 2. */
    double nRemainder = 1000000.0 / ((bnNext - bnFraction << 24) / bnNext).getuint32();//GetFractionalDifficulty(bnNext);
    if(nRemainder > 1.0 || nRemainder < 0.0)
        nRemainder = 0.0;

    return (nClusterSize + nRemainder);
}

void Fermat()
{
    uint1024_t hashNumber = uint1024_t("0x010009f035e34e85a13fe2c51d56d96781ace0b2df31fecff9ff09094e7772db452d335fe59dfaab61a6bafcf399a5705e98a9b2e1b368e37d267f76693388ffe8255177a734eb77ceac385f0a994288f24bc2526d4c53499aaf270232eb9d31f6ee6c78627bbd490ac899c5a814d861acafd17f51882e68dc01f7330db013cc");

    uint64_t nonce = uint64_t(5190024797402611181);

    LLC::CBigNum bnPrime = LLC::CBigNum(hashNumber + nonce);
    while(true)
    {
        GetPrimeDifficulty(bnPrime);

        ++nTotalFermat;
    }
}


int main(int argc, char** argv)
{

    nTotalFermat = 0;

    runtime::timer timer;
    timer.Start();

    std::thread t1 = std::thread(Fermat);
    std::thread t2 = std::thread(Fermat);
    std::thread t3 = std::thread(Fermat);
    std::thread t4 = std::thread(Fermat);
    //std::thread t5 = std::thread(Fermat);
    //std::thread t6 = std::thread(Fermat);


    uint64_t nTotalTime   = 0;
    uint64_t nAvgFermat = 0;
    while(true)
    {
        runtime::sleep(1000);

        uint64_t nElapsed = timer.ElapsedMilliseconds();

        nAvgFermat += nTotalFermat.load();
        nTotalTime += nElapsed;

        debug::log(0, nTotalFermat.load(), " fermats in ", nElapsed, " milliseconds ", (nTotalFermat * 1000) / nElapsed, " per second [Average ", (nAvgFermat * 1000) / nTotalTime, "]");



        timer.Reset();
        nTotalFermat = 0;
    }


    return 0;
}
