/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2019

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To The Voice of The People

____________________________________________________________________________________________*/

#pragma once

#include <thread>
#include <vector>

/* Global TAO namespace. */
namespace TAO::API
{
    /** @class
     *
     *  This class is responsible for handling all notifications related to given user session.
     *
     **/
    class Notifications
    {
        /** Track the list of threads for processing. **/
        static std::vector<std::thread> vThreads;


    public:

        /** Initialize
         *
         *  Initializes the current notification systems.
         *
         **/
        static void Initialize();


        /** Manager
         *
         *  Handle notification of all events for API.
         *
         *  @param[in] nThread The current thread id we have assigned
         *  @param[in] nThreads The total threads that were allocated for events.
         *
         **/
        static void Manager(const int64_t nThread, const uint64_t nThreads);


        /** Shutdown
         *
         *  Shuts down the current notification systems.
         *
         **/
        static void Shutdown();

    private:

        /** sanitize_contract
        *
        *  Checks that the contract passes both Build() and Execute()
        *
        *  @param[in] rContract The contract to sanitize
        *  @param[out] mapStates map of register states used by Build()
        *
        *  @return True if the contract was sanitized without errors.
        *
        **/
        static bool sanitize_contract(TAO::Operation::Contract &rContract, std::map<uint256_t, TAO::Register::State> &mapStates);


        /** validate_transaction
        *
        *  Validate a transaction by sending it off to a peer, For use in -client mode.
        *
        *  @param[in] tx The transaction to validate
        *  @param[out] nContract ID of the first failed contract
        *
        *  @return True if the transaction was validated without errors, false if an error was encountered.
        *
        **/
        static bool validate_transaction(const TAO::Ledger::Transaction& tx, uint32_t& nContract);

    };
}
