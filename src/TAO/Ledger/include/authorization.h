/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once

#include <TAO/Register/types/basevm.h>

namespace TAO::Ledger
{

    /** @class Authorization Class:
     *
     *  This class is responsible for handling authentication of a given sigchain. This allows us to create
     *  multiple credentials for our given sigchain and ensure that each set of credentials only has access
     *  to a specific part of our sigchain.
     *
     **/
    class Authorization : public TAO::Register::BaseVM
    {

        /** The main authorization script that will be used to process our authentication in the Ledger VM. **/
    	const TAO::Ledger::Stream ssAuthorization; //this is replacement for hashNext


        /** The evaluation script on incoming transaction will be giving input to execute upon this authorization script. **/
        const TAO::Ledger::Stream ssEvaluation; //this is replacement for vPubKey and vSignature


    	/** Build the stack for nested grouping for unlimited tAuthorizational clauses. **/
    	std::stack<std::pair<bool, uint8_t>> vEvaluate;

    public:

        /** Computational limits for validation script. **/
        uint64_t nCost;


        /** We don't need default constructor because we should always have an auth stream to process. **/
        Authorization() = delete;


        /** Copy constructor. **/
        Authorization(const Authorization& tAuthorization);


        /** Move constructor. **/
        Authorization(Authorization&& tAuthorization) noexcept;


        /** Copy assignment. **/
        Authorization& operator=(const Authorization& tAuthorization) = delete;


        /** Move assignment. **/
        Authorization& operator=(Authorization&& tAuthorization)      = delete;


        /** Default Destructor **/
        virtual ~Authorization();


        /** Default constructor. **/
        Authorization(const TAO::Ledger::Stream& ssAuthorizationIn, const TAO::Ledger::Stream& ssTransactionIn);


        /** Evaluate
         *
         *  Evaluate the validation script.
         *
         **/
        bool Authorized();


    private:

        /** GetValue
         *
         *  Get a value from the register virtual machine.
         *
         **/
         bool get_value(TAO::Register::Value& vRet);
    };
}
