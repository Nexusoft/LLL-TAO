/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once

#include <LLD/cache/template_lru.h>

#include <TAO/API/include/check.h>
#include <TAO/API/include/extract.h>
#include <TAO/API/types/exception.h>

#include <Util/include/json.h>

#include <functional>
#include <map>

/* Global TAO namespace. */
namespace TAO::API
{
    /** Global value to tell cache systems to refresh state. **/
    extern std::atomic<uint32_t> nCacheChain;
    extern std::atomic<uint32_t> nCacheRegister;

    /** ResponseCache
     *
     *  Class to track cached API requests so that we can page and cache them if asked for repeatedly and chain state remains unchanged.
     *
     **/
    class ResponseCache
    {
        /** This just makes our constructors more readable. **/
        static const uint8_t CACHING  = (1 << 1);  //allow caching
        static const uint8_t CHAIN    = (1 << 5);  //reset cache from chain updates

        /** The function pointer to be called. */
        LLD::TemplateLRU<encoding::json, encoding::json> mapCache;


        /** Track if our cache has been refreshed. **/
        std::atomic<uint32_t> nCacheCounter;

    public:


        /** Track our internal settings inhereted from function. **/
        uint8_t nSettings;


        /** Default Constructor. **/
        ResponseCache   (const uint8_t nSettingsIn = 0, const uint32_t nMaxItems = 8)
        : mapCache      (nMaxItems)
        , nCacheCounter (0)
        , nSettings     (nSettingsIn)
        {
        }


        /** Copy Constructor. **/
        ResponseCache(const ResponseCache& a)
        : mapCache      (8)
        , nCacheCounter (a.nCacheCounter.load())
        , nSettings     (a.nSettings)
        {
        }


        /** Move Constructor. **/
        ResponseCache(ResponseCache&& a)
        : mapCache      (8)
        , nCacheCounter (a.nCacheCounter.load())
        , nSettings     (std::move(a.nSettings))
        {
        }


        /** Copy Assignment **/
        ResponseCache& operator=(const ResponseCache& a)
        {
            mapCache      = a.mapCache;
            nCacheCounter = a.nCacheCounter.load();
            nSettings     = a.nSettings;

            *this;
        }

        /** Move Assignment **/
        ResponseCache& operator=(ResponseCache&& a)
        {
            mapCache      = std::move(a.mapCache);
            nCacheCounter = a.nCacheCounter.load();
            nSettings     = std::move(a.nSettings);

            *this;
        }

        /** Default Destructor. **/
        ~ResponseCache()
        {
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
        bool Get(const encoding::json& jParams, encoding::json &jRet)
        {
            /* Check if caching is disabled. */
            if(!(nSettings & CACHING))
                return false;

            /* Check if we need to refresh our cache. */
            if(refresh_cache())
                return false;

            /* Check the list of all of our available caches for this command. */
            const std::vector<encoding::json> vParams = mapCache.Keys();
            for(const auto& jCachedParams : vParams)
            {
                /* Check that we don't have any additional parameters here. */
                //if(jCachedParams.size() != jParams.size())
                //    continue;

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
                    if(jCached.key() == "page" || jCached.key() == "limit" || jCached.key() == "offset" || jCached.key() == "where")
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
                    return mapCache.Get(jCachedParams, jRet);
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
        void Insert(const encoding::json& jParams, const encoding::json& jCache)
        {
            /* Check if caching is disabled. */
            if(!(nSettings & CACHING))
                return;

            /* Make sure our cache is up to date. */
            refresh_cache();

            /* Add to our LRU cache. */
            mapCache.Put(jParams, jCache);
        }

    private:

        /** refresh_cache
         *
         *  Local helper function to check if the cache needs to be refreshed.
         *
         **/
        bool refresh_cache()
        {
            /* First check that our cache is current. */
            bool fCacheRefresh = false;
            if(nSettings & CHAIN)
            {
                /* Check our counter against chain states with this setting. */
                if(nCacheChain.load() != nCacheCounter.load())
                {
                    /* Set that our height has been reached. */
                    nCacheCounter.store(nCacheChain.load());
                    fCacheRefresh = true;
                }
            }
            else
            {
                /* Check our counter against chain states with this setting. */
                if(nCacheRegister.load() != nCacheCounter.load())
                {
                    /* Set that our height has been reached. */
                    nCacheCounter.store(nCacheRegister.load());
                    fCacheRefresh = true;
                }
            }

            /* Wipe our cache if the state needs to update. */
            if(fCacheRefresh)
            {
                /* Clear our map cache LRU. */
                mapCache.Clear();

                return true;
            }

            return false;
        }
    };
}
