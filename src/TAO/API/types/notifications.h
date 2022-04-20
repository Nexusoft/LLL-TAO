/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2019

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To The Voice of The People

____________________________________________________________________________________________*/

#pragma once

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
         **/
        static void Manager();


        /** Shutdown
         *
         *  Shuts down the current notification systems.
         *
         **/
        static void Shutdown();
    }
}
