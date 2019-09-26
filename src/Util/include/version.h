/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2019

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To The Voice of The People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_UTIL_INCLUDE_VERSION_H
#define NEXUS_UTIL_INCLUDE_VERSION_H

#include <string>
#include <cstdint>

/* Nexus follows Semantic Versioning 2.0.0 according to this SPEC: https://semver.org/ */
namespace version
{

	/* Major version X (X.y.z | X > 0) MUST be incremented if any backwards
	 * incompatible changes are introduced to the public API. It MAY include
	 * minor and patch level changes. Patch and minor version MUST be reset
	 * to 0 when major version is incremented. */
	extern const uint32_t CLIENT_MAJOR;


	/* Minor version Y (x.Y.z | x > 0) MUST be incremented if new, backwards
	 * compatible functionality is introduced to the public API. It MUST be
	 * incremented if any public API functionality is marked as deprecated.
	 * It MAY be incremented if substantial new functionality or improvements
	 * are introduced within the private code. It MAY include patch level
	 * changes. Patch version MUST be reset to 0 when minor version is incremented. */
	extern const uint32_t CLIENT_MINOR;


	/* Patch version Z (x.y.Z | x > 0) MUST be incremented if only backwards
	 * compatible bug fixes are introduced. A bug fix is defined as an internal
	 * change that fixes incorrect behavior. */
	extern const uint32_t CLIENT_PATCH;


    /* The version of the actual wallet client. */
    extern const uint32_t CLIENT_VERSION;


	/** These external variables are the display only variables. They are used to track the updates of Nexus independent of Database and Protocol Upgrades. **/
	extern const std::string CLIENT_VERSION_BUILD_STRING;
	extern const std::string CLIENT_DATE;


}

#endif
