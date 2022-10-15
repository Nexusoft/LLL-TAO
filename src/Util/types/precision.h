/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once

#include <LLC/types/uint1024.h>

#include <Util/include/debug.h>
#include <Util/include/math.h>

/** @class precision_t
 *
 *  Class to handle multiple precision values using only integers. Does not use floating point algorithms but raw integers.
 *
 **/
class precision_t
{
    /* Try to use a tetra-int from GCC if possible first which is only available for 64-bit processors. */
    #ifdef __SIZEOF_INT128__
        typedef unsigned int compute_t __attribute__((mode(TI)));

    /* Otherwise revert to our standard uint128_t that supports 32-bit processors or incompatible compilers. */
    #else
        typedef uint128_t compute_t;
    #endif


    /** reduction_cast
     *
     *  Case our compute_t type into a 64-bit equivalent.
     *
     *  @param[in] nCompute The value to cast.
     *
     *  @return the casted value in uint64_t form.
     *
     **/
    uint64_t reduction_cast(const compute_t& nCompute) const
    {
        /* We can use a static cast if using GCC tetra-integer. */
        #ifdef __SIZEOF_INT128__
            return static_cast<uint64_t>(nCompute);

        /* Otherwise we need to get the 64-bit integer from our uint128_t class. */
        #else
            return nCompute.Get64();
        #endif
    }

    /** adjust_precision
     *
     *  Adjust our value based on precision difference.
     *
     *  @param[in] DIGITS2 The secondary digits we are converting to.
     *
     **/
    void adjust_precision(const uint32_t nDigitsIn)
    {
        /* Adjust our base precision up if using more figures. */
        if(nDigits > nDigitsIn)
            nValue *= math::pow(10, nDigits - nDigitsIn);

        /* Adjust our base precision down if using less figures. */
        else
            nValue /= math::pow(10, nDigitsIn - nDigits);
    }

public:

    /** adjusted_value
     *
     *  Get the adjusted value for mixed precision.
     *
     *  @param[in] dValueIn The value we are converting to same precision.
     *
     *  @return the adjusted value of mixed precision.
     *
     **/
    uint64_t adjusted_value(const precision_t& dValueIn) const
    {
        /* Check for difference in figures. */
        uint64_t nAdjustedValue = 0;
        if(nDigits > dValueIn.nDigits)
        {
            /* Calculate the figures we need to offset by. */
            const uint64_t nOffset =
                math::pow(10, uint8_t(nDigits - dValueIn.nDigits));

            /* Set our adjusted value now. */
            nAdjustedValue = (dValueIn.nValue * nOffset);
        }
        else
        {
            /* Calculate the figures we need to offset by. */
            const uint64_t nOffset =
                math::pow(10, uint8_t(dValueIn.nDigits - nDigits));

            /* Set our adjusted value now. */
            nAdjustedValue = (dValueIn.nValue / nOffset);
        }

        return nAdjustedValue;
    }

    /** This value tracks the number of internal decimals. **/
    uint8_t nDigits;


    /** This value stores our internal figures for handling our precision. **/
    uint64_t nFigures;


    /** This value stores our internal value that is the decimal represented as an integer. **/
    uint64_t nValue;


    /** We don't have default constructor. **/
    precision_t() = delete;


    /** Constructor
     *
     *  Base constructor with no initial values set.
     *
     **/
    precision_t(const uint8_t nDigitsIn)
    : nDigits  (nDigitsIn)
    , nFigures (math::pow(10, nDigitsIn))
    , nValue   (0)
    {
    }


    /** Copy Constructor
     *
     *  Constructor to copy with mixed precision construction.
     *
     *  @param[in] dValueIn The value we are setting this from.
     *
     **/
    precision_t(const precision_t& dValueIn)
    : nDigits  (dValueIn.nDigits)
    , nFigures (math::pow(10, nDigits))
    , nValue   (dValueIn.nValue)
    {
        /* Adjust our precision now. */
        adjust_precision(dValueIn.nDigits);
    }


    /** Move Constructor
     *
     *  Constructor to copy with mixed precision construction.
     *
     *  @param[in] dValueIn The value we are setting this from.
     *
     **/
    precision_t(precision_t&& dValueIn)
    : nDigits  (std::move(dValueIn.nDigits))
    , nFigures (math::pow(10, nDigits))
    , nValue   (std::move(dValueIn.nValue))
    {
        /* Adjust our precision now. */
        adjust_precision(dValueIn.nDigits);
    }


