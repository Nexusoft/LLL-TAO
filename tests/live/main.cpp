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




template<uint32_t FIGURES>
class precision_t
{
    uint64_t nFigures;
    uint192_t nValue;


    uint64_t get_figures()
    {
        uint64_t nTemp = 10;
        for(int i = 1; i < FIGURES; i++)
            nTemp *= 10;

        return nTemp;
    }


public:
    precision_t()
    : nFigures(get_figures())
    , nValue(0)
    {
    }


    precision_t(double value)
    : nFigures(get_figures())
    , nValue(value * nFigures)
    {
    }


    precision_t& operator=(double value)
    {
        nValue = static_cast<uint192_t>(value * nFigures);

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
        uint192_t compare = static_cast<uint192_t>(value) * nFigures;

        return compare == nValue;
    }


    bool operator==(uint32_t value) const
    {
        uint192_t compare = static_cast<uint192_t>(value) * nFigures;

        return compare == nValue;
    }


    bool operator<(int32_t value) const
    {
        uint192_t compare = static_cast<uint192_t>(value) * nFigures;

        return nValue < compare;
    }


    bool operator>(int32_t value) const
    {
        uint192_t compare = static_cast<uint192_t>(value) * nFigures;

        return nValue > compare;
    }


    precision_t& operator+=(const precision_t& value)
    {
        nValue += value.nValue;

        return *this;
    }


    precision_t& operator-=(const precision_t& value)
    {
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
        nValue     *= value.nFigures;
        nValue     /= value.nValue;

        return *this;
    }


    precision_t& operator*=(const precision_t& value)
    {
        nValue *= value.nValue;
        nValue /= value.nFigures;

        return *this;
    }


    precision_t& operator/=(const uint64_t& value)
    {
        nValue /= value;

        return *this;
    }


    precision_t& operator/=(const uint192_t& value)
    {
        nValue /= value;

        return *this;
    }


    precision_t& operator*=(const uint192_t& value)
    {
        nValue *= value;

        return *this;
    }


    precision_t operator/(const precision_t& value) const
    {
        precision_t ret = *this;

        ret.nValue     *= value.nFigures;
        ret.nValue     /= value.nValue;

        return ret;
    }


    precision_t operator/(const uint192_t& value) const
    {
        precision_t ret = *this;

        ret.nValue /= value;

        return ret;
    }


    precision_t operator*(const precision_t& value) const
    {
        precision_t ret = *this;

        ret.nValue *= value.nValue;
        ret.nValue /= value.nFigures;

        return ret;
    }

    precision_t operator*(const uint32_t& value) const
    {
        precision_t ret = *this;

        ret.nValue *= value;

        return ret;
    }

/*

    // should be much more precise with large b
    inline double fastPrecisePow(double a, double b) {
      // calculate approximation with fraction of the exponent
      int e = (int) b;
      union {
        double d;
        int x[2];
      } u = { a };
      u.x[1] = (int)((b - e) * (u.x[1] - 1072632447) + 1072632447);
      u.x[0] = 0;

      // exponentiation by squaring with the exponent's integer part
      // double r = u.d makes everything much slower, not sure why
      double r = 1.0;
      while (e) {
        if (e & 1) {
          r *= a;
        }
        a *= a;
        e >>= 1;
      }

      return r * u.d;
    }

*/


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


    /*
    Function exp_by_squaring(x, n)
  if n < 0  then return exp_by_squaring(1 / x, -n);
  else if n = 0  then return  1;
  else if n = 1  then return  x ;
  else if n is even  then return exp_by_squaring(x * x,  n / 2);
  else if n is odd  then return x * exp_by_squaring(x * x, (n - 1) / 2);
  */

    precision_t& exp(bool fPrint = false)
    {
        uint192_t fact = 1;

        const precision_t base  = *this;
        precision_t power  = base;

        precision_t temp;

        if(fPrint)
        {
            printf("BASE: ");
            base.print();
        }
        //precision_t temp;


        //double dBase = get();

        *this = 0;
        for(uint32_t i = 1; i <= 40; ++i)
        {
            //double dTemp = pow(dBase, (i - 1)) / fact;

            if(fPrint)
            {
                printf("POWER: ");
                power.print();

                printf("BASE: ");
                base.print();
            }


            if(i == 1)
                power = 1;
            else if(i == 2)
                power = base;
            else
                power *= base;

            // / fact;

            //if(fPrint)
            //    temp.print();

            precision_t temp = (power / fact);
            if(temp == 0)
            {
                if(fPrint)
                    printf("Taylor out at %u\n", i);
                break;
            }



            //nSum += power(x, i - 1) / nFactorial;
            *this += temp;



            fact *= i;
        }

        return *this;
    }


    /*
    precision_t e()
    {
        precision_t fact = 1;

        precision_t ret = 0;
        precision_t one = 1;
        for(int i = 1; i <= 1000; ++i)
        {
            ret += one / fact;
            //nSum += power(x, i - 1) / nFactorial;

            fact *= i;
        }

        return ret;
    }
    */


