/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2021

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

#include <TAO/API/types/commands/system.h>
#include <TAO/API/include/format.h>

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {
        /* Returns local database and other metrics */
        encoding::json System::Metrics(const encoding::json& params, const bool fHelp)
        {
            /* Build json response. */
            encoding::json jRet;

            /* Keep track of global stats. */
            uint64_t nTotalStake = 0;
            uint64_t nTotalTrust = 0;
            uint64_t nTotalTrustKeys  = 0;

            uint64_t nTotalNames = 0;
            uint64_t nTotalGlobalNames = 0;
            uint64_t nTotalNamespacedNames = 0;
            uint64_t nTotalNamespaces = count_registers("namespace");
            uint64_t nTotalAccounts   = count_registers("account");
            uint64_t nTotalTokens     = count_registers("token");

            uint64_t nTotalAppend     = count_registers("append");
            uint64_t nTotalRaw        = count_registers("raw");
            uint64_t nTotalReadOnly   = count_registers("readonly");

            /* Calculate count of all registers */
            uint64_t nTotalRegisters = nTotalNamespaces + nTotalAccounts + nTotalTokens + nTotalAppend + nTotalRaw + nTotalReadOnly;

            /* For list of items we are building. */
            uint64_t nTotalCrypto = 0;
            uint64_t nTotalObjects = 0;
            uint64_t nTotalTokenized = 0;

            /* Special handle if address indexed. */
            if(config::GetBoolArg("-indexaddress"))
            {
                /* Batch read all trust keys. */
                std::vector<std::pair<uint256_t, TAO::Register::Object>> vTrust;
                if(LLD::Register->BatchRead("trust_address", vTrust, -1))
                {
                    /* Check through all trust accounts. */
                    for(auto& object : vTrust)
                    {
                        /* Skip over invalid objects (THIS SHOULD NEVER HAPPEN). */
                        if(!object.second.Parse())
                            continue;

                        /* Increment total register. */
                        ++nTotalRegisters;

                        /* Check stake value over 0. */
                        if(object.second.get<uint64_t>("stake") == 0)
                            continue;

                        /* Update stake amount. */
                        nTotalStake += object.second.get<uint64_t>("stake");
                        nTotalTrust += object.second.get<uint64_t>("trust");
                        nTotalTrustKeys++;
                    }
                }
            }
            else
            {
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

                        /* Increment total register. */
                        ++nTotalRegisters;

                        /* Check stake value over 0. */
                        if(object.get<uint64_t>("stake") == 0)
                            continue;

                        /* Update stake amount. */
                        nTotalStake += object.get<uint64_t>("stake");
                        nTotalTrust += object.get<uint64_t>("trust");
                        nTotalTrustKeys++;
                    }
                }
            }



            /* Special handle if address indexed. */
            if(config::GetBoolArg("-indexaddress"))
            {
                /* Batch read all names. */
                std::vector<std::pair<uint256_t, TAO::Register::Object>> vNames;
                if(LLD::Register->BatchRead("name_address", vNames, -1))
                {
                    /* Check through all names. */
                    for(auto& object : vNames)
                    {
                        /* Skip over invalid objects (THIS SHOULD NEVER HAPPEN). */
                        if(!object.second.Parse())
                            continue;

                        /* Increment total register. */
                        ++nTotalRegisters;

                        /* global */
                        if(object.second.get<std::string>("namespace") == TAO::Register::NAMESPACE::GLOBAL)
                            nTotalGlobalNames++;

                        /* namespaced */
                        else if(object.second.get<std::string>("namespace") != "")
                            nTotalNamespacedNames ++;

                        /* Update count*/
                        nTotalNames++;
                    }
                }
            }
            else
            {
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

                        /* Increment total register. */
                        ++nTotalRegisters;

                        /* global */
                        if(object.get<std::string>("namespace") == TAO::Register::NAMESPACE::GLOBAL)
                            nTotalGlobalNames++;

                        /* namespaced */
                        else if(object.get<std::string>("namespace") != "")
                            nTotalNamespacedNames ++;

                        /* Update count*/
                        nTotalNames++;
                    }
                }
            }



            /* Special handle if address indexed. */
            if(config::GetBoolArg("-indexaddress"))
            {
                /* Batch read all object registers. */
                std::vector<std::pair<uint256_t, TAO::Register::Object>> vObjects;
                if(LLD::Register->BatchRead("object_address", vObjects, -1))
                {
                    /* Check through all names. */
                    for(auto& object : vObjects)
                    {
                        /* Increment total register. */
                        ++nTotalRegisters;

                        /* Check if tokenized*/
                        if(TAO::Register::Address(object.second.hashOwner).IsToken())
                            nTotalTokenized++;

                        /* Update count*/
                        nTotalObjects++;
                    }
                }
            }
            else
            {
                /* Batch read all object registers. */
                std::vector<TAO::Register::Object> vObjects;
                if(LLD::Register->BatchRead("object", vObjects, -1))
                {
                    /* Check through all names. */
                    for(auto& object : vObjects)
                    {
                        /* Increment total register. */
                        ++nTotalRegisters;

                        /* Check if tokenized*/
                        if(TAO::Register::Address(object.hashOwner).IsToken())
                            nTotalTokenized++;

                        /* Update count*/
                        nTotalObjects++;
                    }
                }
            }

            /* Check by unique owners. */
            std::set<uint256_t> setOwners;

            /* Special handle if address indexed. */
            if(config::GetBoolArg("-indexaddress"))
            {
                /* Batch read all object registers. */
                std::vector<std::pair<uint256_t, TAO::Register::Object>> vCrypto;
                if(LLD::Register->BatchRead("crypto_address", vCrypto, -1))
                {
                    /* Check through all names. */
                    for(auto& rObject : vCrypto)
                    {
                        /* Increment total register. */
                        ++nTotalRegisters;

                        /* Use a set to get unique owners. */
                        setOwners.insert(rObject.second.hashOwner);
                        ++nTotalCrypto;
                    }

                }
            }
            else
            {
                /* Batch read all object registers. */
                std::vector<TAO::Register::Object> vCrypto;
                if(LLD::Register->BatchRead("crypto", vCrypto, -1))
                {
                    /* Check through all names. */
                    for(auto& rObject : vCrypto)
                    {
                        /* Increment total register. */
                        ++nTotalRegisters;

                        /* Use a set to get unique owners. */
                        setOwners.insert(rObject.hashOwner);
                        ++nTotalCrypto;
                    }
                }
            }


            /* Add register metrics */
            const encoding::json jRegisters =
            {
                { "total", nTotalRegisters },
                { "names",
                    {
                        { "global",    nTotalGlobalNames      },
                        { "local",     nTotalNames            },
                        { "namespaced", nTotalNamespacedNames }
                    }
                },

                { "namespaces", nTotalNamespaces },

                /* Track our total objects. */
                { "objects",
                    {
                        { "accounts",  nTotalAccounts  },
                        { "assets",    nTotalObjects   },
                        { "crypto",    nTotalCrypto    },
                        { "tokenized", nTotalTokenized },
                        { "tokens",    nTotalTokens    }
                    }
                },

                /* Track our state registers. */
                { "state",
                    {
                        { "raw", nTotalRaw           },
                        { "readonly", nTotalReadOnly }
                    }
                }
            };

            /* Add to the array key now. */
            jRet["registers"] = jRegisters;

            /* Add sig chain metrics */
            jRet["sigchains"] = setOwners.size();

            /* We only need supply data when on a public network or testnet, private and hybrid do not have supply. */
            if(!config::fHybrid.load())
            {
                /* Add trust metrics */
                encoding::json jTrust;
                jTrust["total"]  = nTotalTrustKeys;
                jTrust["stake"]  = FormatBalance(nTotalStake);
                jTrust["trust"]  = nTotalTrust;

                jRet["trust"] = jTrust;

                /* Add reserves */
                encoding::json jReserves;

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

                jReserves["ambassador"] = double(nAmbassador) / TAO::Ledger::NXS_COIN;
                jReserves["developer"] = double(nDeveloper) / TAO::Ledger::NXS_COIN;
                jReserves["fee"] =  double(nFee) / TAO::Ledger::NXS_COIN ;
                jReserves["hash"] = fHasHash ? double(lastHashBlockState.nReleasedReserve[0]) / TAO::Ledger::NXS_COIN : 0;
                jReserves["prime"] = fHasPrime ? double(lastPrimeBlockState.nReleasedReserve[0]) / TAO::Ledger::NXS_COIN : 0;
                jRet["reserves"] = jReserves;
            }

            return jRet;
        }


        /* Returns the count of registers of the given type in the register DB */
        uint64_t System::count_registers(const std::string& strType)
        {
            /* Special handle if address indexed. */
            if(config::GetBoolArg("-indexaddress"))
            {
                /* Batch read up to 1000 at a time */
                std::vector<std::pair<uint256_t, TAO::Register::Object>> vRegisters;
                LLD::Register->BatchRead(strType + "_address", vRegisters, -1);

                return vRegisters.size();
            }

            /* Batch read all registers . */
            std::vector<TAO::Register::Object> vRegisters;
            LLD::Register->BatchRead(strType, vRegisters, -1);

            return vRegisters.size();
        }
    }
}
