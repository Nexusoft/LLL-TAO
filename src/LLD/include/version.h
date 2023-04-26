/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2019

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To The Voice of The People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLD_INCLUDE_VERSION_H
#define NEXUS_LLD_INCLUDE_VERSION_H

#include <string>

#define DATABASE_MAJOR       0
#define DATABASE_MINOR       1
#define DATABASE_PATCH       1
#define DATABASE_BUILD       0

namespace LLD
{

    /* Used for features in the database. */
    const uint32_t DATABASE_VERSION =
                        1000000 * DATABASE_MAJOR
                      +   10000 * DATABASE_MINOR
                      +     100 * DATABASE_PATCH
                      +       1 * DATABASE_BUILD;


    /* The database type used (Berklee DB or Lower Level Database) */
    #ifdef USE_LLD
    const std::string DATABASE_NAME("LLD");
    #else
    const std::string DATABASE_NAME("BDB");
    #endif

}

#endif
