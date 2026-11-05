/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2026

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/
#pragma once

#include <LLC/types/uint1024.h>

#include <TAO/API/include/compare.h>
#include <TAO/API/types/accounts.h>

#include <map>

/* Forward declarations. */
namespace TAO::Register { class Address; }


/* Global TAO namespace. */
namespace TAO::API
{

    /** ListObjects
     *
     *  Scans a signature chain to work out all non-standard object that it owns
     *
     *  @param[in] hashGenesis The genesis hash of the signature chain to scan
     *  @param[out] vObjects The list of object register addresses from sigchain.
     *
     *  @return A vector of register addresses owned by the sig chain
     *
     **/
    bool ListObjects(const uint256_t& hashOwner, std::vector<TAO::Register::Address>& vObjects);


    /** ListAccounts
     *
     *  Scans a signature chain to work out all assets that it owns
     *
     *  @param[in] hashGenesis The genesis hash of the signature chain to scan
     *  @param[in] fTokens If set then this will include tokens in the list
     *  @param[in] fTrust If set then this will include trust accounts in the list
     *  @param[out] vAccounts The list of account register addresses from sigchain.
     *
     *  @return A vector of register addresses owned by the sig chain
     *
     **/
    bool ListAccounts(const uint256_t& hashGenesis, std::vector<TAO::Register::Address>& vAccounts,
                      bool fTokens, bool fTrust);

    /** ListPartial
     *
     *  Lists all object registers partially owned by way of tokens that the sig chain owns
     *
     *  @param[in] hashGenesis The genesis hash of the signature chain to scan
     *  @param[out] mapAssets A compiled map of all accounts supporting given tokenized asset.
     *
     *  @return true if we found partially owned assets
     *
     **/
    bool ListPartial(const uint256_t& hashGenesis, std::map<uint256_t, std::pair<Accounts, uint256_t>> &mapAssets);


    /** ListResults
     *
     *  Take a results set and compile it into a results json object for caching.
     *
     *  @param[in] setResults The results data list that we are compiling into a specific page
     *  @param[out] jRet The paged json data as an array.
     *
     **/
    void ListResults(const std::set<encoding::json, CompareResults>& setResults, encoding::json &jRet);


    /** ListPages
     *
     *  List pages based on our incoming list parameters and generate an output
     *
     *  @param[in] jCache The cached results object that we want to list pages from
     *  @param[in] nLimit The total number of items for this page
     *  @param[in] nOffset The numerical offset that we use to determine page.
     *  @param[out] jRet The paged json data as an array.
     *
     **/
    void ListPages(const encoding::json& jCache, const uint32_t nLimit, const uint32_t nOffset, encoding::json &jRet);

}
