/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2021

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <Util/include/args.h>
#include <Util/include/convert.h>
#include <Util/include/runtime.h>
#include <Util/include/string.h>
#include <Util/include/mutex.h>

#include <LLP/include/port.h>

#include <TAO/Ledger/types/credentials.h>
#include <TAO/Ledger/include/enum.h>

#include <cstring>
#include <string>
#include <cmath>

namespace config
{
    std::map<std::string, std::string> mapArgs;
    std::map<std::string, std::vector<std::string> > mapMultiArgs;
    std::map<uint16_t, std::vector<std::string> > mapIPFilters;

    std::atomic<bool> fShutdown(false);
    std::atomic<bool> fSuspended(false);
    std::atomic<bool> fSuspendProtocol(false);
    std::atomic<bool> fDaemon(false);
    std::atomic<bool> fClient(false);
    std::atomic<bool> fTestNet(false);
    std::atomic<bool> fListen(false);
    std::atomic<bool> fMultiuser(false);
    std::atomic<bool> fProcessNotifications(false);
    std::atomic<bool> fInitialized(false);
    std::atomic<bool> fPoolStaking(false);
    std::atomic<bool> fStaking(false);
    std::atomic<bool> fHybrid(false);
    std::atomic<bool> fSister(false);
    std::atomic<bool> fIndexProofs(false);
    std::atomic<bool> fIndexAddress(false);
    std::atomic<bool> fIndexRegister(false);
    std::atomic<int32_t> nVerbose(0);
    std::atomic<bool> fApiAuth(false);

    /* Keeps track of the network owner hash. */
    uint256_t hashNetworkOwner;

    /* Declare our arguments mutex. */
    std::recursive_mutex ARGS_MUTEX;

    /* Give Opposite Argument Settings */
    void InterpretNegativeSetting(const std::string &name, std::map<std::string, std::string>& mapSettingsRet)
    {
        // interpret -nofoo as -foo=0 (and -nofoo=0 as -foo=1) as long as -foo not set
        if(name.find("-no") == 0)
        {
            std::string positive("-");
            positive.append(name.begin()+3, name.end());

            if(mapSettingsRet.count(positive) == 0)
            {
                if(mapSettingsRet[name].empty())
                    mapSettingsRet[positive] = "0";
                else
                    mapSettingsRet[positive] = (convert::atoi32(mapArgs[name]) == 0) ? "1" : "0";
            }
        }
    }

    /* Parse the Argument Parameters */
    void ParseParameters(int argc, const char*const argv[])
    {
        RECURSIVE(ARGS_MUTEX);

        for(int i = 1; i < argc; ++i)
        {
            /* Check for arguments value. */
            std::string strArg = std::string(argv[i]);
            if(strArg[0] != '-')
                break;

            /* Compress '--' value into '-' */
            if(strArg[1] == '-')
                strArg.erase(0, 1);

            /* Find value split. */
            std::string::size_type pos = strArg.find("=");

            /* Parse key and value. */
            std::string strKey = strArg.substr(0, pos);
            std::string strVal = "";

            /* Add value if not boolean. */
            if(pos != strArg.npos)
                strVal = strArg.substr(pos + 1);

            /* Build args map. */
            mapArgs[strKey] = strVal;
            mapMultiArgs[strKey].push_back(strVal);

            /* Interpret -nofoo as -foo=0 (and -nofoo=0 as -foo=1) as long as -foo not set */
            InterpretNegativeSetting(strKey, mapArgs);
        }
    }


    /* Return string argument or default value */
    std::string GetArg(const std::string& strArg, const std::string& strDefault)
    {
        RECURSIVE(ARGS_MUTEX);

        if(mapArgs.count(strArg))
            return mapArgs[strArg];

        return strDefault;
    }

    /* Return boolean if given argument is in map. */
    bool HasArg(const std::string& strArg)
    {
        RECURSIVE(ARGS_MUTEX);

        return mapMultiArgs.count(strArg) || mapArgs.count(strArg);
    }


