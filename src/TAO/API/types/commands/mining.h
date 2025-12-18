/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once

#include <TAO/API/types/base.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /** Mining
     *
     *  Mining API Class.
     *  Provides endpoints for mining pool management and discovery.
     *
     **/
    class Mining : public Derived<Mining>
    {
    public:

        /** Default Constructor. **/
        Mining()
        : Derived<Mining>()
        {
        }


        /** Initialize.
         *
         *  Sets the function pointers for this API.
         *
         **/
        void Initialize() final;


        /** Name
         *
         *  Returns the name of this API.
         *
         **/
        static std::string Name()
        {
            return "mining";
        }


        /** ListPools
         *
         *  Returns a list of available mining pools.
         *
         *  @param[in] jParams The parameters from the API call.
         *  @param[in] fHelp Trigger for help data.
         *
         *  @return The return object in JSON.
         *
         **/
        encoding::json ListPools(const encoding::json& jParams, const bool fHelp);


        /** GetPoolStats
         *
         *  Returns current pool statistics for this node (if running as a pool).
         *
         *  @param[in] jParams The parameters from the API call.
         *  @param[in] fHelp Trigger for help data.
         *
         *  @return The return object in JSON.
         *
         **/
        encoding::json GetPoolStats(const encoding::json& jParams, const bool fHelp);


        /** StartPool
         *
         *  Enable pool service and begin broadcasting announcements.
         *
         *  @param[in] jParams The parameters from the API call.
         *  @param[in] fHelp Trigger for help data.
         *
         *  @return The return object in JSON.
         *
         **/
        encoding::json StartPool(const encoding::json& jParams, const bool fHelp);


        /** StopPool
         *
         *  Disable pool service and stop broadcasting announcements.
         *
         *  @param[in] jParams The parameters from the API call.
         *  @param[in] fHelp Trigger for help data.
         *
         *  @return The return object in JSON.
         *
         **/
        encoding::json StopPool(const encoding::json& jParams, const bool fHelp);


        /** GetConnectedMiners
         *
         *  Returns a list of currently connected miners (if running as a pool).
         *
         *  @param[in] jParams The parameters from the API call.
         *  @param[in] fHelp Trigger for help data.
         *
         *  @return The return object in JSON.
         *
         **/
        encoding::json GetConnectedMiners(const encoding::json& jParams, const bool fHelp);
    };

} // namespace TAO::API
