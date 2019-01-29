/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2019

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To The Voice of The People

____________________________________________________________________________________________*/

#ifndef NEXUS_UTIL_INCLUDE_VERSION_H
#define NEXUS_UTIL_INCLUDE_VERSION_H

#include <string>


namespace version
{
	const int CLIENT_MAJOR	= 	0;
	const int CLIENT_MINOR	=	3;
	const int CLIENT_PATCH	=	0;
	const int CLIENT_BUILD	=	0;


	/** These external variables are the display only variables. They are used to track the updates of Nexus independent of Database and Protocol Upgrades. **/
	extern const std::string CLIENT_VERSION_BUILD_STRING;
	extern const std::string CLIENT_DATE;


	/* The version of the actual wallet client. */
	extern const int CLIENT_VERSION;

}

#endif
