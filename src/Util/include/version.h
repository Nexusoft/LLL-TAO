/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2017] ++

			(c) Copyright The Nexus Developers 2014 - 2017

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To The Voice of The People

____________________________________________________________________________________________*/

#ifndef NEXUS_UTIL_INCLUDE_VERSION_H
#define NEXUS_UTIL_INCLUDE_VERSION_H


#define CLIENT_MAJOR         0
#define CLIENT_MINOR         3
#define CLIENT_PATCH         0
#define CLIENT_BUILD         0


/* The version of the actual wallet client. */
const int CLIENT_VERSION =
                    1000000 * CLIENT_MAJOR
                  +   10000 * CLIENT_MINOR
                  +     100 * CLIENT_PATCH
                  +       1 * CLIENT_BUILD;

/* Client Version Outputs. */
const std::string CLIENT_NAME("Tritium");
const std::string CLIENT_TYPE("Alpha");
const std::string CLIENT_DATE(__DATE__ " " __TIME__);


#endif
