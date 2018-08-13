/*__________________________________________________________________________________________
 
			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2017] ++
			
			(c) Copyright The Nexus Developers 2014 - 2017
			
			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.
			
			"ad vocem populi" - To The Voice of The People
  
____________________________________________________________________________________________*/

#include "include/global.h"
#include "include/unifiedtime.h"
#include "include/manager.h"

#include "../Util/include/runtime.h"
#include "../Util/include/args.h"
#include "../Util/include/convert.h"
#include "../Util/include/debug.h"

#include "../LLP/include/hosts.h"
#include "../LLP/include/time.h"
#include "../LLP/include/network.h"

namespace Core
{
	
	/* Flag Declarations */
	bool fTimeUnified = false;

	
	/* The Average Offset of all Nodes. */
	int UNIFIED_AVERAGE_OFFSET = 0;
	
	
	/* The Current Record Iterating for Moving Average. */
	int UNIFIED_MOVING_ITERATOR = 0;

	
	/* Unified Time Declarations */
	std::vector<int> UNIFIED_TIME_DATA;
    
	
	/** Gets the UNIX Timestamp from the Nexus Network **/
	uint64 UnifiedTimestamp(bool fMilliseconds){ return Timestamp(fMilliseconds) + (UNIFIED_AVERAGE_OFFSET * (fMilliseconds ? 1000 : 1)); }


	/* Calculate the Average Unified Time. Called after Time Data is Added */
	int GetUnifiedAverage()
	{
		if(UNIFIED_TIME_DATA.empty())
			return UNIFIED_AVERAGE_OFFSET;
			
		int nAverage = 0;
		for(int index = 0; index < std::min(MAX_UNIFIED_SAMPLES, (int)UNIFIED_TIME_DATA.size()); index++)
			nAverage += UNIFIED_TIME_DATA[index];
			
		return std::round(nAverage / (double) std::min(MAX_UNIFIED_SAMPLES, (int)UNIFIED_TIME_DATA.size()));
	}
	

