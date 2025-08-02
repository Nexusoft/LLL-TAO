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
#include <TAO/API/include/compare.h>
#include <TAO/API/include/extract.h>
#include <TAO/API/types/exception.h>

#include <Util/include/json.h>

#include <functional>
#include <map>

/* Global TAO namespace. */
namespace TAO::API
{

    /** Global value to tell cache systems when to refresh state. **/
    extern std::atomic<uint32_t> nBlockCounter;
    extern std::atomic<uint32_t> nRegisterCounter;
    extern std::atomic<uint32_t> nTransactionCounter;


    /** Struct to hold our enum values for our function settings. **/
    struct ENABLE
    {
        /** Enum to handle page caching. */
        enum : uint8_t
        {
            CACHING   = (1 << 1), //enable caching of given requests based on parameters
            PAGING    = (1 << 2), //a list where we want to page the results
            SORTING   = (1 << 3), //if we want to allow sorting of the datasets
            FILTERS   = (1 << 4), //if we want to apply filters to our results
            QUERIES   = (1 << 5), //if we want to allow queries to our results
            OPERATORS = (1 << 6), //if we want to allow computing on the dataset
        };
    };


    /** CacheSettings
     *
     *  Initialization structure to make bracket initialization and constructors that instantiate this cache cleaner.
     *
     **/
    struct CacheSettings
    {

        /** Track our internal settings inhereted from function. **/
        const uint8_t nSettings;


        /** The maximum number of items in the cache. **/
        const uint32_t nMaxItems;


        /** If LISTING setting is enabled, we need a default sort field. */
        const std::string strDefaultColumn;


        /** Track a reference of our external counter. */
        const std::atomic<uint32_t>* pExternalCounter;


        /** Default Constructor. **/
        CacheSettings()
        : nSettings        (ENABLE::FILTERS) //by default we want to support filters
        , nMaxItems        (0)
        , strDefaultColumn ( )
        , pExternalCounter (nullptr)
        {
        }

        /** Constructor
         *
         *  Set for settings, ordered with counter and then column.
         *
         *  @param[in] nSettingsIn The settings to apply to the cache object
         *  @param[in] pExternalCounterIn The external counter object to determine when to refresh the cache. Default: Registers
         *  @param[in] strColumnIn The column to sort by if sorting is enabled. Default: modified
         *
         **/
        CacheSettings(const uint8_t nSettingsIn, const std::atomic<uint32_t>* pExternalCounterIn = &nRegisterCounter,
                      const std::string& strColumnIn = "modified")
        : nSettings        (nSettingsIn)
        , nMaxItems        (8)
        , strDefaultColumn (strColumnIn)
        , pExternalCounter (pExternalCounterIn)
        {
        }

        /** Constructor
         *
         *  Set for settings, ordered with column and then counter.
         *
         *  @param[in] nSettingsIn The settings to apply to the cache object
         *  @param[in] strColumnIn The column to sort by if sorting is enabled. Default: modified
         *  @param[in] pExternalCounterIn The external counter object to determine when to refresh the cache. Default: Registers
         *
         **/
        CacheSettings(const uint8_t nSettingsIn, const std::string& strColumnIn,
                      const std::atomic<uint32_t>* pExternalCounterIn = &nRegisterCounter)
        : nSettings        (nSettingsIn)
        , nMaxItems        (8)
        , strDefaultColumn (strColumnIn)
        , pExternalCounter (pExternalCounterIn)
        {
        }
    };


    /** ResponseCache
     *
     *  Class to track cached API requests so that we can page and cache them if asked for repeatedly and chain state remains unchanged.
     *
     **/
    class ResponseCache
    {

        /** The function pointer to be called. */
        LLD::TemplateLRU<encoding::json, encoding::json> mapCache;


        /** Track if our cache has been refreshed. **/
        std::atomic<uint32_t> nCacheCounter;


        /** Track a reference of our external counter. */
        const std::atomic<uint32_t>* pExternalCounter;

    public:


        /** Track our internal settings inhereted from function. **/
        uint8_t nSettings;


        /** If LISTING setting is enabled, we need a default sort field. */
        std::string strDefaultColumn;


