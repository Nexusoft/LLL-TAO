/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/Ledger/include/constants.h>

#include <TAO/Register/types/object.h>

#include <TAO/API/types/system.h>

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {
        /* Validates a register / legacy address */
        json::json System::GetTrustInfo(const json::json& params, bool fHelp)
        {
            if(fHelp || params.size() != 0)
                return std::string("get/trustinfo: no parameters required");

            /* Build json response. */
            json::json jsonRet;

            /* Keep track of global stats. */
            uint64_t nTotalStake = 0;
            uint64_t nTotalTrust = 0;
            uint64_t nTotalKeys  = 0;

            /* Keep track of unique genesis. */
            std::set<uint256_t> setUnique;

            /* Batch read all trust keys. */
            std::vector<TAO::Register::Object> vTrust;
            if(LLD::Register->BatchRead("trust", vTrust, -1))
            {
                /* Check through all trust accounts. */
                for(auto& object : vTrust)
                {
                    /* Skip over duplicates. */
                    if(setUnique.count(object.hashOwner))
                        continue;

                    /* Skip over invalid objects (THIS SHOULD NEVER HAPPEN). */
                    if(!object.Parse())
                        continue;

                    /* Check stake value over 0. */
                    if(object.get<uint64_t>("stake") == 0)
                        continue;

                    /* Update stake amount. */
                    nTotalStake += object.get<uint64_t>("stake");
                    nTotalTrust += object.get<uint64_t>("trust");
                    nTotalKeys  ++;

                    setUnique.insert(object.hashOwner);
                }
            }

            /* Update json return values. */
            jsonRet["stake"] = double(nTotalStake / TAO::Ledger::NXS_COIN);
            jsonRet["trust"] = nTotalTrust;
            jsonRet["keys"]  = nTotalKeys;

            return jsonRet;
        }
    }
}
