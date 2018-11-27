/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2018

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/


#ifndef NEXUS_UNIFIED_TIME_H
#define NEXUS_UNIFIED_TIME_H

#include <vector>
#include <time.h>

#include "../net/net.h"
#include "../wallet/db.h"


/** Carried on from uint1024.h **/
typedef long long  int64_t;


/** Unified Time Flags **/
extern bool fTimeUnified;
extern bool fIsTimeSeed;
extern bool fIsSeedNode;


/** Offset to be added to your Local Time. This counteracts the effects of changing your clock by will or accident. **/
extern int UNIFIED_LOCAL_OFFSET;


/** Offset calculated from average Unified Offset collected over time. **/
extern int UNIFIED_AVERAGE_OFFSET;


/** Vector to Contain list of Unified Time Offset from Time Seeds, Seed Nodes, and Peers. **/
extern std::vector<int> UNIFIED_TIME_DATA;

extern std::vector<Net::CAddress> SEED_NODES;



/** The Maximum Seconds that a Clock can be Off. This is set to account
    for Network Propogation Times **/
extern int MAX_UNIFIED_DRIFT;


/** Initializes the Unifed Time System.
    A] Checks Database for Offset Average List
    B] Gets Periodic Average of 10 Seeds if first Unified Time **/
void InitializeUnifiedTime();


/** Gets the Current Unified Time Average. The More Data in Time Average, the less
    a pesky node with a manipulated time seed can influence. Keep in mind too that the
    Unified Time will only be updated upon your outgoing connection... otherwise anyone flooding
    network with false time seed will just be ignored. The average is a moving one, so that iter_swap
    will allow clocks that operate under different intervals to always stay synchronized with the network. **/
int GetUnifiedAverage();



/** Unified Time Clock Regulator. Periodically gets Offset from Time Seeds to build a strong Average.
    Checks current time against itself, if there is too much drift, your local offset adjusts to Unified Average. **/
void ThreadUnifiedSamples(void* parg);

std::vector<Net::CAddress> DNS_Lookup(const std::vector<std::string>& DNS_Seed);


#endif