        ResponseCache() = delete;


        /** Default Constructor. **/
        ResponseCache (const CacheSettings& rSettings)
        : mapCache         (rSettings.nMaxItems)
        , nCacheCounter    (0)
        , pExternalCounter (rSettings.pExternalCounter)
        , nSettings        (rSettings.nSettings)
        , strDefaultColumn (rSettings.strDefaultColumn)
        {
        }


        /** Copy Constructor. **/
        ResponseCache(const ResponseCache& a)
        : mapCache         (a.mapCache)
        , nCacheCounter    (a.nCacheCounter.load())
        , pExternalCounter (a.pExternalCounter)
        , nSettings        (a.nSettings)
        , strDefaultColumn (a.strDefaultColumn)
        {
        }


        /** Move Constructor. **/
        ResponseCache(ResponseCache&& a)
        : mapCache         (std::move(a.mapCache))
        , nCacheCounter    (a.nCacheCounter.load())
        , pExternalCounter (a.pExternalCounter)
        , nSettings        (std::move(a.nSettings))
        , strDefaultColumn (std::move(a.strDefaultColumn))
        {
        }


        /** Copy Assignment **/
        ResponseCache& operator=(const ResponseCache& a)
        {
            mapCache         = a.mapCache;
            nCacheCounter    = a.nCacheCounter.load();
            pExternalCounter = a.pExternalCounter; //we copy the pointer
            nSettings        = a.nSettings;
            strDefaultColumn = a.strDefaultColumn;

            return *this;
        }