    /** Regulator of the Unified Clock **/
    void ThreadUnifiedSamples(void* parg)
    {
        /** Compile the Seed Nodes into a set of Vectors. **/
		std::vector<LLP::CAddress> SEED_NODES = LLP::DNS_Lookup(fTestNet ? LLP::DNS_SeedNodes_Testnet : LLP::DNS_SeedNodes);
        
		for(int nIndex = 0; nIndex < SEED_NODES.size(); nIndex++)
			pManager->AddAddress(SEED_NODES[nIndex]);
			
        /* If the node happens to be offline, wait and recursively attempt to get the DNS seeds. */
        if(SEED_NODES.empty()) {
            Sleep(10000);
            
            fTimeUnified = true;
            ThreadUnifiedSamples(parg);
        }
        
        /** Iterator to be used to ensure every time seed is giving an equal weight towards the Global Seeds. **/
        int nIterator = -1;
        
		
        /** The Entry Client Loop for Core LLP. **/
        std::string ADDRESS = "";
        LLP::TimeOutbound SERVER;
        while(true)
        {
            try
            {
            
                /** Increment the Time Seed Connection Iterator. **/
                nIterator++;
                
                
                /** Reset the ITerator if out of Seeds. **/
                if(nIterator == SEED_NODES.size())
                nIterator = 0;
                    
                    
                /** Connect to the Next Seed in the Iterator. **/
				printf("***** Core LLP: [%u] Attempting Connection %s:9324\n", nIterator, SEED_NODES[nIterator].ToStringIP().c_str());
                SERVER.Connect(SEED_NODES[nIterator].ToStringIP(), "9324", SERVER.IO_SERVICE);
                
                
                /** If the Core LLP isn't connected, Retry in 10 Seconds. **/
                if(!SERVER.Connected())
                {
                    printf("***** Core LLP: Failed To Connect To %s::%s\n", SEED_NODES[nIterator].ToStringIP().c_str(), SERVER.ErrorMessage().c_str());
                    
                    continue;
                }

                
                /** Use a CMajority to Find the Sample with the Most Weight. **/
                CMajority<int> nSamples;
                
                
                /** Get 10 Samples From Server. **/
                SERVER.GetOffset((unsigned int)Timestamp());
                    
                    
                /** Read the Samples from the Server. **/
                while(SERVER.Connected() && !SERVER.Errors() && !SERVER.Timeout(5))
                {
                    Sleep(100);
                
                    SERVER.ReadPacket();
                    if(SERVER.PacketComplete())
                    {
                        LLP::Packet PACKET = SERVER.NewPacket();
                        
                        /** Add a New Sample each Time Packet Arrives. **/
                        if(PACKET.HEADER == SERVER.TIME_OFFSET)
                        {
                            int nOffset = bytes2int(PACKET.DATA);
                            nSamples.Add(nOffset);
                            
                            SERVER.GetOffset((unsigned int)Timestamp());
                            
                            if(GetArg("-verbose", 0) >= 2)
                                printf("***** Core LLP: Added Sample %i | Seed %s\n", nOffset, SEED_NODES[nIterator].ToStringIP().c_str());
                        }
                        
                        SERVER.ResetPacket();
                    }
                    
                    /** Close the Connection Gracefully if Received all Packets. **/
                    if(nSamples.Samples() == 9)
                    {
                        SERVER.Disconnect();
                        break;
                    }
                }
                
                
                /** If there are no Successful Samples, Try another Connection. **/
                if(nSamples.Samples() == 0)
                {
                    printf("***** Core LLP: Failed To Get Time Samples.\n");
                    SERVER.Disconnect();
                    
                    continue;
                }
                
                /* These checks are for after the first time seed has been established. 
                    TODO: Look at the possible attack vector of the first time seed being manipulated.
                            This could be easily done by allowing the time seed to be created by X nodes and then check the drift. */
                if(fTimeUnified)
                {
                
                    /* Check that the samples don't violate time changes too drastically. */
                    if(nSamples.Majority() > GetUnifiedAverage() + MAX_UNIFIED_DRIFT ||
                        nSamples.Majority() < GetUnifiedAverage() - MAX_UNIFIED_DRIFT ) {
                    
                        printf("***** Core LLP: Unified Samples Out of Drift Scope Current (%u) Samples (%u)\n", GetUnifiedAverage(), nSamples.Majority());
                        SERVER.Disconnect();
                    
                        continue;
                    }
                }
                
                /** If the Moving Average is filled with samples, continue iterating to keep it moving. **/
                if(UNIFIED_TIME_DATA.size() >= MAX_UNIFIED_SAMPLES)
                {
                    if(UNIFIED_MOVING_ITERATOR >= MAX_UNIFIED_SAMPLES)
                        UNIFIED_MOVING_ITERATOR = 0;
                                        
                    UNIFIED_TIME_DATA[UNIFIED_MOVING_ITERATOR] = nSamples.Majority();
                    UNIFIED_MOVING_ITERATOR ++;
                }
                    
                    
                /** If The Moving Average is filling, move the iterator along with the Time Data Size. **/
                else
                {
                    UNIFIED_MOVING_ITERATOR = UNIFIED_TIME_DATA.size();
                    UNIFIED_TIME_DATA.push_back(nSamples.Majority());
                }
                

                /** Update Iterators and Flags. **/
                if((UNIFIED_TIME_DATA.size() > 0))
                {
                    fTimeUnified = true;
                    UNIFIED_AVERAGE_OFFSET = GetUnifiedAverage();
                    
                    if(GetArg("-verbose", 0) >= 1)
                        printf("***** %i Iterator | %i Offset | %i Current | %" PRId64 "\n", UNIFIED_MOVING_ITERATOR, nSamples.Majority(), UNIFIED_AVERAGE_OFFSET, UnifiedTimestamp());
                }
                
                /* Sleep for 5 Minutes Between Sample. */
                Sleep(60000);
            }
            catch(std::exception& e){ printf("UTM ERROR: %s\n", e.what()); }
        }
    }
}



