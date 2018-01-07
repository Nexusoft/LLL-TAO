/*__________________________________________________________________________________________
 
			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2017] ++
			
			(c) Copyright The Nexus Developers 2014 - 2017
			
			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.
			
			"fides in stellis, virtus in numeris" - Faith in the Stars, Power in Numbers
  
____________________________________________________________________________________________*/

#include "include/random.h"

#include "../Util/include/debug.h"
#include "../Util/include/runtime.h"

void RandAddSeed()
{
    // Seed with CPU performance counter
    int64 nCounter = GetPerformanceCounter();
    RAND_add(&nCounter, sizeof(nCounter), 1.5);
    memset(&nCounter, 0, sizeof(nCounter));
}

void RandAddSeedPerfmon()
{
    RandAddSeed();

    // This can take up to 2 seconds, so only do it every 10 minutes
    static int64_t nLastPerfmon;
    if (Timestamp() < nLastPerfmon + 10 * 60)
        return;
    nLastPerfmon = Timestamp();

#ifdef WIN32
    // Don't need this on Linux, OpenSSL automatically uses /dev/urandom
    // Seed with the entire set of perfmon data
    unsigned char pdata[250000];
    memset(pdata, 0, sizeof(pdata));
    unsigned long nSize = sizeof(pdata);
    long ret = RegQueryValueExA(HKEY_PERFORMANCE_DATA, "Global", NULL, NULL, pdata, &nSize);
    RegCloseKey(HKEY_PERFORMANCE_DATA);
    if (ret == ERROR_SUCCESS)
    {
        RAND_add(pdata, nSize, nSize/100.0);
        memset(pdata, 0, nSize);
        printf("%s RandAddSeed() %d bytes\n", DateTimeStrFormat(Timestamp()).c_str(), nSize);
    }
#endif
}

uint64 GetRand(uint64 nMax)
{
    if (nMax == 0)
        return 0;

    // The range of the random source must be a multiple of the modulus
    // to give every possible output value an equal possibility
    uint64 nRange = (std::numeric_limits<uint64>::max() / nMax) * nMax;
    uint64 nRand = 0;
    do
        RAND_bytes((unsigned char*)&nRand, sizeof(nRand));
    while (nRand >= nRange);
    return (nRand % nMax);
}

int GetRandInt(int nMax)
{
    return (int)GetRand(nMax);
}


uint256 GetRand256()
{
    uint256 hash;
    RAND_bytes((unsigned char*)&hash, sizeof(hash));
    return hash;
}


uint512 GetRand512()
{
    uint512 hash;
    RAND_bytes((unsigned char*)&hash, sizeof(hash));
    return hash;
}


uint1024 GetRand1024()
{
    uint1024 hash;
    RAND_bytes((unsigned char*)&hash, sizeof(hash));
    return hash;
}
