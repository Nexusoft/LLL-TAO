/*__________________________________________________________________________________________
 
 (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2017] ++
 
 (c) Copyright The Nexus Developers 2014 - 2017
 
 Distributed under the MIT software license, see the accompanying
 file COPYING or http://www.opensource.org/licenses/mit-license.php.
 
 "fides in stellis, virtus in numeris" - Faith in the Stars, Power in Numbers
 
 ____________________________________________________________________________________________*/

#ifndef NEXUS_LLD_TEMPLATES_INITIALIZE_H
#define NEXUS_LLD_TEMPLATES_INITIALIZE_H

namespace LLD
{
    /* Check registered journal db's in the data directory of /data/journal/
     * 
     * Read each jounral database and check that to the core keychain and sector database on initialization
     * Add a database registration system where it scans active databases on initialize
     * TODO: Migrate from keychain.h / keychain.cpp in memory holding structure of keys.
     * this should allow the key databases to stay registered.
     */
}

#endif
