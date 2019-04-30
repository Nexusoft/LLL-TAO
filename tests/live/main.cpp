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

#include <bitset>

#include <LLD/templates/sector.h>
#include <LLD/keychain/hashtree.h>
#include <LLD/cache/binary_lru.h>

#include <TAO/Ledger/include/prime.h>

#include <assert.h>

std::atomic<uint32_t> nTotalFermat;

void Fermat()
{
    uint1024_t hashNumber = uint1024_t("0x010009f035e34e85a13fe2c51d56d96781ace0b2df31fecff9ff09094e7772db452d335fe59dfaab61a6bafcf399a5705e98a9b2e1b368e37d267f76693388ffe8255177a734eb77ceac385f0a994288f24bc2526d4c53499aaf270232eb9d31f6ee6c78627bbd490ac899c5a814d861acafd17f51882e68dc01f7330db013cc");

    uint64_t nonce = uint64_t(5190024797402611181);

    LLC::CBigNum bnPrime = LLC::CBigNum(hashNumber + nonce);
    while(true)
    {
        TAO::Ledger::FermatTest(bnPrime, 2);

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
