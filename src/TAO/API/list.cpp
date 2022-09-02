/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <unordered_set>

#include <TAO/API/include/get.h>
#include <TAO/API/include/list.h>

#include <TAO/API/types/accounts.h>
#include <TAO/API/types/exception.h>

#include <TAO/Operation/include/enum.h>

#include <TAO/Register/types/address.h>
#include <TAO/Register/types/object.h>

#include <TAO/Ledger/types/transaction.h>

#include <LLD/include/global.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Scans a signature chain to work out all assets that it owns */
    bool ListObjects(const uint256_t& hashGenesis, std::vector<TAO::Register::Address>& vObjects)
    {
        /* Get all registers owned by the sig chain */
        std::vector<TAO::Register::Address> vRegisters;
        if(!LLD::Logical->ListRegisters(hashGenesis, vRegisters))
            return false;

        /* Filter out only those that are objects */
        for(const auto& rAddress : vRegisters)
        {
            /* Check that the address is for an object */
            if(rAddress.IsObject())
                vObjects.push_back(rAddress);
        }

        return true;
    }


    /* Scans a signature chain to work out all accounts that it owns */
    bool ListAccounts(const uint256_t& hashGenesis, std::vector<TAO::Register::Address>& vAccounts, bool fTokens, bool fTrust)
    {
        /* Get all registers owned by the sig chain */
        std::vector<TAO::Register::Address> vRegisters;
        if(!LLD::Logical->ListRegisters(hashGenesis, vRegisters))
            return false;

        /* Filter out only those that are accounts */
        for(const auto& rAddress : vRegisters)
        {
            /* Check that the address is for an account or token */
            if(rAddress.IsAccount() || (fTokens && rAddress.IsToken()) || (fTrust && rAddress.IsTrust()))
                vAccounts.push_back(rAddress);
        }

        return true;
    }


    /* Lists all object registers partially owned by way of tokens that the sig chain owns  */
    bool ListPartial(const uint256_t& hashGenesis, std::map<uint256_t, std::pair<Accounts, uint256_t>> &mapAssets)
    {
        /* Get the list of registers owned by this sig chain */
        std::vector<std::pair<uint256_t, uint256_t>> vAddresses;
        LLD::Logical->ListTokenized(hashGenesis, vAddresses);

        /* Check for empty return. */
        if(vAddresses.size() == 0)
            return false;

        /* Add the register data to the response */
        for(const auto& pairTokenized : vAddresses)
        {
            /* Grab our object from disk. */
            TAO::Register::Object oAccount;
            if(!LLD::Register->ReadObject(pairTokenized.first, oAccount))
                continue;

            /* Check for available balance. */
            if(oAccount.get<uint64_t>("balance") == 0)
                continue;

            /* Get a copy of our token-id. */
            const uint256_t hashToken =
                oAccount.get<uint256_t>("token");

            /* Initialize our map if required. */
            if(!mapAssets.count(hashToken))
                mapAssets[hashToken] = std::make_pair(Accounts(GetDecimals(oAccount)), pairTokenized.second);

            /* Add our new value to our map now. */
            mapAssets[hashToken].first.Insert(pairTokenized.first, oAccount.get<uint64_t>("balance"));
        }

        return !mapAssets.empty();
    }
} // End TAO namespace
