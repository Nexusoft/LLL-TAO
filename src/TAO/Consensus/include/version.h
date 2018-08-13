/*__________________________________________________________________________________________
 
			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2017] ++
			
			(c) Copyright The Nexus Developers 2014 - 2017
			
			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.
			
			"ad vocem populi" - To The Voice of The People
  
____________________________________________________________________________________________*/

#ifndef NEXUS_CORE_INCLUDE_VERSION_H
#define NEXUS_CORE_INCLUDE_VERSION_H

#include <string>

#define DATABASE_MAJOR       0
#define DATABASE_MINOR       1
#define DATABASE_PATCH       1
#define DATABASE_BUILD       0

#define CLIENT_MAJOR         0
#define CLIENT_MINOR         3
#define CLIENT_PATCH         0
#define CLIENT_BUILD         0

#define PROTOCOL_MAJOR       0
#define PROTOCOL_MINOR       2
#define PROTOCOL_REVISION    0
#define PROTOCOL_BUILD       0


/* Current Database Serialization Version. */
extern const int DATABASE_VERSION;
extern const std::string DATABASE_NAME;


/* The current Client Version */
extern const int CLIENT_VERSION;
extern const std::string CLIENT_NAME;
extern const std::string CLIENT_TYPE;
extern const std::string CLIENT_DATE;

	
	
/* Used to Lock-Out Nodes that are running a protocol version that is too old,  
 * Or to allow certain new protocol changes without confusing Old Nodes. */
extern const int MIN_PROTO_VERSION;


/* Minimum Tritium Version. */
extern const int MIN_TRITIUM_VERSION;

	
/* Used to determine the features available in the Nexus Network */
extern const int PROTOCOL_VERSION;


/* Version Specifiers. */
std::string FormatFullVersion();
std::string FormatSubVersion();


#endif
