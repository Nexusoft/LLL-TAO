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

#include <iostream>
#include <iomanip>

#include <cmath>

#include <LLC/types/bignum.h>


typedef unsigned int uint128_t __attribute__((mode(TI)));

class precision_t
{
    uint32_t FIGURES;
public:
    uint64_t nFigures;
    uint64_t nValue;


    uint64_t get_figures()
    {
        uint64_t nTemp = 10;
        for(int i = 1; i < FIGURES; i++)
            nTemp *= 10;

        return nTemp;
    }



    precision_t()
    : FIGURES(8)
    , nFigures(get_figures())
    , nValue(0)
    {
    }


    precision_t(double value)
    : FIGURES(8)
    , nFigures(get_figures())
    , nValue(value * nFigures)
    {
    }


    precision_t& operator=(double value)
    {
        nValue = static_cast<uint64_t>(value * nFigures);

        return *this;
    }


    precision_t& operator=(uint32_t value)
    {
        nValue = value * nFigures;

        return *this;
    }


    precision_t& operator=(int32_t value)
    {
        nValue = value * nFigures;

        return *this;
    }


    bool operator==(int32_t value) const
    {
        uint64_t compare = static_cast<uint64_t>(value) * nFigures;

        return compare == nValue;
    }


    bool operator==(uint32_t value) const
    {
        uint64_t compare = static_cast<uint64_t>(value) * nFigures;

        return compare == nValue;
    }


    bool operator<(int32_t value) const
    {
        uint64_t compare = static_cast<uint64_t>(value) * nFigures;

        return nValue < compare;
    }


    bool operator>(int32_t value) const
    {
        uint64_t compare = static_cast<uint64_t>(value) * nFigures;

        return nValue > compare;
    }


    precision_t& operator+=(const precision_t& value)
    {
        if(value.nFigures < nFigures)
        {
            uint64_t nDiff = nFigures / value.nFigures;

            nValue += (value.nValue * nDiff);

            return *this;
        }

        if(value.nFigures > nFigures)
        {
            uint64_t nDiff = value.nFigures / nFigures;

            nValue += (value.nValue / nDiff);

            return *this;
        }

        nValue += value.nValue;

        return *this;
    }


    precision_t& operator-=(const precision_t& value)
    {
        if(value.nFigures < nFigures)
        {
            uint64_t nDiff = nFigures / value.nFigures;

            nValue -= (value.nValue * nDiff);

            return *this;
        }

        if(value.nFigures > nFigures)
        {
            uint64_t nDiff = value.nFigures / nFigures;

            nValue -= (value.nValue / nDiff);

            return *this;
        }

        nValue -= value.nValue;

        return *this;
    }


    precision_t operator-(const int32_t& value)
    {
        precision_t ret = *this - (value * nFigures);

        return ret;
    }


    precision_t& operator/=(const precision_t& value)
    {
        uint128_t nTemp = nValue;

        nTemp     *= value.nFigures;
        nTemp     /= value.nValue;

        nValue   = static_cast<uint64_t>(nTemp);

        return *this;
    }


    precision_t& operator*=(const precision_t& value)
    {
        uint128_t nTemp = nValue;

        nTemp *= value.nValue;
        nTemp /= value.nFigures;

        while(nTemp > std::numeric_limits<uint64_t>::max())
        {
            nTemp    /= 10;
            nFigures /= 10;
        }

        nValue   = static_cast<uint64_t>(nTemp);

        return *this;
    }



    precision_t& operator/=(const uint128_t& value)
    {
        nValue /= value;

        return *this;
    }


    precision_t& operator*=(const uint64_t& value)
    {
        nValue *= value;

        return *this;
    }


    precision_t operator/(const precision_t& value) const
    {
        precision_t ret = *this;

        uint128_t nTemp = nValue;

        nTemp     *= value.nFigures;
        nTemp     /= value.nValue;

        ret.nValue = static_cast<uint64_t>(nTemp);

        return ret;
    }



    precision_t operator/(const uint128_t& value) const
    {
        precision_t ret = *this;

        ret.nValue /= value;

        return ret;
    }


    precision_t operator*(const precision_t& value) const
    {
        precision_t ret = *this;

        uint128_t nTemp = nValue;

        nTemp *= value.nValue;
        nTemp /= value.nFigures;

        while(nTemp > std::numeric_limits<uint64_t>::max())
        {
            //uint128_t nDiff = nTemp;

            //uint64_t nRound = (nDiff - nTemp);
            nTemp    /= 10;

            //debug::log(0, "Rounding ", nRound);
            ret.nFigures /= 10;
        }

        ret.nValue = static_cast<uint64_t>(nTemp);

        return ret;
    }

