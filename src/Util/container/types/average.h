/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once

#include <vector>
#include <map>
#include <cmath>


namespace util::container
{

    /** average
     *
     *  Filter used to give the Numerical Average over given set of values.
     *
     * **/
    template <typename Type> class average
    {
        std::vector<Type> vList;

    public:


        /**	CAverage
         *
         *  Default constructor
         *
         **/
        average(){}


        /**	Add
         *
         *  Add another Element to the Average Count.
         *
         *  @param[in] value Element to add
         *
         **/
        void add(Type value)
        {
            vList.push_back(value);
        }


        /**	Average
         *
         *  Return an average value of type T.
         *
         *  @return Average value
         *
         **/
        Type average()
        {
            Type nAverage = 0;
            for(int nIndex = 0; nIndex < vList.size(); nIndex++)
                nAverage += vList[nIndex];

            return std::round(nAverage / (double)vList.size());
        }
    };
}
