/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_UTIL_INCLUDE_STRING_H
#define NEXUS_UTIL_INCLUDE_STRING_H

#include <vector>
#include <string>
#include <cstdint>


/** ParseString
 *
 *  Parses a string and tokenizes it into substrings based on the character delimiter.
 *
 *  @param[in] str The string to parse.
 *  @param[in] c The character delimiter.
 *
 *  @param[out] v The vector to store the tokens into.
 *
 **/
void ParseString(const std::string& str, char c, std::vector<std::string>& v);


/** ReplaceAll
 *
 *  Replace all instances in a string.
 *
 *  @param[out] str The string to replace in.
 *  @param[in] strFind The string to find.
 *  @param[in] strReplace The string to replace with.
 *
 **/
void ReplaceAll(std::string& str, const std::string& strFind, const std::string& strReplace);


/** FormatMoney
 *
 *  Take as input an encoded money amount and format it into an output string.
 *
 *  @param[in] n The encoded money amount.
 *  @param[in] fPlus Flag for if plus sign should be output for positive values.
 *  @param[in] COIN_SIZE The magnitude of the coin amount used for decoding
 *                       decimal value from integer.
 *
 *  @return The formatted money string.
 *
 **/
std::string FormatMoney(int64_t n, bool fPlus = false, int64_t COIN_SIZE = 1000000);


/** ParseMoney
 *
 *  Parse the money amount from input string and return the encoded money value.
 *  Return if there were errors or not.
 *
 *  @param[in] pszIn The input string to parse money amount from.
 *  @param[out] nRet The amount encoded from the parsed money string.
 *  @param[in] COIN_SIZE max amount of units of coins to encode.
 *  @param[in] CENT_SIZE max amount of units of cents to encode.
 *
 *  @return True if no errors, false otherwise.
 *
 **/
bool ParseMoney(const char* pszIn, int64_t& nRet, int64_t COIN_SIZE = 1000000, int64_t CENT_SIZE = 10000);


/** ParseMoney
 *
 *  Parse the money amount from input string and return the encoded money value.
 *  Return if there were errors or not.
 *
 *  @param[in] str The input string to parse money amount from.
 *  @param[out] nRet The amount encoded from the parsed money string.
 *
 *  @return True if no errors, false otherwise.
 *
 **/
bool ParseMoney(const std::string& str, int64_t& nRet);


/** Split
 *
 *  Split a string into it's components by delimiter.
 *
 *  @param[in] strInput The input string.
 *  @param[in] strDelimiter The delimeter to seperate at.
 *
 *  @return The vector of the tokenized strings.
 *
 **/
std::vector<std::string> Split(const std::string& strInput, char strDelimiter);


/** Split
 *
 *  Split a string into it's components by delimiter.
 *
 *  @param[in] strInput The input string.
 *  @param[in] strDelimiter The delimeter to seperate at.
 *
 *  @return The vector of the tokenized strings.
 *
 **/
std::vector<std::string> Split(const std::string& strInput, const std::string& strDelimiter);


/** ltrim
*
*  Trims spaces from the left of a std::string.
*
*  @param[in] s The string to be trimmed.
*
*  @return The string with all leading spaces removed.
*
**/
std::string &ltrim(std::string &s);


/** rtrim
*
*  Trims spaces from the right of a std::string.
*
*  @param[in] s The string to be trimmed.
*
*  @return The string with all trailing spaces removed.
*
**/
std::string &rtrim(std::string &s);


/** trim
*
*  Trims spaces from both ends of a std::string.
*
*  @param[in] s The string to be trimmed.
*
*  @return The string with all leading and trailing spaces removed.
*
**/
std::string &trim(std::string &s);


/** EqualsNoCase
*
*  Compares the two string parameters and returns true if they are the same, ignoring the case of each.
*  Uses a lambda function for the character comparison function to pass to std::equal, just so that we can
*  implement this in one function.
*
*  @param[in] str1 First string to be compared.
*  @param[in] str2 Second string to be compared.
*
*  @return true if they are equal, otherwise false.
*
**/
bool EqualsNoCase(const std::string& str1, const std::string& str2);


/** ToLower
*
*  Converts the string parameter to lowercase.  Does not modify the string parameter.
*
*  @param[in] strIn The string to be converted.
*
*  @return A string representing the the lowercase version of the parameter string.
*
**/
std::string ToLower(const std::string& strIn);


/** IsAllDigit
*
*  Checks if all characters in the string are digits.
*
*  @param[in] strIn The string to be checked.
*
*  @return true if all characters in the string are digits.
*
**/
bool IsAllDigit(const std::string& strIn);


/** IsUINT64
*
*  Checks if the string can be cast to a uint64.
*
*  @param[in] strIn The string to be checked.
*
*  @return true if the string can be cast to a uint64.
*
**/
bool IsUINT64(const std::string& strIn);


#endif
