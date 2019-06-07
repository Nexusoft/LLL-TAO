/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <algorithm>
#include <functional>
#include <cctype>
#include <locale>

#include <Util/include/debug.h>
#include <Util/include/convert.h>
#include <Util/include/string.h>


/*  Parses a string and tokenizes it into substrings based on the character delimiter. */
void ParseString(const std::string& str, char c, std::vector<std::string>& v)
{
    if(str.empty())
        return;

    std::string::size_type i1 = 0;
    std::string::size_type i2;
    while(true)
    {
        i2 = str.find(c, i1);
        if(i2 == str.npos)
        {
            v.push_back(str.substr(i1));

            return;
        }
        v.push_back(str.substr(i1, i2-i1));

        i1 = i2 + 1;
    }
}


/*  Replace all instances in a string. */
void ReplaceAll(std::string& str, const std::string& strFind, const std::string& strReplace)
{
    /* Catch any call that would not terminate. */
    if(strFind == strReplace)
        return;

    /* Loop through the source string to replace all instances. */
    std::string::size_type pos = str.find(strFind);
    while(pos != std::string::npos)
    {
        /* Replace the instance found. */
        str.replace(pos, strFind.length(), strReplace);

        /* Seek for another instance. */
        pos = str.find(strFind);
    }
}


/*  Take as input an encoded money amount and format it into an output string. */
std::string FormatMoney(int64_t n, bool fPlus, int64_t COIN_SIZE)
{
    // Note: not using straight sprintf here because we do NOT want
    // localized number formatting.
    int64_t n_abs = (n > 0 ? n : -n);
    int64_t quotient = n_abs / COIN_SIZE;
    int64_t remainder = n_abs % COIN_SIZE;

    std::string str = debug::safe_printstr(quotient, ".", std::setfill('0'), std::setw(6), remainder);

    // Right-trim excess 0's before the decimal point:
    uint64_t nTrim = 0;
    for(uint64_t i = str.size()-1; (str[i] == '0' && isdigit(str[i-2])); --i)
        ++nTrim;

    if(nTrim)
        str.erase(str.size()-nTrim, nTrim);

    if(n < 0)
        str.insert((uint32_t)0, 1, '-');
    else if(fPlus && n > 0)
        str.insert((uint32_t)0, 1, '+');
    return str;
}


/*  Parse the money amount from input string and return the encoded money value.
 *  Return if there were errors or not. */
bool ParseMoney(const char* pszIn, int64_t& nRet, int64_t COIN_SIZE, int64_t CENT_SIZE)
{
    std::string strWhole;
    int64_t nUnits = 0;
    const char* p = pszIn;
    while(isspace(*p))
        p++;
    for(; *p; p++)
    {
        if(*p == '.')
        {
            p++;
            int64_t nMult = CENT_SIZE * 10;
            while(isdigit(*p) && (nMult > 0))
            {
                nUnits += nMult * (*p++ - '0');
                nMult /= 10;
            }

            break;
        }

        if(isspace(*p))
            break;

        if(!isdigit(*p))
            return false;

        strWhole.insert(strWhole.end(), *p);
    }

    for(; *p; p++)
        if(!isspace(*p))
            return false;
    if(strWhole.size() > 10) // guard against 63 bit overflow
        return false;
    if(nUnits < 0 || nUnits > COIN_SIZE)
        return false;

    int64_t nWhole = convert::atoi64(strWhole);
    int64_t nValue = nWhole* COIN_SIZE + nUnits;

    nRet = nValue;
    return true;
}


/*  Parse the money amount from input string and return the encoded money value.
 *  Return if there were errors or not. */
bool ParseMoney(const std::string& str, int64_t& nRet)
{
    return ParseMoney(str.c_str(), nRet);
}


/*  Split a string into it's components by delimiter. */
std::vector<std::string> Split(const std::string& strInput, char strDelimiter)
{
    std::string::size_type nIndex = 0;
    std::string::size_type nFind  = strInput.find(strDelimiter);

    std::vector<std::string> vData;
    while(nFind != std::string::npos)
    {
        vData.push_back(strInput.substr(nIndex, nFind - nIndex));
        nIndex = ++nFind;
        nFind  = strInput.find(strDelimiter, nFind);

        if(nFind == std::string::npos)
            vData.push_back(strInput.substr(nIndex, strInput.length()));
    }

    return vData;
}


/*  Split a string into it's components by delimiter. */
std::vector<std::string> Split(const std::string& strInput, const std::string& strDelimiter)
{
    std::string::size_type nIndex = 0;
    std::string::size_type nFind  = strInput.find(strDelimiter);

    std::vector<std::string> vData;
    while(nFind != std::string::npos)
    {
        vData.push_back(strInput.substr(nIndex, nFind - nIndex));
        nIndex = ++nFind;
        nFind  = strInput.find(strDelimiter, nFind);

        if(nFind == std::string::npos)
            vData.push_back(strInput.substr(nIndex, strInput.length()));
    }

    return vData;
}


/*  Trims spaces from the left of a std::string. */
std::string &ltrim(std::string &s)
{
    s.erase(s.begin(), std::find_if(s.begin(), s.end(),
            std::not1(std::ptr_fun<int, int>(std::isspace))));
    return s;
}



/*  Trims spaces from the right of a std::string. */
std::string &rtrim(std::string &s)
{
    s.erase(std::find_if(s.rbegin(), s.rend(),
            std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
    return s;
}


/*  Trims spaces from both ends of a std::string. */
std::string &trim(std::string &s)
{
    return ltrim(rtrim(s));
}


/*  Compares the two string parameters and returns true if they are the same, ignoring the case of each.
 *  Uses a lambda function for the character comparison function to pass to std::equal, just so that we can
 *  implement this in one function. */
bool EqualsNoCase(const std::string& str1, const std::string& str2)
{
	return ((str1.size() == str2.size()) && std::equal(str1.cbegin(), str1.cend(), str2.cbegin(),
    [](const char & c1, const char & c2)
    {
        return (c1 == c2 || std::toupper(c1) == std::toupper(c2));
	}));
}


/*  Converts the string parameter to lowercase.  Does not modify the string parameter. */
std::string ToLower(const std::string& strIn)
{
    std::string strOut = strIn;
    std::transform(strOut.begin(), strOut.end(), strOut.begin(), ::tolower);
    return strOut;
}


/*  Checks if all characters in the string are digits. */
bool IsAllDigit(const std::string& strIn)
{
    return !strIn.empty() && std::find_if(strIn.begin(), strIn.end(),
    [](char c)
    {
        return !std::isdigit(c);
    }) == strIn.end();
}
