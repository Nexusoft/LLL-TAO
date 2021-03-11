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

    /**	majority
     *
     *  Filter designed to give the majority of set of values.
     *  Keeps count of every addition of template parameter type,
     *  in order to give a reasonable majority of votes.
     *
     **/
    template <typename Type>
    class majority
    {
    public:
        std::map<Type, uint32_t> mapList;
        uint32_t nSamples;


        /**	majority
         *
         *  Default constructor
         *
         **/
        majority()
        : nSamples (0)
        {
        }


        /**	Add
         *
         *  Add another Element to the Majority Count.
         *
         *  @param[in] value Element to add
         *
         **/
        void Add(Type value)
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
         *  Return the Element of Type with the highest Majority.
         *
         *  @return Element of Type with highest Majority.
         *
         **/
        Type Majority()
        {
            if(nSamples == 0)
                return 0;

            /* Temporary Reference Variable to store the largest majority to then compare every element of the map to it. */
            std::pair<Type, uint32_t> pairMajority;
            for(typename std::map<Type, uint32_t>::iterator it = mapList.begin(); it != mapList.end(); ++it)
            {
                /* Set the return to be the first element, to then compare the rest of the map to it. */
                if(it == mapList.begin())
                {
                    pairMajority = std::make_pair(it->first, it->second);
                    continue;
                }

                /* If a record has higher count, use that one. */
                if(it->second > pairMajority.second)
                {
                    pairMajority.first = it->first;
                    pairMajority.second = it->second;
                }
            }

            return pairMajority.first;
        }
    };
}
