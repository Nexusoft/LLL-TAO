/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/Ledger/include/constants.h>
#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/include/difficulty.h>
#include <TAO/Ledger/include/retarget.h>
#include <TAO/Ledger/include/supply.h>

#include <TAO/Register/types/object.h>

#include <TAO/API/types/system.h>

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {
        /* Returns local database and other metrics */
        json::json System::Metrics(const json::json& params, bool fHelp)
        {
            if(fHelp || params.size() != 0)
                return std::string("get/metrics: no parameters required");

            /* Build json response. */
            json::json jsonRet;

            /* Keep track of global stats. */
            uint64_t nTotalStake = 0;
            uint64_t nTotalTrust = 0;
            uint64_t nTotalTrustKeys  = 0;
            
            uint64_t nTotalNames = 0;
            uint64_t nTotalGlobalNames = 0;
            uint64_t nTotalNamespacedNames = 0;
            uint64_t nTotalNamespaces = count_registers("namespace");
            uint64_t nTotalAccounts = count_registers("account");
            uint64_t nTotalCrypto = count_registers("crypto");
            uint64_t nTotalTokens = count_registers("token");

            uint64_t nTotalAppend = count_registers("append");
            uint64_t nTotalRaw = count_registers("raw");
            uint64_t nTotalReadOnly = count_registers("readonly");
            
            uint64_t nTotalObjects = 0;
            uint64_t nTotalTokenized = 0;

            /* There is one crypto register per sig chain so just counting these is a reliable count of the sig chains */
            uint64_t nTotalSigChains = nTotalCrypto;


            /* Batch read all trust keys. */
            std::vector<TAO::Register::Object> vTrust;
            if(LLD::Register->BatchRead("trust", vTrust, -1))
            {
                /* Check through all trust accounts. */
                for(auto& object : vTrust)
                {
                    /* Skip over invalid objects (THIS SHOULD NEVER HAPPEN). */
                    if(!object.Parse())
                        continue;

                    /* Check stake value over 0. */
                    if(object.get<uint64_t>("stake") == 0)
                        continue;

                    /* Update stake amount. */
                    nTotalStake += object.get<uint64_t>("stake");
                    nTotalTrust += object.get<uint64_t>("trust");
                    nTotalTrustKeys  ++;

                }
            }

            /* Batch read all names. */
            std::vector<TAO::Register::Object> vNames;
            if(LLD::Register->BatchRead("name", vNames, -1))
            {
                /* Check through all names. */
                for(auto& object : vNames)
                {
                    /* Skip over invalid objects (THIS SHOULD NEVER HAPPEN). */
                    if(!object.Parse())
                        continue;

                    /* global */
                    if(object.get<std::string>("namespace") == TAO::Register::NAMESPACE::GLOBAL)
                        nTotalGlobalNames ++;
                    /* namespaced */
                    else if(object.get<std::string>("namespace") != "" )
                        nTotalNamespacedNames ++;

                    /* Update count*/
                    nTotalNames  ++;

                }
            }

            /* Batch read all object registers. */
            std::vector<TAO::Register::Object> vObjects;
            if(LLD::Register->BatchRead("object", vObjects, -1))
            {
                /* Check through all names. */
                for(auto& object : vObjects)
                {
                    /* Skip over invalid objects (THIS SHOULD NEVER HAPPEN). */
                    if(!object.Parse())
                        continue;

                    /* Check if tokenized*/
                    if(TAO::Register::Address(object.hashOwner).IsToken() )
                        nTotalTokenized ++;

                    /* Update count*/
                    nTotalObjects  ++;
                }
            }

            /* Calculate count of all registers */
            uint64_t nTotalRegisters = nTotalTrustKeys + nTotalNames +nTotalNamespaces + nTotalAccounts + nTotalCrypto
                    + nTotalTokens + nTotalAppend + nTotalRaw + nTotalReadOnly + nTotalObjects;


            /* Add register metrics */
            json::json jsonRegisters;
            jsonRegisters["total"] = nTotalRegisters;
            jsonRegisters["account"] = nTotalAccounts;
            jsonRegisters["append"] = nTotalAppend;
            jsonRegisters["crypto"] = nTotalCrypto;
            jsonRegisters["name"]  = nTotalNames;
            jsonRegisters["name_global"]  = nTotalGlobalNames;
            jsonRegisters["name_namespaced"]  = nTotalNamespacedNames;
            jsonRegisters["namespace"] = nTotalNamespaces;
            jsonRegisters["object"] = nTotalObjects;
            jsonRegisters["object_tokenized"] = nTotalTokenized;
            jsonRegisters["raw"] = nTotalRaw;
            jsonRegisters["readonly"] = nTotalReadOnly;
            jsonRegisters["token"] = nTotalTokens;
            jsonRet["registers"] = jsonRegisters;

            /* Add sig chain metrics */
            jsonRet["sig_chains"] = nTotalSigChains;
            
            /* Add trust metrics */
            json::json jsonTrust;
            jsonTrust["total"]  = nTotalTrustKeys;
            jsonTrust["stake"] = double(nTotalStake / TAO::Ledger::NXS_COIN);
            jsonTrust["trust"] = nTotalTrust;
            jsonRet["trust"] = jsonTrust;

            
            /* Add supply metrics */
            json::json jsonSupply;

            /* Read the stateBest using hashBestChain
             * Cannot use ChainState::stateBest for this because certain fields (like money supply) are not populated */
            TAO::Ledger::BlockState stateBest;
            if(!LLD::Ledger->ReadBlock(TAO::Ledger::ChainState::hashBestChain.load(), stateBest))
                return std::string("Block not found");
            
            uint32_t nMinutes = TAO::Ledger::GetChainAge(stateBest.GetBlockTime());
            int64_t nSupply = stateBest.nMoneySupply;
            int64_t nTarget = TAO::Ledger::CompoundSubsidy(nMinutes);

            jsonSupply["total"] = double(nSupply) / TAO::Ledger::NXS_COIN;
            jsonSupply["target"] = double(nTarget) / TAO::Ledger::NXS_COIN;
            jsonSupply["inflationrate"] = ((nSupply * 100.0) / nTarget) - 100.0;

            jsonSupply["minute"] = double(TAO::Ledger::SubsidyInterval(nMinutes, 1)) / TAO::Ledger::NXS_COIN; //1
            jsonSupply["hour"] = double(TAO::Ledger::SubsidyInterval(nMinutes, 60)) / TAO::Ledger::NXS_COIN; //60
            jsonSupply["day"] = double(TAO::Ledger::SubsidyInterval(nMinutes, 1440)) / TAO::Ledger::NXS_COIN; //1440
            jsonSupply["week"] = double(TAO::Ledger::SubsidyInterval(nMinutes, 10080)) / TAO::Ledger::NXS_COIN;//10080
            jsonSupply["month"] = double(TAO::Ledger::SubsidyInterval(nMinutes, 40320)) / TAO::Ledger::NXS_COIN; //40320
            //jsonSupply["year"] = double(TAO::Ledger::SubsidyInterval(nMinutes, 524160)) / TAO::Ledger::NXS_COIN; //524160
            jsonRet["supply"] = jsonSupply;

            /* Add reserves */
            json::json jsonReserves;

            TAO::Ledger::BlockState lastStakeBlockState = TAO::Ledger::ChainState::stateBest.load();
            bool fHasStake = TAO::Ledger::GetLastState(lastStakeBlockState, 0);
            
            TAO::Ledger::BlockState lastPrimeBlockState = TAO::Ledger::ChainState::stateBest.load();
            bool fHasPrime = TAO::Ledger::GetLastState(lastPrimeBlockState, 1);

            TAO::Ledger::BlockState lastHashBlockState = TAO::Ledger::ChainState::stateBest.load();
            bool fHasHash = TAO::Ledger::GetLastState(lastHashBlockState, 2);

            /* for the ambassador/dev/fee reserves we need to add together the reserves from the last block from each of the respective channels*/
            uint64_t nAmbassador = (fHasPrime ? lastPrimeBlockState.nReleasedReserve[1] : 0) + (fHasHash ? lastHashBlockState.nReleasedReserve[1] : 0);
            uint64_t nDeveloper = (fHasPrime ? lastPrimeBlockState.nReleasedReserve[2] : 0) + (fHasHash ? lastHashBlockState.nReleasedReserve[2] : 0);
            uint64_t nFee = (fHasStake ? lastStakeBlockState.nFeeReserve : 0) + (fHasPrime ? lastPrimeBlockState.nFeeReserve : 0) + (fHasHash ? lastHashBlockState.nFeeReserve : 0);

            jsonReserves["ambassador"] = double(nAmbassador) / TAO::Ledger::NXS_COIN;
            jsonReserves["developer"] = double(nDeveloper) / TAO::Ledger::NXS_COIN;
            jsonReserves["fee"] =  double(nFee) / TAO::Ledger::NXS_COIN ;
            jsonReserves["hash"] = fHasHash ? double(lastHashBlockState.nReleasedReserve[0]) / TAO::Ledger::NXS_COIN : 0;
            jsonReserves["prime"] = fHasPrime ? double(lastPrimeBlockState.nReleasedReserve[0]) / TAO::Ledger::NXS_COIN : 0;
            jsonRet["reserves"] = jsonReserves;
            

            return jsonRet;
        }


        /* Returns the count of registers of the given type in the register DB */
        uint64_t System::count_registers(const std::string& strType)
        {
            /* Batch read all registers . */
            std::vector<TAO::Register::Object> vRegisters;
            LLD::Register->BatchRead(strType, vRegisters, -1);
            
            return vRegisters.size();
        }
    }
}
