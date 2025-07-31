/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once

#include <TAO/Ledger/include/chainstate.h>

#include <TAO/API/include/check.h>
#include <TAO/API/include/extract.h>
#include <TAO/API/types/exception.h>

#include <Util/include/json.h>

#include <functional>
#include <map>

/* Global TAO namespace. */
namespace TAO::API
{
    /** ResponseCache
     *
     *  Class to track cached API requests so that we can page and cache them if asked for repeatedly and chain state remains unchanged.
     *
     **/
    class ResponseCache
    {
        /** The function pointer to be called. */
        std::map<std::string, std::vector<std::pair<encoding::json, encoding::json>>> mapCache;

        /** Track our current block versus our current block cache. **/
        uint1024_t hashLastBlock;

    public:


        /** Default Constructor. **/
        ResponseCache()
        : mapCache      ( )
        , hashLastBlock (0)
        {
        }


        /** Default Destructor. **/
        ~ResponseCache()
        {
            mapCache.clear();
        }


        /** Get
         *
         *  Get the cached request data based on command and parameters.
         *
         *  @param[in] strCommand The command to push data to
         *  @param[in] jParams The json formatted parameters
         *  @param[out] jRet The cached request data
         *
         *  @return true if cache was found, false if it was not
         *
         **/
        bool Get(const std::string& strCommand, const encoding::json& jParams, encoding::json &jRet)
        {
            /* First check that our cache is current. */
            if(hashLastBlock != TAO::Ledger::ChainState::hashBestChain.load())
            {
                /* Set our last block in our cache object. */
                hashLastBlock = TAO::Ledger::ChainState::hashBestChain.load();

                /* Clear our current cache on new block. */
                mapCache.clear();

                return false;
            }

            /* Check if we have this in our current cache. */
            if(!mapCache.count(strCommand))
                return false;

            /* Check the list of all of our available caches for this command. */
            const std::vector<std::pair<encoding::json, encoding::json>> vItems = mapCache[strCommand];
            for(const auto& tItem : vItems)
            {
                /* Grab a copy of our cache parameters. */
                const encoding::json jCachedParams = tItem.first;

                /* Check that we don't have any additional parameters here. */
                if(jCachedParams.size() != jParams.size())
                    continue;

                /* Check our types match here. */
                if(CheckRequest(jCachedParams, "type", "string, array"))
                {
                    /* Check that we have matching types. */
                    if(ExtractTypes(jParams) != ExtractTypes(jCachedParams))
                        continue;
                }

                /* Use this to escape this loop. */
                bool fFound = true;

                /* Decode now if we are paging a list command or if we are just passing back a cached request. */
                for(const auto& jCached : jCachedParams.items())
                {
                    /* Skip over paging information. */
                    if(jCached.key() == "page" || jCached.key() == "limit" || jCached.key() == "offset")
                        continue;

                    /* Skip over other required fields. */
                    if(jCached.key() == "request" || jCached.key() == "pin")
                        continue;

                    /* Check that the types match. */
                    if(jParams.find(jCached.key()) == jParams.end())
                    {
                        fFound = false;
                        break;
                    }

                    /* Check that our values match now. */
                    if(jParams[jCached.key()] != jCached.value())
                    {
                        fFound = false;
                        break;
                    }
                }

                /* Track if we have found our cache object. */
                if(fFound)
                {
                    jRet = tItem.second;
                    return true;
                }
            }

            return false;
        }


        /** Insert
         *
         *  Push data into our cache object.
         *
         *  @param[in] strCommand The command to push data to
         *  @param[in] jParams The json formatted parameters
         *  @param[in] jCache The cached request data to push
         *
         **/
        void Insert(const std::string& strCommand, const encoding::json& jParams, const encoding::json& jCache)
        {
            /* Check if we push to the vector. */
            if(!mapCache.count(strCommand))
                mapCache[strCommand] = std::vector<std::pair<encoding::json, encoding::json>>();

            /* Push the data to the back of the vector. */
            mapCache[strCommand].push_back(std::make_pair(jParams, jCache));
        }
    };
}
