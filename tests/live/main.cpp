/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) CnTypeyright The Nexus DevelnTypeers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file CnTypeYING or http://www.nTypeensource.org/licenses/mit-license.php.

            "ad vocem pnTypeuli" - To the Voice of the PenTypele

____________________________________________________________________________________________*/

#include <openssl/rand.h>

#include <TAO/Register/include/object.h>

#include <LLC/aes/aes.h>


#include <LLC/types/uint1024.h>


#define WINDOW_BITS 8
#define WINDOW_SIZE (1 << WINDOW_BITS)


template<uint8_t WORD_MAX>
inline void assign(uint32_t *l, uint32_t *r)
{
    //#pragma unroll
    for(uint8_t i = 0; i < WORD_MAX; ++i)
        l[i] = r[i];
}


template<uint8_t WORD_MAX>
inline void assign_zero(uint32_t *l)
{
    //#pragma unroll
    for(uint8_t i = 0; i < WORD_MAX; ++i)
        l[i] = 0;
}


inline int32_t inv2adic(uint32_t x)
{
    uint32_t a;
    a = x;
    x = (((x+2)&4)<<1)+x;
    x *= 2 - a*x;
    x *= 2 - a*x;
    x *= 2 - a*x;
    return -x;
}


template<uint8_t WORD_MAX>
inline uint32_t cmp_ge_n(uint32_t *x, uint32_t *y)
{
    for(int8_t i = WORD_MAX-1; i >= 0; --i)
    {
        if(x[i] > y[i])
            return 1;

        if(x[i] < y[i])
            return 0;
    }
    return 1;
}


template<uint8_t WORD_MAX>
inline uint8_t sub_n(uint32_t *z, uint32_t *x, uint32_t *y)
{
    uint32_t temp;
    uint8_t c = 0;

    //#pragma unroll
    for(uint8_t i = 0; i < WORD_MAX; ++i)
    {
        temp = x[i] - y[i] - c;
        c = (temp > x[i]);
        z[i] = temp;
    }
    return c;
}


template<uint8_t WORD_MAX>
inline void sub_ui(uint32_t *z, uint32_t *x, const uint32_t &ui)
{
    uint32_t temp = x[0] - ui;
    uint8_t c = temp > x[0];
    z[0] = temp;

    //#pragma unroll
    for(uint8_t i = 1; i < WORD_MAX; ++i)
    {
        temp = x[i] - c;
        c = (temp > x[i]);
        z[i] = temp;
    }
}


template<uint8_t WORD_MAX>
inline void add_ui(uint32_t *z, uint32_t *x, const uint64_t &ui)
{
    uint32_t temp = x[0] + static_cast<uint32_t>(ui & 0xFFFFFFFF);
    uint8_t c = temp < x[0];
    z[0] = temp;

    temp = x[1] + static_cast<uint32_t>(ui >> 32) + c;
    c = temp < x[1];
    z[1] = temp;

    //#pragma unroll
    for(uint8_t i = 2; i < WORD_MAX; ++i)
    {
        temp = x[i] + c;
        c = (temp < x[i]);
        z[i] = temp;
    }
}


template<uint8_t WORD_MAX>
inline uint32_t addmul_1(uint32_t *z, uint32_t *x, const uint32_t y)
{
    uint64_t prod;
    uint32_t c = 0;

    //#pragma unroll
    for(uint8_t i = 0; i < WORD_MAX; ++i)
    {
        prod = static_cast<uint64_t>(x[i]) * static_cast<uint64_t>(y);
        prod += c;
        prod += z[i];
        z[i] = prod;
        c = prod >> 32;
    }

    return c;
}

template<uint8_t WORD_MAX>
inline uint32_t addsqr_1(uint32_t *z, uint32_t *x, const uint8_t i)
{
    uint64_t prod;
    uint32_t c = 0;

    //#pragma unroll
    for(uint8_t j = i + 1; j < WORD_MAX; ++j)
    {
        prod = static_cast<uint64_t>(x[j]) * static_cast<uint64_t>(x[i]);
        prod <<= 1;
        prod += c;
        prod += z[j];
        z[j] = prod;
        c = prod >> 32;
    }

    return c;
}

template<uint8_t WORD_MAX>
inline void sqrredc(uint32_t *z, uint32_t *x, uint32_t *n, const uint32_t d, uint32_t *t)
{
    uint32_t m;//, c;
    //uint64_t temp;

    assign_zero<WORD_MAX>(t);
    t[WORD_MAX] = 0;

    for(uint8_t i = 0; i < WORD_MAX; ++i)
    {
        t[WORD_MAX] += addsqr_1<WORD_MAX>(t, x, i);

        m = t[0]*d;

        t[WORD_MAX] += addmul_1<WORD_MAX>(t, n, m);

        //#pragma unroll
        for(uint8_t j = 0; j <= WORD_MAX; ++j)
            t[j] = t[j+1];
    }
    if(cmp_ge_n<WORD_MAX>(t, n))
        sub_n<WORD_MAX>(t, t, n);

    //#pragma unroll
    for(uint8_t i = 0; i < WORD_MAX; ++i)
        z[i] = t[i];
}