    double get() const
    {
        return nValue.getdouble() / nFigures;
    }

    uint192_t getint() const
    {
        return nValue;
    }

    uint64_t get_uint(uint32_t nTotalFigures = FIGURES)
    {
        uint192_t nRet = nValue;
        for(int i = 0; i < (FIGURES - nTotalFigures); i++)
        {
            //debug::log(0, i, " UINT ", uint64_t(nRet));

            nRet /= 10;
        }

        return nRet.Get64();
    }


    void print() const
    {
        printf("%.16f\n", get());
    }


    friend std::ostream &operator<<(std::ostream &o, const precision_t &value)
    {
        //uint32_t nFigures = std::min(12u, FIGURES);
        //o << std::fixed << std::setprecision(nFigures) << value.get();
        o << std::fixed << std::setprecision(12) << value.get();
        return o;
    }
};


#include <TAO/Ledger/include/supply.h>


const precision_t<15> decays[3][3] =
{
    {50.0, 0.00000110, 1.000},
    {10.0, 0.00000055, 1.000},
    {1.00, 0.00000059, 0.032}
};


uint64_t Subsidy(const uint32_t nMinutes, const uint32_t nType, bool fPrint = false)
{
    precision_t<15> exponent = decays[nType][1] * nMinutes;
    exponent.exp(fPrint);

    precision_t<15> ret = decays[nType][0] / exponent;
    ret += decays[nType][2];

    ret /= 2;

    if(fPrint)
        ret.print();

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
    uint32_t nTotals = 10000000;
    for(int i = 0; i < nTotals; i++)
    {
        if(i % 10000 == 0)
            printf("%u\n", i);

        //debug::log(0, i);
        if(GetSubsidy(i, 0) != Subsidy(i, 0))
        {
            debug::log(0, "MINUTE ", i);
            debug::log(0, GetSubsidy(i, 0));
            debug::log(0, Subsidy(i, 0, true));
            printf("------------------------------\n");

            ++nFails;

            return 0;
        }

        assert(GetSubsidy(i, 1) == Subsidy(i, 1));
        assert(GetSubsidy(i, 2) == Subsidy(i, 2));
    }

    debug::log(0, "FAILURES: ", nFails, " ", nFails * 100.0 / nTotals, " %");

    return 0;

    //precision_t<9> testing2 = _exp(euler, 3.333333333);

    //testing.print();
    //testing2.print();

    debug::log(0, "------------------------------");

    precision_t<9> base = 5;

    debug::log(0, "base = ", base);

    precision_t<9> expo = base^3.222;

    debug::log(0, "expo = ", expo);

    precision_t<12> exponent = 3;
    exponent.exp();

    debug::log(0, "exponent = ", exponent);


    precision_t<6> value = 100.333;
    precision_t<6> value2 = 0.333;



    precision_t<6> value3 = value / value2;
    precision_t<6> value4 = value * value2;

    debug::log(0, "value  = ", value);
    debug::log(0, "value2 = ", value2);
    debug::log(0, "value3 = ", value3);
    debug::log(0, "value4 = ", value4);

    return 0;

    using namespace TAO::Register;

    Object object;
    object << std::string("byte") << uint8_t(TYPES::MUTABLE) << uint8_t(TYPES::UINT8_T) << uint8_t(55)
           << std::string("test") << uint8_t(TYPES::MUTABLE) << uint8_t(TYPES::STRING) << std::string("this string")
           << std::string("bytes") << uint8_t(TYPES::MUTABLE) << uint8_t(TYPES::BYTES) << std::vector<uint8_t>(10, 0xff)
           << std::string("balance") << uint8_t(TYPES::UINT64_T) << uint64_t(55)
           << std::string("identifier") << uint8_t(TYPES::STRING) << std::string("NXS");


    //unit tests
    uint8_t nTest;
    object.Read("byte", nTest);

    debug::log(0, "TEST ", uint32_t(nTest));

    object.Write("byte", uint8_t(98));

    uint8_t nTest2;
    object.Read("byte", nTest2);

    debug::log(0, "TEST2 ", uint32_t(nTest2));

    std::string strTest;
    object.Read("test", strTest);

    debug::log(0, "STRING ", strTest);

    object.Write("test", std::string("fail"));
    object.Write("test", "fail");

    object.Write("test", std::string("THIS string"));

    std::string strGet;
    object.Read("test", strGet);

    debug::log(0, strGet);

    std::vector<uint8_t> vBytes;
    object.Read("bytes", vBytes);

    debug::log(0, "DATA ", HexStr(vBytes.begin(), vBytes.end()));

    vBytes[0] = 0x00;
    object.Write("bytes", vBytes);

    object.Read("bytes", vBytes);

    debug::log(0, "DATA ", HexStr(vBytes.begin(), vBytes.end()));



    std::string identifier;
    object.Read("identifier", identifier);

    debug::log(0, "Token Type ", identifier);

    return 0;
}
