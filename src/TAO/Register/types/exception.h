/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "If only you knew the magnificence of the 3, 6, and 9, then you
             would have a key to the universe" - Nikola Tesla

____________________________________________________________________________________________*/

#pragma once


#include <Util/include/debug.h>

namespace TAO
{

    namespace Register
    {
        /** BaseVmException
        *
        *  Specialzed Exception so that we can capture exceptions originating from the BaseVM class
        *
        **/
        class BaseVMException : public debug::exception
        {
        public: 

            /** Constructor
             *
             *  @param[in] args The variadic template for initialization.
             *
             **/
            template<class... Args>
            BaseVMException(Args&&... args) : debug::exception(args...){}
        
        };


        /** MalformedException
        *
        *  Used to describe malformed conditions
        *
        **/
        class MalformedException : public TAO::Register::BaseVMException
        {
        public: 

            /** Constructor
             *
             *  @param[in] args The variadic template for initialization.
             *
             **/
            template<class... Args>
            MalformedException(Args&&... args) : BaseVMException(args...){}
        
        };

    } /* End Register Namespace */

} /* End TAO Namespace */