    /** Copy Assignment
     *
     *  Constructor to copy with mixed precision construction.
     *
     *  @param[in] dValueIn The value we are setting this from.
     *
     **/
    precision_t& operator=(const precision_t& dValueIn)
    {
        /* Set our values from value in. */
        nValue   = dValueIn.nValue;

        /* Adjust our precision now. */
        adjust_precision(dValueIn.nDigits);

        return *this;
    }


    /** Move Assignment
     *
     *  Constructor to move with mixed precision construction.
     *
     *  @param[in] dValueIn The value we are setting this from.
     *
     **/
    precision_t& operator=(precision_t&& dValueIn)
    {
        /* Set our values from value in. */
        nValue   = std::move(dValueIn.nValue);

        /* Adjust our precision now. */
        adjust_precision(dValueIn.nDigits);

        return *this;
    }


    /** Basic Destructor. **/
    ~precision_t()
    {
    }


    /** Constructor
     *
     *  Build a precision value from whole number integer.
     *
     *  @param[in] nValueIn The whole value to input.
     *  @param[in] nDigitsIn The total digits to store as precision.
     *
     **/
    precision_t(const uint64_t nValueIn, const uint8_t nDigitsIn)
    : nDigits  (nDigitsIn)
    , nFigures (math::pow(10, nDigits))
    , nValue   (nValueIn)
    {
    }


    /** Constructor
     *
     *  Constructor to handle construction based on a c++ string.
     *
     *  @param[in] strValueIn The value we are setting this from.
     *  @param[in] fThrow Flag to determine if we should throw if input exceeds supported decimals.
     *
     **/
    precision_t(const double dValueIn, const uint8_t nDigitsIn, const bool fThrow = false)
    : nDigits  (nDigitsIn)
    , nFigures (math::pow(10, nDigits))
    , nValue   (0)
    {
        /* Only need to do this if we are checking precision. */
        if(fThrow)
        {
            /* Calculate our whole number value so we can know how much to adjust precision. */
            const std::string strInt  =
                debug::safe_printstr(static_cast<uint64_t>(dValueIn));

            /* Get string representation of decimals. */
            const std::string strDigits =
                debug::safe_printstr(std::setprecision(nDigits + strInt.length()), dValueIn);

            /* Get the character count. */
            const uint64_t nPos = strDigits.find('.');
            if(nPos != strDigits.npos)
            {
                /* Calculate our input decimals. */
                const uint64_t nDigitsCheck =
                    (strDigits.length() - nPos - 1);

                /* Check if we have correct number of digits. */
                if(nDigitsCheck > nDigits)
                    throw debug::exception(FUNCTION, "Parameter can only have ", nDigits, " decimal places");
            }
        }

        /* Set our return value. */
        nValue = (dValueIn * nFigures);
    }


    /** Constructor
     *
     *  Constructor to handle construction based on a c++ string.
     *
     *  @param[in] strValueIn The value we are setting this from.
     *  @param[in] fThrow Flag to determine if we should throw if input exceeds supported decimals.
     *
     **/
    precision_t(const std::string strValueIn, const uint8_t nDigitsIn, const bool fThrow = false)
    : nDigits  (nDigitsIn)
    , nFigures (math::pow(10, nDigits))
    , nValue   (0)
    {
        /* Get the character count. */
        const uint64_t nPos =
            strValueIn.find('.');

        /* Check that we found decimal. */
        if(nPos != strValueIn.npos)
        {
            /* Calculate our input decimals. */
            const uint64_t nDigitsCheck =
                (strValueIn.length() - nPos - 1);

            /* Check that we are within correct digits. */
            if(nDigitsCheck > nDigits && fThrow)
                throw debug::exception(FUNCTION, "Parameter can only have ", nDigits, " decimal places");
        }

        /* Get our double from the string. */
        const double dValueIn =
            std::stod(strValueIn);

        /* Set our internal values now. */
        nValue = (dValueIn * nFigures);
    }


    /** Constructor
     *
     *  Constructor to handle construction based on a c++ string.
     *
     *  @param[in] strValueIn The value we are setting this from.
     *
     **/
    precision_t(const std::string strValueIn)
    : nDigits  (0)
    , nFigures (1)
    , nValue   (0)
    {
        /* Get the character count. */
        const uint64_t nPos =
            strValueIn.find('.');

        /* Check that we found decimal. */
        if(nPos != strValueIn.npos)
        {
            /* Calculate our input decimals. */
            const uint64_t nDigitsCheck =
                (strValueIn.length() - nPos - 1);

            /* Set our digits now. */
            nDigits  = nDigitsCheck;
            nFigures = math::pow(10, nDigits);
        }

        /* Get our double from the string. */
        const double dValueIn =
            std::stod(strValueIn);

        /* Set our internal values now. */
        nValue = (dValueIn * nFigures);
    }


