/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_TAO_OPERATION_TYPES_VALIDATE_H
#define NEXUS_TAO_OPERATION_TYPES_VALIDATE_H

#include <TAO/Operation/types/stream.h>

#include <TAO/Register/types/basevm.h>
#include <TAO/Register/types/value.h>

#include <TAO/Ledger/types/transaction.h>

namespace TAO
{

    namespace Operation
    {

        /** Validate
         *
         *  An object to handle the executing of validation scripts.
         *
         **/
        class Validate : public TAO::Register::BaseVM
        {

            /** Computational limits for validation script. **/
            int32_t nLimits;


            /** Reference of the contract being execute. **/
            const Contract& contract;


            /** Reference of the contract that is calling validation. **/
            const Contract& caller;


        public:

            /** Default constructor. **/
            Validate(const Contract& contractIn, const Contract& callerIn, int32_t nLimitsIn = 2048);


            /** Copy constructor. **/
            Validate(const Validate& in);


            /** Reset
             *
             *  Reset the validation script for re-executing.
             *
             **/
            void Reset();


            /** Execute
             *
             *  Execute the validation script.
             *
             **/
            bool Execute();


            /** GetValue
             *
             *  Get a value from the register virtual machine.
             *
             **/
            bool GetValue(TAO::Register::Value& vRet);
        };
    }
}

#endif
