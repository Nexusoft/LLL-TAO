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


        /** SanitizeContract
         *
         *  Checks that the contract passes both Build() and Execute()
         *
         *  @param[out] rContract The contract to sanitize
         *  @param[out] mapStates map of register states used by Build()
         *
         *  @return True if the contract was sanitized without errors.
         *
         **/
        static bool SanitizeContract(TAO::Operation::Contract &rContract, std::map<uint256_t, TAO::Register::State> &mapStates);


        /** SanitizeContract
         *
         *  Checks that the contract passes both Build() and Execute()
         *
         *  @param[in] hashGenesis The sigchain that is calling to sanitize.
         *  @param[out] rContract The contract to sanitize
         *
         *  @return True if the contract was sanitized without errors.
         *
         **/
        static bool SanitizeContract(const uint256_t& hashGenesis, TAO::Operation::Contract &rContract);


        /** SanitizeUnconfirmed
         *
         *  Checks that the current unconfirmed transactions are in a valid state.
         *
         *  @param[in] hashGenesis The sigchain that is calling to sanitize.
         *  @param[in] jSession The current session data to use in building.
         *
         **/
        static bool SanitizeUnconfirmed(const uint256_t& hashGenesis, const encoding::json& jSession);


    private:


        /** build_notification
         *
         *  Build an contract response to given notificaion.
         *
         *  @param[in] hashGenesis The current user-id we are building for.
         *  @param[in] jSession The current session data to use in building.
         *  @param[in] rEvent The event we are building notification for.
         *  @param[in] fMine Flag to tell if processing our own sends or events.
         *  @param[in] fStop Flag to tell if we have stopped incrementing our sequence.
         *  @param[out] vContracts The list of contracts we are building.
         *
         *  @return true if event did not increment, false if it did increment.
         *
         **/
        static bool build_notification(const uint256_t& hashGenesis, const encoding::json& jSession, const std::pair<uint512_t, uint32_t>& rEvent,
            const bool fMine, const bool fStop, std::vector<TAO::Operation::Contract> &vContracts);

    };
}
