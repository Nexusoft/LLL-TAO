/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2023

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once

#include <TAO/API/types/exception.h>

#include <Util/include/json.h>
#include <Util/types/precision.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /** @class CompareResults
     *
     *  Basic functor for handling sort operations in generic json lists.
     *
     **/
    class CompareResults final
    {
        /** Flag to determine if order is ascending or descending. **/
        bool fDesc;


        /** String to track which column we are sorting by. **/
        std::string strColumn;

    public:

        /* Default Constructor Disabled. */
        CompareResults() = delete;


        /** Default Constructor
         *
         *  Constructs our functor from an ordering and column to order by.
         *
         *  @param[in] strOrderIn The order we are sorting by.
         *  @param[in] strColumnIn The column that we are sorting by.
         *
         **/
        CompareResults(const std::string& strOrderIn, const std::string& strColumnIn)
        : fDesc     (strOrderIn == "desc")
        , strColumn (strColumnIn)
        {
        }


        /** Default Constructor
         *
         *  Constructs our functor from an ordering and column to order by.
         *
         *  @param[in] fDescIn The order we are sorting by as boolean.
         *  @param[in] strColumnIn The column that we are sorting by.
         *
         **/
        CompareResults(const bool fDescIn, const std::string& strColumnIn)
        : fDesc     (fDescIn)
        , strColumn (strColumnIn)
        {
        }


        /** Function Overload
         *
         *  This is called when sorting a std::set so that we can sort as we construct.
         *
         *  @param[in] a The first value to compare against.
         *  @param[in] b The second value to compare against.
         *
         *  @return true or false against given values.
         *
         **/
        bool operator()(const encoding::json& a, const encoding::json& b) const
        {
            /* Check for nested sorting values and handle recursively. */
            const auto nFind = strColumn.find(".");
            if(nFind != strColumn.npos)
            {
                /* Grab our first value to recurse through. */
                const std::string strNext = strColumn.substr(0, nFind);

                /* Check that we have at least the base type we are recursing. */
                if(a.find(strNext) == a.end() || b.find(strNext) == b.end())
                    return true;

                /* Create a new function object to recursively evaluate. */
                const CompareResults tEvaluate = CompareResults(fDesc, strColumn.substr(nFind + 1));
                return tEvaluate(a[strNext], b[strNext]);
            }

            /* Check for sort by invalid parameter. */
            if(a.find(strColumn) == a.end() || b.find(strColumn) == b.end())
                return true;

            /* Handle based on unsigned integer type. */
            if(a[strColumn].is_number_unsigned())
            {
                /* Grab both of our values. */
                const uint64_t nA = a[strColumn].get<uint64_t>();
                const uint64_t nB = b[strColumn].get<uint64_t>();

                /* Check if they are equal. */
                if(nA == nB)
                    return true;

                /* Regular descending sort. */
                if(fDesc)
                    return nA > nB;

                /* Ascending sorting. */
                return nA < nB;
            }

            /* Handle based on signed integer type. */
            else if(a[strColumn].is_number_integer())
            {
                /* Grab both of our values. */
                const int64_t nA = a[strColumn].get<int64_t>();
                const int64_t nB = b[strColumn].get<int64_t>();

                /* Check if they are equal. */
                if(nA == nB)
                    return true;

                /* Regular descending sort. */
                if(fDesc)
                    return nA > nB;

                /* Ascending sorting. */
                return nA < nB;
            }

            /* Handle based on signed integer type. */
            else if(a[strColumn].is_number_float())
            {
                /* Grab a copy of our doubles here casting to ints at given figures for efficiency. */
                const double nA = a[strColumn].get<double>();
                const double nB = b[strColumn].get<double>();

                /* Check if they are equal. */
                if(nA == nB)
                    return true;

                /* Regular descending sort. */
                if(fDesc)
                    return nA > nB;

                /* Ascending sorting. */
                return nA < nB;
            }

            /* Handle based on integer type. */
            else if(a[strColumn].is_string())
            {
                /* Grab both of our values. */
                const std::string strA = a[strColumn].get<std::string>();
                const std::string strB = b[strColumn].get<std::string>();

                /* Check if they are equal. */
                if(strA == strB)
                    return true;

                /* Regular descending sort. */
                if(fDesc)
                    return strA > strB;

                /* Ascending sorting. */
                return strA < strB;
            }

            throw Exception(-57, "Invalid Parameter [", strColumn, "=", a[strColumn].type_name(), "]");
        }
    };
}
