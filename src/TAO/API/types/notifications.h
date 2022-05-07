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
         *
         **/
        static void Manager(const int64_t nThread);


        /** Shutdown
         *
         *  Shuts down the current notification systems.
         *
         **/
        static void Shutdown();
    };
}