    /** Constructor
     *
     *  Constructor to handle construction based on a c-style string.
     *
     *  @param[in] chValueIn The value we are setting this from.
     *  @param[in] fThrow Flag to determine if we should throw if input exceeds supported decimals.
     *
     **/
    precision_t(const char* chValueIn, const uint8_t nDigitsIn, const bool fThrow = false)
    : nDigits  (nDigitsIn)
    , nFigures (math::pow(10, nDigits))
    , nValue   (0)
    {
        /* We just wrap our string constructor here. */
        *this =
            precision_t(std::string(chValueIn), nDigitsIn, fThrow);
    }


    /** operator ==
     *
     *  Operator to support equivalent with mixed precisions.
     *
     *  @param[in] dValueIn The value we are comparing to this.
     *
     *  @return True if values match with different precision.
     *
     **/
    bool operator==(const precision_t& dValueIn) const
    {
        return (nValue == adjusted_value(dValueIn));
    }


    /** operator !=
     *
     *  Operator to support not equivalent with mixed precisions.
     *
     *  @param[in] dValueIn The value we are comparing to this.
     *
     *  @return True if values match with different precision.
     *
     **/
    bool operator!=(const precision_t& dValueIn) const
    {
        return (nValue != adjusted_value(dValueIn));
    }


    /** operator >=
     *
     *  Operator to support greater-than or equal with mixed precisions.
     *
     *  @param[in] dValueIn The value we are comparing to this.
     *
     *  @return True if values match with different precision.
     *
     **/
    bool operator>=(const precision_t& dValueIn) const
    {
        return (nValue >= adjusted_value(dValueIn));
    }


    /** operator <=
     *
     *  Operator to support less-than or equal with mixed precisions.
     *
     *  @param[in] dValueIn The value we are comparing to this.
     *
     *  @return True if values match with different precision.
     *
     **/
    bool operator<=(const precision_t& dValueIn) const
    {
        return (nValue <= adjusted_value(dValueIn));
    }


    /** operator <
     *
     *  Operator to support less-than with mixed precisions.
     *
     *  @param[in] dValueIn The value we are comparing to this.
     *
     *  @return True if values match with different precision.
     *
     **/
    bool operator<(const precision_t& dValueIn) const
    {
        return (nValue < adjusted_value(dValueIn));
    }


    /** operator >
     *
     *  Operator to support less-than with mixed precisions.
     *
     *  @param[in] dValueIn The value we are comparing to this.
     *
     *  @return True if values match with different precision.
     *
     **/
    bool operator>(const precision_t& dValueIn) const
    {
        return (nValue > adjusted_value(dValueIn));
    }


    /** operator +
     *
     *  Operator to support addition with mixed precisions.
     *
     *  @param[in] dValueIn The value we are adding to this.
     *
     *  @return A copy of this class with new value.
     *
     **/
    precision_t operator+(const precision_t& dValueIn) const
    {
        /* Check for difference in figures. */
        const uint64_t nAdjustedValue =
            adjusted_value(dValueIn);

        /* Sanity check for overflow. */
        if(nValue > (std::numeric_limits<uint64_t>::max() - nAdjustedValue))
            throw debug::exception(FUNCTION, "cannot overflow value");

        /* Create our return value. */
        precision_t dRet = *this;

        /* Calculate final value from adjusted value. */
        dRet.nValue += nAdjustedValue;

        return dRet;
    }


    /** operator -
     *
     *  Operator to support subtraction with mixed precisions.
     *
     *  @param[in] dValueIn The value we are subtracting from this.
     *
     *  @return A copy of this class with new value.
     *
     **/
    precision_t operator-(const precision_t& dValueIn) const
    {
        /* Check for difference in figures. */
        const uint64_t nAdjustedValue =
            adjusted_value(dValueIn);

        /* Sanity check for underflow. */
        if(nAdjustedValue > nValue)
            throw debug::exception(FUNCTION, "cannot underflow value into negative range");

        /* Create our return value. */
        precision_t dRet = *this;

        /* Calculate final value from adjusted value. */
        dRet.nValue -= nAdjustedValue;

        return dRet;
    }