        /** Move Assignment **/
        ResponseCache& operator=(ResponseCache&& a)
        {
            mapCache         = std::move(a.mapCache);
            nCacheCounter    = a.nCacheCounter.load();
            pExternalCounter = a.pExternalCounter; //we copy the pointer
            nSettings        = std::move(a.nSettings);
            strDefaultColumn = std::move(a.strDefaultColumn);

            return *this;
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
         *  @param[out] fReversed Determine if the cache needs to be reverse iterated
         *
         *  @return true if cache was found, false if it was not
         *
         **/
        bool Get(const encoding::json& jParams, encoding::json &jRet)
        {
            /* Check if caching is disabled. */
            if(!(nSettings & ENABLE::CACHING))
                return false;

            /* Check if we need to refresh our cache. */
            if(refresh_cache())
                return false; //cache is out of date, we need to re-insert it

            /* Check the list of all of our available caches for this command. */
            const std::vector<encoding::json> vParams = mapCache.Keys();
            for(const auto& jCachedParams : vParams)
            {
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

                    /* Skip over other these additional fields. */
                    if(jCached.key() == "request" || jCached.key() == "order" || jCached.key() == "sort")
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
                    /* Get a copy of our cache here. */
                    if(mapCache.Get(jCachedParams, jRet))
                    {
                        /* Handle here if we need to sort. */
                        if(nSettings & ENABLE::SORTING)
                        {
                            /* Get our previous order and column. */
                            std::string strPrevOrder = "desc", strPrevColumn = strDefaultColumn;
                            ExtractSort(jCachedParams, strPrevOrder, strPrevColumn);

                            /* Get our current order and column. */
                            std::string strOrder = "desc", strColumn = strDefaultColumn;
                            ExtractSort(jParams, strOrder, strColumn);

                            /* Check if our orders have changed. */
                            const bool fOrderChanged =
                                (strOrder != strPrevOrder);

                            /* Check if our columns have changed. */
                            const bool fColumnChanged =
                                (strColumn != strPrevColumn);

                            /* Check if we need to adjust our sorts. */
                            if(fOrderChanged || fColumnChanged)
                            {
                                /* We need to clear out old key since next call will use same sort */
                                mapCache.Remove(jCachedParams);

                                /* Reverse if only the order changed. */
                                if(!fColumnChanged)
                                    std::reverse(jRet.begin(), jRet.end());
                                else //we can sort using our compare functor
                                    sort(strOrder, strColumn, jRet);

                                /* Update our cache with new sorts and parameters now. */
                                mapCache.Put(jParams, jRet);
                            }
                        }

                        return true;
                    }
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
        void Insert(const encoding::json& jParams, encoding::json &jCache)
        {
            /* Check if caching is disabled. */
            if(!(nSettings & ENABLE::CACHING))
                return;

            /* Make sure our cache is up to date. */
            refresh_cache();

            /* Handle here if we need to sort. */
            if(nSettings & ENABLE::SORTING)
            {
                /* Get our current order and column. */
                std::string strOrder = "desc", strColumn = strDefaultColumn;
                ExtractSort(jParams, strOrder, strColumn);

                /* Now we sort the data. */
                sort(strOrder, strColumn, jCache);
            }

            /* Add to our LRU cache. */
            mapCache.Put(jParams, jCache);
        }

    private:

        /** sort
         *
         *  Local helper function to sort our lists.
         *
         *  @param[in] strOrder The order to sort it either ascending or descending
         *  @param[in] strColumn The column we want to sort our data by.
         *  @param[out] jRet The data that we are sorting returned by reference.
         *
         **/
        void sort(const std::string& strOrder, const std::string& strColumn, encoding::json &jRet)
        {
            /* Detect if we are descending or ascending order. */
            const bool fDesc =
                (strOrder == "desc");

            /* Sort using our own lambda function. */
            std::sort(jRet.begin(), jRet.end(), [&](const encoding::json& a, const encoding::json& b)
            {
                /* Check for sort by invalid parameter. */
                if(a.find(strColumn) == a.end() || b.find(strColumn) == b.end())
                    return true;

                /* Handle based on unsigned integer type. */
                if(a[strColumn].is_number_unsigned())
                {
                    /* Grab both of our values. */
                    const uint64_t nA = a[strColumn].get<uint64_t>();
                    const uint64_t nB = b[strColumn].get<uint64_t>();

                    /* Check if they are equal. */
                    if(nA == nB)
                        return false;

                    /* Regular descending sort. */
                    if(fDesc)
                        return nA > nB;

                    /* Ascending sorting. */
                    return nA < nB;
                }

                /* Handle based on signed integer type. */
                else if(a[strColumn].is_number_integer())
                {
                    /* Grab both of our values. */
                    const int64_t nA = a[strColumn].get<int64_t>();
                    const int64_t nB = b[strColumn].get<int64_t>();

                    /* Check if they are equal. */
                    if(nA == nB)
                        return false;

                    /* Regular descending sort. */
                    if(fDesc)
                        return nA > nB;

                    /* Ascending sorting. */
                    return nA < nB;
                }

                /* Handle based on signed integer type. */
                else if(a[strColumn].is_number_float())
                {
                    /* Grab a copy of our doubles here casting to ints at given figures for efficiency. */
                    const double nA = a[strColumn].get<double>();
                    const double nB = b[strColumn].get<double>();

                    /* Check if they are equal. */
                    if(nA == nB)
                        return false;

                    /* Regular descending sort. */
                    if(fDesc)
                        return nA > nB;

                    /* Ascending sorting. */
                    return nA < nB;
                }

                /* Handle based on integer type. */
                else if(a[strColumn].is_string())
                {
                    /* Grab both of our values. */
                    const std::string strA = a[strColumn].get<std::string>();
                    const std::string strB = b[strColumn].get<std::string>();

                    /* Check if they are equal. */
                    if(strA == strB)
                        return false;

                    /* Regular descending sort. */
                    if(fDesc)
                        return strA > strB;

                    /* Ascending sorting. */
                    return strA < strB;
                }

                return false;
            });
        }

        /** refresh_cache
         *
         *  Local helper function to check if the cache needs to be refreshed.
         *
         **/
        bool refresh_cache()
        {
            /* Check if caching is disabled. */
            if(!(nSettings & ENABLE::CACHING))
                return false; //just an extra paranoid check

            /* Check our counter against chain states with this setting. */
            if(pExternalCounter->load() != nCacheCounter.load())
            {
                /* Set that our height has been reached. */
                nCacheCounter.store(pExternalCounter->load());

                /* Clear our map cache LRU. */
                mapCache.Clear();

                return true;
            }

            return false;
        }
    };
}
