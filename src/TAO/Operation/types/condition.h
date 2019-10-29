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

#include <stack>

namespace TAO
{

    namespace Operation
    {
        /** Validate
         *
         *  An object to handle the executing of validation scripts.
         *
         **/
        class Condition : public TAO::Register::BaseVM
        {
            /** Reference of the contract being execute. **/
            const Contract& contract;


            /** Reference of the contract that is calling validation. **/
            const Contract& caller;


            /** Build the stack for nested grouping. **/
            std::stack<std::pair<bool, uint8_t>> vEvaluate;


            /** List of warning flags and stream positions encountered during evaluation **/
            std::vector<std::pair<uint16_t, uint8_t>> vWarnings;


        public:


            /** Computational limits for validation script. **/
            uint64_t nCost;


            /** Default Constructor. **/
            Condition() = delete;


            /** Copy constructor. **/
            Condition(const Condition& condition);


            /** Move constructor. **/
            Condition(Condition&& condition) noexcept;


            /** Copy assignment. **/
            Condition& operator=(const Condition& condition) = delete;


            /** Move assignment. **/
            Condition& operator=(Condition&& condition)      = delete;


            /** Default Destructor **/
            virtual ~Condition();


            /** Default constructor. **/
            Condition(const Contract& contractIn, const Contract& callerIn, const int64_t nCostIn = 0);


            /** Reset
             *
             *  Reset the conditions script for re-executing.
             *
             **/
            void Reset();


            /** Verify
             *
             *  Verifies that the contract conditions script is not malformed and can be executed.
             *
             *  @param[in] contract The Contract to validate.
             *
             *  @return True if the contract conditions can be executed without error.
             **/
            static bool Verify(const Contract& contract);


            /** Verify
             *
             *  Verifies that the contract conditions script is not malformed and can be executed.  Populates vWarnings with
             *  warning flags and stream positions, which the caller can use to identify potential problems such as overflows
             *
             *  @param[in] contract The Contract to validate.
             *  @param[out] vWarnings Vector of warning flags and stream positions encountered during evaluation.  These include  
             *  things such as overflows, which do not impede execution but may not be what the contract builder intended.
             *
             *  @return True if the contract conditions can be executed without error.
             **/
            static bool Verify(const Contract& contract, std::vector<std::pair<uint16_t, uint64_t>> &vWarnings);


            /** Execute
             *
             *  Execute the validation script.
             
             *  @return True if the conditions evaluated to true.
             **/
            bool Execute();


            /** WarningToString
            *
            *  Converts a warning flag to a string
            *
            *  @param[in] nWarning The warning to get the text for
            *
            *  @return The warning string
            *
            **/
            static std::string WarningToString(uint16_t nWarning);

        private:

            /** Execute
             *
             *  Execute the validation script.
             * 
             *  @param[out] vWarnings Vector of warning flags and stream positions encountered during evaluation.  These include  
             *  things such as overflows, which do not impede execution but may not be what the contract builder intended.
             *
             *  @return True if the conditions evaluated to true.
             **/
            bool execute(std::vector<std::pair<uint16_t, uint64_t>> &vWarnings);

            /** evaluate
             *
             *  Evaluates a group within the conditions script.  
             *  
             *  @param[in] vWarnings Bitmask of warning codes encountered during evaluation 
             * 
             *  @return True if the group of conditions at the current position evaluate to true .
             **/
            bool evaluate(uint16_t &nWarnings);


            /** get_value
             *
             *  Get a value from the register virtual machine.
             *
             *  @param[in] vWarnings Bitmask of warning codes encountered whilst retrieving the value
             * 
             *  @return True if the value at the current position was retrieved.
             **/
            bool get_value(TAO::Register::Value& vRet, uint16_t &nWarnings);


        };
    }
}

#endif