template<uint8_t WORD_MAX>
inline void mulredc(uint32_t *z, uint32_t *x, uint32_t *y, uint32_t *n, const uint32_t d, uint32_t *t)
{
    uint32_t m;//, c;
    //uint64_t temp;

    assign_zero<WORD_MAX>(t);
    t[WORD_MAX] = 0;

    for(uint8_t i = 0; i < WORD_MAX; ++i)
    {
        //c = addmul_1(t, x, y[i]);
        t[WORD_MAX] += addmul_1<WORD_MAX>(t, x, y[i]);
        //temp = static_cast<uint64_t>(t[WORD_MAX]) + c;
        //t[WORD_MAX] = temp;
        //t[WORD_MAX] += c;
        //t[WORD_MAX + 1] = temp >> 32;

        m = t[0]*d;

        //c = addmul_1(t, n, m);
        t[WORD_MAX] += addmul_1<WORD_MAX>(t, n, m);
        //temp = static_cast<uint64_t>(t[WORD_MAX]) + c;
        //t[WORD_MAX] = temp;
        //t[WORD_MAX] += c;
        //t[WORD_MAX + 1] = temp >> 32;

        //#pragma unroll
        for(uint8_t j = 0; j <= WORD_MAX; ++j)
            t[j] = t[j+1];
    }
    if(cmp_ge_n<WORD_MAX>(t, n))
        sub_n<WORD_MAX>(t, t, n);

    //#pragma unroll
    for(uint8_t i = 0; i < WORD_MAX; ++i)
        z[i] = t[i];
}


template<uint8_t WORD_MAX>
void redc(uint32_t *z, uint32_t *x, uint32_t *n, const uint32_t d, uint32_t *t)
{
    uint32_t m;

    assign<WORD_MAX>(t, x);

    t[WORD_MAX] = 0;

    for(uint8_t i = 0; i < WORD_MAX; ++i)
    {
        m = t[0]*d;
        t[WORD_MAX] = addmul_1<WORD_MAX>(t, n, m);

        for(uint8_t j = 0; j < WORD_MAX; ++j)
            t[j] = t[j+1];

        t[WORD_MAX] = 0;
    }

    if(cmp_ge_n<WORD_MAX>(t, n))
        sub_n<WORD_MAX>(t, t, n);

    assign<WORD_MAX>(z, t);
}


template<uint8_t WORD_MAX>
uint16_t bit_count(uint32_t *x)
{
    uint16_t msb = 0; //most significant bit

    //#pragma unroll
    for(uint16_t i = 0; i < (WORD_MAX << 5); ++i)
    {
        if(x[i>>5] & (1 << (i & 31)))
            msb = i;
    }

    return msb + 1; //any number will have at least 1-bit
}


template<uint8_t WORD_MAX>
inline void lshift(uint32_t *r, uint32_t *a, uint16_t shift)
{
    assign_zero<WORD_MAX>(r);

    uint8_t k = shift >> 5;
    shift = shift & 31;

    for(int8_t i = 0; i < WORD_MAX; ++i)
    {
        uint8_t ik = i + k;
        uint8_t ik1 = ik + 1;

        if(ik1 < WORD_MAX && shift != 0)
            r[ik1] |= (a[i] >> (32-shift));
        if(ik < WORD_MAX)
            r[ik] |= (a[i] << shift);
    }
}


template<uint8_t WORD_MAX>
inline void rshift(uint32_t *r, uint32_t *a, uint16_t shift)
{
    assign_zero<WORD_MAX>(r);

    uint8_t k = shift >> 5;
    shift = shift & 31;

    for(int8_t i = 0; i < WORD_MAX; ++i)
    {
        int8_t ik = i - k;
        int8_t ik1 = ik - 1;

        if(ik1 >= 0 && shift != 0)
            r[ik1] |= (a[i] << (32-shift));
        if(ik >= 0)
            r[ik] |= (a[i] >> shift);
    }
}


template<uint8_t WORD_MAX>
inline void lshift1(uint32_t *r, uint32_t *a)
{
    uint32_t t = a[0];
    uint32_t t2;
    r[0] = t << 1;
    for(uint8_t i = 1; i < WORD_MAX; ++i)
    {
        t2 = a[i];
        r[i] = (t2 << 1) | (t >> 31);
        t = t2;
    }
}


template<uint8_t WORD_MAX>
inline void rshift1(uint32_t *r, uint32_t *a)
{
    uint32_t t = a[WORD_MAX-1];
    uint32_t t2;

    r[WORD_MAX-1] = t >> 1;
    for(int8_t i = WORD_MAX-2; i >= 0; --i)
    {
        t2 = a[i];
        r[i] = (t2 >> 1) | (t << 31);
        t = t2;
    }
}


