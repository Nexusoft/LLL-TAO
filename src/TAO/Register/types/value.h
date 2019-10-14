/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_TAO_REGISTER_INCLUDE_VALUE_H
#define NEXUS_TAO_REGISTER_INCLUDE_VALUE_H

#include <vector>
#include <inttypes.h>

namespace TAO
{

    namespace Register
    {

        /** Value
         *
         *  An object that keeps track of a values positions in the register based VM.
         *
         **/
        class Value
        {
        public:

            /** The memory starting value. **/
            uint32_t nBegin;


            /** The memory ending value. **/
            uint32_t nEnd;


            /** The bytes allocated in this register value. **/
            uint16_t nBytes;


            /** Default constructor. **/
            Value()
            : nBegin(0)
            , nEnd(0)
            {
            }


            /** Size (in 64-bit chunks) of value. **/
            uint32_t size() const
            {
                return nEnd - nBegin;
            }


            /** Flag to check for null value. **/
            bool null() const
            {
                return nBegin == 0 && nEnd == 0;
            }
        };
    }
}

#endif
