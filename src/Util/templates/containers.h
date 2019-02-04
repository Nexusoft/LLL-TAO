/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_UTIL_TEMPLATES_CONTAINERS_H
#define NEXUS_UTIL_TEMPLATES_CONTAINERS_H

#include <vector>
#include <map>
#include <cmath>


/**	CMajority
 *
 *  Filter designed to give the majority of set of values.
 *  Keeps count of every addition of template parameter type,
 *  in order to give a reasonable majority of votes.
 *
 **/
template <typename CType>
class CMajority
{
private:
    std::map<CType, int> mapList;
    uint32_t nSamples;

public:


    /**	CMajority
     *
     *  Default constructor
     *
     **/
    CMajority() : nSamples(0) {}


    /**	Add
     *
     *  Add another Element to the Majority Count.
     *
     *  @param[in] value Element to add
     *
     **/
    void Add(CType value)
    {
        if(!mapList.count(value))
            mapList[value] = 1;
        else
            mapList[value]++;

        nSamples++;
    }


    /**	Samples
     *
     *  Return the total number of samples this container holds.
     *
     *  @return Total number of samples
     *
     **/
    uint32_t Samples()
    {
        return nSamples;
    }


    /** Clear
     *
     *  Remove all contents from majority.
     *
     **/
     void clear()
     {
         mapList.clear();
         nSamples = 0;
     }


    /**	Majority
     *
     *  Return the Element of CType with the highest Majority.
     *
     *  @return Element of CType with highest Majority.
     *
     **/
    CType Majority()
    {
        if(nSamples == 0)
            return 0;

        /* Temporary Reference Variable to store the largest majority to then compare every element of the map to it. */
        std::pair<CType, int> nMajority;


        for(typename std::map<CType, int>::iterator nIterator = mapList.begin(); nIterator != mapList.end(); ++nIterator)
        {
            /* Set the return to be the first element, to then compare the rest of the map to it. */
            if(nIterator == mapList.begin())
            {
                nMajority = std::make_pair(nIterator->first, nIterator->second);
                continue;
            }

            /* If a record has higher count, use that one. */
            if(nIterator->second > nMajority.second)
            {
                nMajority.first = nIterator->first;
                nMajority.second = nIterator->second;
            }
        }

        return nMajority.first;
    }
};


/** CAverage
 *
 *  Filter used to give the Numerical Average over given set of values.
 *
 * **/
template <typename T> class CAverage
{
private:
    std::vector<T> vList;

public:


    /**	CAverage
     *
     *  Default constructor
     *
     **/
    CAverage(){}


    /**	Add
     *
     *  Add another Element to the Average Count.
     *
     *  @param[in] value Element to add
     *
     **/
    void Add(T value)
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
    T Average()
    {
        T average = 0;
        for(int nIndex = 0; nIndex < vList.size(); nIndex++)
            average += vList[nIndex];

        return round(average / (double)vList.size());
    }
};

#endif