    /** operator *
     *
     *  Operator to support multiplication with mixed precisions.
     *
     *  @param[in] dValueIn The value we are multiplying to this.
     *
     *  @return A copy of this class with new value.
     *
     **/
    precision_t operator*(const precision_t& dValueIn) const
    {
        /* Check for difference in figures. */
        compute_t nAdjustedValue = (nValue * dValueIn.nValue);
        if(nDigits > dValueIn.nDigits)
        {
            /* Calculate the figures we need to offset by. */
            const uint64_t nOffset =
                math::pow(10, uint8_t(nDigits - dValueIn.nDigits));

            /* Adjust or new value by our offset. */
            nAdjustedValue *= nOffset;
        }
        else
        {
            /* Calculate the figures we need to offset by. */
            const uint64_t nOffset =
                math::pow(10, uint8_t(dValueIn.nDigits - nDigits));

            /* Adjust or new value by our offset. */
            nAdjustedValue /= nOffset;
        }

        /* Set our initial return value. */
        precision_t dRet = precision_t(nDigits);

        /* Shift our value by total figures in return value. */
        nAdjustedValue /= dRet.nFigures;

        /* Check for overflows. */
        if(nAdjustedValue > std::numeric_limits<uint64_t>::max())
            throw debug::exception(FUNCTION, "cannot overflow value");

        /* Set our return value now. */
        dRet.nValue =
            reduction_cast(nAdjustedValue);

        return dRet;
    }


    /** operator *
     *
     *  Operator to support multiplication with mixed precisions.
     *
     *  @param[in] dValueIn The value we are dividing from this.
     *
     *  @return A copy of this class with new value.
     *
     **/
    precision_t operator*(const uint64_t& nProduct) const
    {
        /* Set our initial return value. */
        precision_t dRet = precision_t(nDigits);

        /* Set our return value now. */
        dRet.nValue =
            (nValue * nProduct);

        return dRet;
    }


    /** operator /
     *
     *  Operator to support division with mixed precisions.
     *
     *  @param[in] dValueIn The value we are dividing from this.
     *
     *  @return A copy of this class with new value.
     *
     **/
    precision_t operator/(const precision_t& dValueIn) const
    {
        /* Set our initial return value. */
        precision_t dRet = precision_t(nDigits);

        /* Check for difference in figures. */
        compute_t nAdjustedValue = (nValue * dRet.nFigures);
        if(nDigits > dValueIn.nDigits)
        {
            /* Calculate the figures we need to offset by. */
            const uint64_t nOffset =
                math::pow(10, uint8_t(nDigits - dValueIn.nDigits));

            /* Set our adjusted value now. */
            nAdjustedValue /= (dValueIn.nValue * nOffset);
        }
        else
        {
            /* Calculate the figures we need to offset by. */
            const uint64_t nOffset =
                math::pow(10, uint8_t(dValueIn.nDigits - nDigits));

            /* Set our adjusted value now. */
            nAdjustedValue /= (dValueIn.nValue / nOffset);
        }

        /* Check for overflows. */
        if(nAdjustedValue > std::numeric_limits<uint64_t>::max())
            throw debug::exception(FUNCTION, "cannot overflow value");

        /* Set our return value now. */
        dRet.nValue =
            reduction_cast(nAdjustedValue);

        return dRet;
    }


    /** operator /
     *
     *  Operator to support division with mixed precisions.
     *
     *  @param[in] dValueIn The value we are dividing from this.
     *
     *  @return A copy of this class with new value.
     *
     **/
    precision_t operator/(const uint64_t& nQuotient) const
    {
        /* Set our initial return value. */
        precision_t dRet = precision_t(nDigits);

        /* Set our return value now. */
        dRet.nValue =
            (nValue / nQuotient);

        return dRet;
    }


    /** operator *=
     *
     *  Operator to support '*=' with mixed precisions.
     *
     *  @param[in] dValueIn The value we are multiplying to this.
     *
     *  @return A reference of this class with new value.
     *
     **/
    precision_t& operator*=(const precision_t& dValueIn)
    {
        /* Check for difference in figures. */
        compute_t nAdjustedValue = (nValue * dValueIn.nValue);
        if(nDigits > dValueIn.nDigits)
        {
            /* Calculate the figures we need to offset by. */
            const uint64_t nOffset =
                math::pow(10, uint8_t(nDigits - dValueIn.nDigits));

            /* Adjust or new value by our offset. */
            nAdjustedValue *= nOffset;
        }
        else
        {
            /* Calculate the figures we need to offset by. */
            const uint64_t nOffset =
                math::pow(10, uint8_t(dValueIn.nDigits - nDigits));

            /* Adjust or new value by our offset. */
            nAdjustedValue /= nOffset;
        }

        /* Now shift our value by total figures. */
        nAdjustedValue /= nFigures;

        /* Check for overflows. */
        if(nAdjustedValue > std::numeric_limits<uint64_t>::max())
            throw debug::exception(FUNCTION, "cannot overflow value");

        /* Set our internal value now. */
        nValue =
            reduction_cast(nAdjustedValue);

        return *this;
    }