    /* Return integer argument or default value. */
    int64_t GetArg(const std::string& strArg, int64_t nDefault)
    {
        RECURSIVE(ARGS_MUTEX);

        if(mapArgs.count(strArg))
            return convert::atoi64(mapArgs[strArg]);

        return nDefault;
    }


    /* Return boolean argument or default value */
    bool GetBoolArg(const std::string& strArg, bool fDefault)
    {
        RECURSIVE(ARGS_MUTEX);

        if(mapArgs.count(strArg))
        {
            /* Get a reference copy of arguement. */
            const std::string& strValue = mapArgs[strArg];

            /* Check for empty values. */
            if(strValue.empty())
                return true;

            /* Check for raw boolean string value 'true'. */
            if(strValue == "true")
                return true;

            /* Check for our raw boolean string value 'false'. */
            if(strValue == "false")
                return false;

            return (std::stoll(strValue) != 0);
        }

        return fDefault;
    }


    /* Set an argument if it doesn't already have a value */
    bool SoftSetArg(const std::string& strArg, const std::string& strValue)
    {
        RECURSIVE(ARGS_MUTEX);

        if(mapArgs.count(strArg))
            return false;

        mapArgs[strArg] = strValue;
        return true;
    }


    /* Set a boolean argument if it doesn't already have a value */
    bool SoftSetBoolArg(const std::string& strArg, bool fValue)
    {
        if(fValue)
            return SoftSetArg(strArg, std::string("1"));
        else
            return SoftSetArg(strArg, std::string("0"));
    }


