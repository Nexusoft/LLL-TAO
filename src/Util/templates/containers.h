/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++
            
            (c) Copyright The Nexus Developers 2014 - 2018
            
            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.
            
            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_UTIL_TEMPLATES_CONTAINERS_H
#define NEXUS_UTIL_TEMPLATES_CONTAINERS_H

#include <vector>
#include <map>
#include <math.h>


/**	Filter designed to give the majority of set of values.
 *		Keeps count of every addition of template paramter type, 
 * 	in order to give a reasonable majority of votes. 
 */
template <typename CType> class CMajority
{
private:
    std::map<CType, int> mapList;
    uint32_t nSamples;
    
public:
    CMajority() : nSamples(0) {}
    
    
    /* Add another Element to the Majority Count. */
    void Add(CType value)
    {
        if(!mapList.count(value))
            mapList[value] = 1;
        else
            mapList[value]++;
            
        nSamples++;
    }
    
    
    /* Return the total number of samples this container holds. */
    uint32_t Samples(){ return nSamples; }
    
    
    /* Return the Element of Type that has the highest Majority. */
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


/** Average Filter used to give the Numerical Average over given set of values. **/
template <typename type> class CAverage
{
private:
    std::vector<type> vList;
    
public:
    CAverage(){}
    
    void Add(type value)
    {
        vList.push_back(value);
    }
    
    type Average()
    {
        type average = 0;
        for(int nIndex = 0; nIndex < vList.size(); nIndex++)
            average += vList[nIndex];
            
        return round(average / (double)vList.size());
    }
};

#endif