/* Calculate ABar and BBar for Montgomery Modular Multiplication. */
template<uint8_t WORD_MAX>
void calcBar(uint32_t *a, uint32_t *n, uint32_t *t)
{
    assign_zero<WORD_MAX>(a); // set R = 2^BITS == 0 (overflow mitigated by subtraction)

    lshift<WORD_MAX>(t, n, (WORD_MAX<<5) - bit_count<WORD_MAX>(n));
    sub_n<WORD_MAX>(a, a, t);

    while(cmp_ge_n<WORD_MAX>(a, n))  //calculate R mod N;
    {
        rshift1<WORD_MAX>(t, t);
        if(cmp_ge_n<WORD_MAX>(a, t))
            sub_n<WORD_MAX>(a, a, t);
    }

    //lshift1<WORD_MAX>(b, a);     //calculate 2R mod N;
    //if(cmp_ge_n<WORD_MAX>(b, n))
    //    sub_n<WORD_MAX>(b, b, n);
}


/* Calculate ABar and BBar for Montgomery Modular Multiplication. */
template<uint8_t WORD_MAX>
void calcTable(uint32_t *a, uint32_t *n, uint32_t *t, uint32_t *table)
{
    lshift1<WORD_MAX>(t, a);     //calculate 2R mod N;
    if(cmp_ge_n<WORD_MAX>(t, n))
        sub_n<WORD_MAX>(t, t, n);

    assign<WORD_MAX>(table, t);


    for(uint16_t i = 1; i < WINDOW_SIZE; ++i) //calculate 2^i R mod N
    {
        lshift1<WORD_MAX>(t, t);
        if(cmp_ge_n<WORD_MAX>(t, n))
            sub_n<WORD_MAX>(t, t, n);

        assign<WORD_MAX>(&table[i * WORD_MAX], t);
    }
}


/* Calculate X = 2^Exp Mod N (Fermat test) */
template<uint8_t WORD_MAX>
void pow2m(uint32_t *X, uint32_t *Exp, uint32_t *N, uint32_t *table)
{
    uint32_t t[WORD_MAX + 1];
    uint32_t wstart, wend, wval;
    uint32_t d = inv2adic(N[0]);

    calcBar<WORD_MAX>(X, N, t);

    calcTable<WORD_MAX>(X, N, t, table);


    uint32_t bits = bit_count<WORD_MAX>(Exp);
    uint32_t window_count = (bits + WINDOW_BITS - 1) / WINDOW_BITS;

    wstart = bits-1;
    wend = wstart - WINDOW_BITS;


    for(int16_t j = 0; j < window_count; ++j)
    {
        wval = 0;

        for(int16_t i = wstart; i > wend; --i)
        {
            if(i < 0)
                break;

            sqrredc<WORD_MAX>(X, X, N, d, t);

            wval <<= 1;

            if(Exp[i>>5] & (1 << (i & 31)))
                wval |= 1;
        }

        mulredc<WORD_MAX>(X, X, &table[wval * WORD_MAX], N, d, t);

        wstart -= WINDOW_BITS;
        wend   -= WINDOW_BITS;

        if(wstart < 0)
            break;

    }

    redc<WORD_MAX>(X, X, N, d, t);
}


/* Test if number p passes Fermat Primality Test base 2. */
template<uint8_t WORD_MAX>
bool fermat_prime(uint32_t *p, uint32_t *table)
{
    uint32_t e[WORD_MAX];
    uint32_t r[WORD_MAX];

    sub_ui<WORD_MAX>(e, p, 1);
    pow2m<WORD_MAX>(r, e, p, table);

    uint32_t result = r[0] - 1;

    //#pragma unroll
    for(uint8_t i = 1; i < WORD_MAX; ++i)
        result |= r[i];

    return (result == 0);
}




void Fermat2()
{
    uint1024_t hashNumber = uint1024_t("0x010009f035e34e85a13fe2c51d56d96781ace0b2df31fecff9ff09094e7772db452d335fe59dfaab61a6bafcf399a5705e98a9b2e1b368e37d267f76693388ffe8255177a734eb77ceac385f0a994288f24bc2526d4c53499aaf270232eb9d31f6ee6c78627bbd490ac899c5a814d861acafd17f51882e68dc01f7330db013cc");

    uint64_t nonce = uint64_t(5190024797402611181);

    uint1024_t bnPrime = hashNumber + nonce;


    uint32_t windowTable[WINDOW_SIZE * 32];


    while(true)
    {

        fermat_prime<32>((uint32_t *)bnPrime.begin(), windowTable);

        ++nTotalFermat;
    }
}



//This main function is for prototyping new code
//It is accessed by compiling with LIVE_TESTS=1
//Prototype code and it's tests created here should move to production code and unit tests
int main(int argc, char** argv)
{


    return 0;
}