    /* Caches some of the common arguments into global variables for quick/easy access */
    void CacheArgs()
    {
        fDaemon                 = GetBoolArg("-daemon", false);
        fTestNet                = (GetArg("-testnet", 0) > 0);
        fListen                 = GetBoolArg("-listen", true);
        fClient                 = GetBoolArg("-client", false);
        //fUseProxy               = GetBoolArg("-proxy")
        fMultiuser              = GetBoolArg("-multiuser", false) || GetBoolArg("-multiusername", false);
        fProcessNotifications   = GetBoolArg("-processnotifications", true);
        fPoolStaking            = GetBoolArg("-poolstaking", false);
        fStaking                = GetBoolArg("-staking", false) || GetBoolArg("-stake", false); //Both supported, -stake deprecated
        fHybrid                 = (GetArg("-hybrid", "") != ""); //-hybrid=<username> where username is the owner.
        //fSister                 = (GetArg("-sister", "") != ""); NOTE: disabled for now, -sister=<token> for sister network.for
        fApiAuth                = GetBoolArg("-apiauth", true);
        fIndexProofs            = GetBoolArg("-indexproofs");
        fIndexAddress           = GetBoolArg("-indexaddress");
        fIndexRegister          = GetBoolArg("-indexregister");
        nVerbose                = GetArg("-verbose", 0);

        /* Private Mode: Sub-Network Testnet. DO NOT USE FOR PRODUCTION. */
        if(GetBoolArg("-private", false))
        {
            {
                RECURSIVE(ARGS_MUTEX);

                /* Set our hybrid value as PRIVATE in private mode. */
                mapArgs["-hybrid"]  = ""; //delete hybrid parameters if supplied for testnet

                /* Automatically set testnet if not enabled, otherwise let user argument be used. */
                if(!mapArgs.count("-testnet"))
                    mapArgs["-testnet"] = "1";
            }

            /* Set our expected flags. */
            fHybrid.store(true);
            fTestNet.store(true);
        }

        /* Hybrid Mode: Sub-Network MainNet. USE FOR PRODUCTION. */
        else if(fHybrid.load())
        {
            RECURSIVE(ARGS_MUTEX);

            /* Grab our network owner. */
            const SecureString strOwner = mapArgs["-hybrid"].c_str();

            /* Calculate their genesis-id. */
            hashNetworkOwner = TAO::Ledger::Credentials::Genesis(strOwner);
            hashNetworkOwner.SetType(TAO::Ledger::GENESIS::OwnerType());
        }


        {
            RECURSIVE(ARGS_MUTEX);

            /* Check for testnet enabled flag, and bump value. */
            if(fTestNet.load())
                mapArgs["-testnet"] = "3";

            /* Parse the allowip entries and add them to a map for easier processing when new connections are made*/
            const std::vector<std::string>& vIPPortFilters = config::mapMultiArgs["-llpallowip"];
            for(const auto& entry : vIPPortFilters)
            {
                /* ensure it has a port*/
                std::size_t nPortPos = entry.find(":");
                if(nPortPos == std::string::npos)
                    continue;

                std::string strIP = entry.substr(0, nPortPos);
                uint16_t nPort = std::stoi(entry.substr(nPortPos + 1));

                mapIPFilters[nPort].push_back(strIP);
            }

            /* Parse the legacy rpcallowip entries and add them to to the filters map too, so that legacy users
               can migrate without having to change their config files*/
            const std::vector<std::string>& vRPCFilters = config::mapMultiArgs["-rpcallowip"];

            /* get the RPC port in use */
            uint16_t nRPCPort = static_cast<uint16_t>(config::fTestNet ? TESTNET_RPC_PORT : MAINNET_RPC_PORT);
            if(mapArgs.count("-rpcport"))
                nRPCPort = static_cast<uint16_t>(convert::atoi64(mapArgs["-rpcport"]));

            /* Add our RPC IP filters to the map. */
            for(const auto& entry : vRPCFilters)
                mapIPFilters[nRPCPort].push_back(entry);
        }


        /* Handle reading our activation data for transactions. */
        if(fHybrid.load() || fTestNet.load()) //this rule is only to activate private, hybrid, or testnets
        {
            RECURSIVE(ARGS_MUTEX);

            /* Handle for our market fees. */
            if(config::mapMultiArgs["-activatetx"].size() > 0)
            {
                /* Add our activation timestamps and versions. */
                for(const std::string& strActivate : config::mapMultiArgs["-activatetx"])
                {
                    /* Split the string by delimiter. */
                    std::vector<std::string> vActivate;
                    ParseString(strActivate, ':', vActivate);

                    /* Check for valid tranaction versions. */
                    if(vActivate.size() != 2)
                        throw debug::exception("-activatetx, too many parameters: ", strActivate);

                    /* Extract our activation version. */
                    const uint32_t nVersion =
                        std::stoi(vActivate[0]);

                    /* Get our activation timestamp. */
                    const uint64_t nTimestamp =
                        std::stoull(vActivate[1]);

                    /* Check our timelocks variable. */
                    const uint32_t nCurrentVersion = TAO::Ledger::CurrentTransactionVersion();
                    if(nVersion > nCurrentVersion)
                        throw debug::exception("-activatetx, cannot activate for ", VARIABLE(nVersion), " must be ", VARIABLE(nCurrentVersion));

                    /* Check our current time-lock. */
                    const uint64_t nTimelock = TAO::Ledger::StartTransactionTimelock(nVersion - 1);
                    if(nTimestamp <= nTimelock)
                        throw debug::exception("-activatetx, cannot activate before ", VARIABLE(nTimelock));

                    /* Log the activation timestamp to our console. */
                    debug::notice("Activation created for ", VARIABLE(nVersion), " at ", VARIABLE(nTimestamp));

                    /* Add to our timelocks code now. */
                    if(fTestNet.load())
                        TAO::Ledger::TESTNET_TRANSACTION_VERSION_TIMELOCK[nVersion - 1] = nTimestamp;
                    else
                        TAO::Ledger::NETWORK_TRANSACTION_VERSION_TIMELOCK[nVersion - 1] = nTimestamp;
                }
            }
        }
    }
}
