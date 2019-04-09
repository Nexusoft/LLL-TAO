/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_TAO_OPERATION_INCLUDE_VALIDATE_H
#define NEXUS_TAO_OPERATION_INCLUDE_VALIDATE_H

#include <TAO/Operation/include/stream.h>

#include <TAO/Register/include/basevm.h>
#include <TAO/Register/include/value.h>

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


            /** Reference of the stream that script exists on. **/
            const Stream& ssOperations;


            /** Reference of the transaction executing script. **/
            const TAO::Ledger::Transaction& tx;


            /** The stream position this script starts on. **/
            const uint64_t nStreamPos;


        public:
            Validate(const Stream& ssOperationIn, const TAO::Ledger::Transaction& txIn, int32_t nLimitsIn = 2048)
            : TAO::Register::BaseVM() //512 bytes of register memory.
            , nLimits(nLimitsIn)
            , ssOperations(ssOperationIn)
            , tx(txIn)
            , nStreamPos(ssOperations.pos())
            {

            }


            /** Copy constructor. **/
            Validate(const Validate& in)
            : TAO::Register::BaseVM(in)
            , nLimits(in.nLimits)
            , ssOperations(in.ssOperations)
            , tx(in.tx)
            , nStreamPos(ssOperations.pos())
            {

            }


            /** Reset
             *
             *  Reset the validation script for re-executing.
             *
             **/
            void Reset()
            {
                ssOperations.reset();
                nLimits = 2048;

                reset();
            }


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
