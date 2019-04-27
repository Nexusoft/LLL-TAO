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

#include <openssl/md5.h>


uint32_t nTotal = 32;

uint32_t GetBucket(const std::vector<uint8_t>& vKey)
{
    uint64_t nHash = XXH64(&vKey[0], vKey.size(), 0);

    return nHash % nTotal;
}

uint32_t GetBucket2(const std::vector<uint8_t>& vKey)
{
    /* Get an MD5 digest. */
    std::vector<uint8_t> vDigest(16, 0);
    MD5((unsigned char*)&vKey[0], vKey.size(), (unsigned char*)&vDigest[0]);

    /* Copy bytes into the bucket. */
    uint64_t nHash;
    std::copy((uint8_t*)&vDigest[0], (uint8_t*)&vDigest[0] + 8, (uint8_t*)&nHash);

    return nHash % nTotal;
}


int main(int argc, char** argv)
{
    {
        std::vector<uint32_t> slots(256 * 256 * 24, 0);
        std::vector<uint32_t> slots2(256 * 256 * 24, 0);
        for(uint32_t i = 0; i < nTotal; i++)
        {
            DataStream ssKey(SER_LLD, LLD::DATABASE_VERSION);
            ssKey << std::make_pair(std::string("data"), LLC::GetRand256());

            {
                uint32_t nBucket = GetBucket(ssKey.Bytes());
                slots[nBucket] ++;
            }

            {
                uint32_t nBucket = GetBucket2(ssKey.Bytes());
                slots2[nBucket] ++;
            }
        }

        for(uint32_t i = 0; i < nTotal; i ++)
        {
            printf("%u [%f][%f]\n", i, 100.0 * slots[i] / nTotal, 100.0 * slots2[i] / nTotal);
        }
    }

    return 0;
}