    /** operator /=
     *
     *  Operator to support '/=' with mixed precisions.
     *
     *  @param[in] dValueIn The value we are dividing from this.
     *
     *  @return A reference of this class with new value.
     *
     **/
    precision_t& operator/=(const precision_t& dValueIn)
    {
        /* Check for difference in figures. */
        compute_t nAdjustedValue = (nValue * nFigures);
        if(nDigits > dValueIn.nDigits)
        {
            /* Calculate the figures we need to offset by. */
            const uint64_t nOffset =
                math::pow(10, uint8_t(nDigits - dValueIn.nDigits));

            /* Now complete our division operation. */
            nAdjustedValue /= (dValueIn.nValue * nOffset);
        }
        else
        {
            /* Calculate the figures we need to offset by. */
            const uint64_t nOffset =
                math::pow(10, uint8_t(dValueIn.nDigits - nDigits));

            /* Now complete our division operation. */
            nAdjustedValue /= (dValueIn.nValue / nOffset);
        }

        /* Check for overflows. */
        if(nAdjustedValue > std::numeric_limits<uint64_t>::max())
            throw debug::exception(FUNCTION, "cannot overflow value");

        /* Set our internal value now. */
        nValue =
            reduction_cast(nAdjustedValue);

        return *this;
    }

    /** operator -=
     *
     *  Operator to support '-=' with mixed precisions.
     *
     *  @param[in] dValueIn The value we are subtracting from this.
     *
     *  @return A reference of this class with new value.
     *
     **/
    precision_t& operator-=(const precision_t& dValueIn)
    {
        /* Check for difference in figures. */
        uint64_t nAdjustedValue = 0;
        if(nDigits > dValueIn.nDigits)
        {
            /* Calculate the figures we need to offset by. */
            const uint64_t nOffset =
                math::pow(10, uint8_t(nDigits - dValueIn.nDigits));

            /* Set our adjusted value now. */
            nAdjustedValue = (dValueIn.nValue * nOffset);
        }
        else
        {
            /* Calculate the figures we need to offset by. */
            const uint64_t nOffset =
                math::pow(10, uint8_t(dValueIn.nDigits - nDigits));

            /* Set our adjusted value now. */
            nAdjustedValue = (dValueIn.nValue / nOffset);
        }

        /* Sanity check for underflow. */
        if(nAdjustedValue > nValue)
            throw debug::exception(FUNCTION, "cannot underflow value into negative range");

        /* Now subtract value from our current value. */
        nValue -=
            nAdjustedValue;

        return *this;
    }


    /** operator +=
     *
     *  Operator to support += with mixed precisions.
     *
     *  @param[in] dValueIn The value we are adding to this.
     *
     *  @return A reference of this class with new value.
     *
     **/
    precision_t& operator+=(const precision_t& dValueIn)
    {
        /* Check for difference in figures where our operating value has more digits. */
        uint64_t nAdjustedValue = 0;
        if(nDigits > dValueIn.nDigits)
        {
            /* Calculate the figures we need to offset by. */
            const uint64_t nOffset =
                math::pow(10, uint8_t(nDigits - dValueIn.nDigits));

            /* Sanity check for overflow. */
            nAdjustedValue = (dValueIn.nValue * nOffset);
        }
        else
        {
            /* Calculate the figures we need to offset by. */
            const uint64_t nOffset =
                math::pow(10, uint8_t(dValueIn.nDigits - nDigits));

            /* Set our adjusted value now. */
            nAdjustedValue = (dValueIn.nValue / nOffset);
        }

        /* Sanity check for overflow. */
        if(nValue > (std::numeric_limits<uint64_t>::max() - nAdjustedValue))
            throw debug::exception(FUNCTION, "cannot overflow value");

        /* Now add value to our current value. */
        nValue +=
            nAdjustedValue;

        return *this;
    }


    /** double_t
     *
     *  Get the value expressed as a double.
     *
     **/
    double double_t() const
    {
        return static_cast<double>(nValue) / nFigures;
    }


    /** float_t
     *
     *  Get the value expressed as a float.
     *
     **/
    float float_t() const
    {
        return static_cast<float>(nValue) / nFigures;
    }


    /** dump
     *
     *  Dump the given value into a string for use in debug output.
     *
     **/
    std::string dump() const
    {
        /* Calculate our whole number value so we can know how much to adjust precision. */
        const std::string strInt  =
            debug::safe_printstr(nValue / nFigures);

        /* Now return with our correct precision. */
        return debug::safe_printstr(std::setprecision(nDigits + strInt.length()), this->double_t());
    }
};