    precision_t operator*(const uint32_t& value) const
    {
        precision_t ret = *this;

        ret.nValue *= value;

        return ret;
    }


    precision_t operator^(const precision_t& value) const
    {
        precision_t ret = *this;
        if(value == 0)
            return 1;

        for(int i = 1; value > i; ++i)
        {
            //if(fPrint)
            ret *= (*this);
        }
        //handle the floating point (exp by squaring)

        return ret;
    }


    double get() const
    {
        return nValue / static_cast<double>(nFigures);
    }


    uint192_t getint() const
    {
        return nValue;
    }

    uint64_t get_uint(uint32_t nTotalFigures)
    {
        uint64_t nRet = nValue;
        for(int i = 0; i < (FIGURES - nTotalFigures); i++)
            nRet /= 10;

        return nRet;
    }


    void print() const
    {
        printf("%.16f FIGURES %lu\n", get(), nFigures);
    }


    friend std::ostream &operator<<(std::ostream &o, const precision_t &value)
    {
        //uint32_t nFigures = std::min(12u, FIGURES);
        //o << std::fixed << std::setprecision(nFigures) << value.get();
        o << std::fixed << std::setprecision(12) << value.get();
        return o;
    }
};


precision_t exp(const precision_t& base)
{
    uint128_t fact = 1;

    precision_t power  = base;

    precision_t ret = 0;
    for(uint32_t i = 1; i <= 40; ++i)
    {

        if(power.nFigures < 100)
            return ret;

        if(base.nFigures < 100)
            return ret;

        if(ret.nFigures < 100)
            return ret;


        if(i == 1)
            power = 1;
        else if(i == 2)
            power = base;
        else
            power *= base;

        if(fact == 0)
            return ret;

        precision_t temp = (power / fact);
        if(temp == 0)
            break;

        ret += temp;

        fact *= i;
    }

    return ret;
}


#include <TAO/Ledger/include/supply.h>


const precision_t decays[3][3] =
{
    {25.0, 0.00000110, 0.500},
    {5.00, 0.00000055, 0.500},
    {0.50, 0.00000059, 0.016}
};


uint64_t Subsidy(const uint32_t nMinutes, const uint32_t nType, bool fPrint = false)
{
    precision_t exponent = exp(decays[nType][1] * nMinutes);
    if(exponent == 0)
        return 500000;

    precision_t ret = decays[nType][0] / exponent;
    ret += decays[nType][2];

    return ret.get_uint(6);
}


//This main function is for prototyping new code
//It is accessed by compiling with LIVE_TESTS=1
//Prototype code and it's tests created here should move to production code and unit tests
int main(int argc, char** argv)
{
    using namespace TAO::Ledger;

//2.7182818284590452353602874713527

    debug::log(0, "Chain Age ", GetChainAge(time(NULL)));

    uint32_t nFails = 0;
    uint32_t nTotals = 100000000;

    runtime::timer timer;
    timer.Start();
    for(int i = 5000000; i < nTotals; i++)
    {
        if(i % 100000 == 0)
        {
            uint32_t nMS = timer.ElapsedMilliseconds();

            debug::log(0, i, " Time ", nMS, " ", 100000000 / (nMS == 0 ? 1 : nMS), " per second");
            timer.Reset();
        }

        //debug::log(0, i);
        //if(GetSubsidy(i, 0) != Subsidy(i, 0))
        {
            Subsidy(i, 0, false);
            //debug::log(0, "MINUTE ", i);
            //debug::log(0, GetSubsidy(i, 0));
            //debug::log(0, Subsidy(i, 0, false));
            //printf("------------------------------\n");

            ++nFails;

            //return 0;
        }

        //assert(GetSubsidy(i, 1) == Subsidy(i, 1));
        //assert(GetSubsidy(i, 2) == Subsidy(i, 2));
    }

    debug::log(0, "FAILURES: ", nFails, " ", nFails * 100.0 / nTotals, " %");

    return 0;

    //precision_t<9> testing2 = _exp(euler, 3.333333333);

    //testing.print();
    //testing2.print();

    debug::log(0, "------------------------------");

    precision_t base = 5;

    debug::log(0, "base = ", base);

    precision_t expo = base^3.222;

    debug::log(0, "expo = ", expo);

    precision_t exponent = exp(3);
    debug::log(0, "exponent = ", exponent);


    precision_t value = 100.333;
    precision_t value2 = 0.333;



    precision_t value3 = value / value2;
    precision_t value4 = value * value2;

    debug::log(0, "value  = ", value);
    debug::log(0, "value2 = ", value2);
    debug::log(0, "value3 = ", value3);
    debug::log(0, "value4 = ", value4);

    return 0;

    return 0;
}
