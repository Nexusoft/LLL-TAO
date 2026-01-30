/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLP/include/session_recovery.h>
#include <Util/include/debug.h>
#include <Util/include/runtime.h>

namespace LLP
{
    /** Default session timeout: 1 hour **/
    static const uint64_t DEFAULT_SESSION_TIMEOUT = 3600;

    /** Default max reconnects **/
    static const uint32_t DEFAULT_MAX_RECONNECTS = 10;


    /***************************************************************************
     * SessionRecoveryData Implementation
     **************************************************************************/

    /** Default Constructor **/
    SessionRecoveryData::SessionRecoveryData()
    : nSessionId(0)
    , hashKeyID(0)
    , hashGenesis(0)
    , nChannel(0)
    , nLastActivity(0)
    , nCreated(0)
    , vPubKey()
    , strAddress()
    , nReconnectCount(0)
    , fAuthenticated(false)
    , nLastLane(0)
    , hashChaCha20Key(0)
    , nChaCha20Nonce(0)
    , vDisposablePubKey()
    , hashDisposableKeyID(0)
    {
    }


    /** Constructor from MiningContext **/
    SessionRecoveryData::SessionRecoveryData(const MiningContext& context)
    : nSessionId(context.nSessionId)
    , hashKeyID(context.hashKeyID)
    , hashGenesis(context.hashGenesis)
    , nChannel(context.nChannel)
    , nLastActivity(context.nTimestamp)
    , nCreated(runtime::unifiedtimestamp())
    , vPubKey(context.vMinerPubKey)
    , strAddress(context.strAddress)
    , nReconnectCount(0)
    , fAuthenticated(context.fAuthenticated)
    , nLastLane(0)
    , hashChaCha20Key(0)
    , nChaCha20Nonce(0)
    , vDisposablePubKey()
    , hashDisposableKeyID(0)
    {
    }


    /** ToContext **/
    MiningContext SessionRecoveryData::ToContext() const
    {
        return MiningContext(
            nChannel,
            0,  /* nHeight - will be updated on reconnect */
            runtime::unifiedtimestamp(),
            strAddress,
            0,  /* nProtocolVersion - will be updated */
            fAuthenticated,
            nSessionId,
            hashKeyID,
            hashGenesis
        ).WithPubKey(vPubKey);
    }


    /** IsExpired **/
    bool SessionRecoveryData::IsExpired(uint64_t nTimeoutSec) const
    {
        uint64_t nNow = runtime::unifiedtimestamp();
        return (nNow - nLastActivity) > nTimeoutSec;
    }


    /** UpdateActivity **/
    void SessionRecoveryData::UpdateActivity()
    {
        nLastActivity = runtime::unifiedtimestamp();
    }


    /***************************************************************************
     * SessionRecoveryManager Implementation
     **************************************************************************/

    /** Private constructor **/
    SessionRecoveryManager::SessionRecoveryManager()
    : mapSessionsByKey()
    , mapAddressToKey()
    , nSessionTimeout(DEFAULT_SESSION_TIMEOUT)
    , nMaxReconnects(DEFAULT_MAX_RECONNECTS)
    {
    }


    /** Get singleton **/
    SessionRecoveryManager& SessionRecoveryManager::Get()
    {
        static SessionRecoveryManager instance;
        return instance;
    }


    /** SaveSession **/
    bool SessionRecoveryManager::SaveSession(const MiningContext& context)
    {
        /* Only save authenticated sessions with valid key IDs */
        if(!context.fAuthenticated || context.hashKeyID == 0)
        {
            debug::log(3, FUNCTION, "Cannot save unauthenticated session");
            return false;
        }

        SessionRecoveryData data(context);

        /* Store by key ID */
        mapSessionsByKey.InsertOrUpdate(context.hashKeyID, data);

        /* Store address mapping if available */
        if(!context.strAddress.empty())
        {
            mapAddressToKey.InsertOrUpdate(context.strAddress, context.hashKeyID);
        }

        debug::log(2, FUNCTION, "Saved session for keyID=", context.hashKeyID.SubString(),
                   " sessionId=", context.nSessionId, " address=", context.strAddress);

        return true;
    }


    /** RecoverSession **/
    bool SessionRecoveryManager::RecoverSession(const uint256_t& hashKeyID, MiningContext& context)
    {
        auto optData = mapSessionsByKey.Get(hashKeyID);
        if(!optData.has_value())
        {
            debug::log(2, FUNCTION, "No recoverable session for keyID=", hashKeyID.SubString());
            return false;
        }

        /* Get a copy of the data - concurrent map returns copies */
        SessionRecoveryData data = optData.value();

        /* Check expiration */
        if(data.IsExpired(nSessionTimeout.load()))
        {
            debug::log(2, FUNCTION, "Session expired for keyID=", hashKeyID.SubString());
            mapSessionsByKey.Erase(hashKeyID);
            return false;
        }

        /* Check reconnect limit */
        if(data.nReconnectCount >= nMaxReconnects.load())
        {
            debug::log(0, FUNCTION, "Session exceeded max reconnects for keyID=", hashKeyID.SubString());
            mapSessionsByKey.Erase(hashKeyID);
            return false;
        }

        /* Increment reconnect count and update activity */
        data.nReconnectCount++;
        data.UpdateActivity();
        mapSessionsByKey.Update(hashKeyID, data);

        /* Restore context */
        context = data.ToContext();

        debug::log(0, FUNCTION, "Recovered session for keyID=", hashKeyID.SubString(),
                   " sessionId=", context.nSessionId, " reconnect #", data.nReconnectCount);

        return true;
    }


    /** RecoverSessionByAddress **/
    bool SessionRecoveryManager::RecoverSessionByAddress(const std::string& strAddress, MiningContext& context)
    {
        auto optKeyID = mapAddressToKey.Get(strAddress);
        if(!optKeyID.has_value())
        {
            debug::log(3, FUNCTION, "No session mapping for address=", strAddress);
            return false;
        }

        return RecoverSession(optKeyID.value(), context);
    }


    /** RecoverSessionByAddress (data) **/
    std::optional<SessionRecoveryData> SessionRecoveryManager::RecoverSessionByAddress(const std::string& strAddress)
    {
        auto optKeyID = mapAddressToKey.Get(strAddress);
        if(!optKeyID.has_value())
        {
            debug::log(3, FUNCTION, "No session mapping for address=", strAddress);
            return std::nullopt;
        }

        auto optData = mapSessionsByKey.Get(optKeyID.value());
        if(!optData.has_value())
        {
            debug::log(3, FUNCTION, "No recoverable session for keyID=", optKeyID.value().SubString());
            return std::nullopt;
        }

        SessionRecoveryData data = optData.value();
        if(data.IsExpired(nSessionTimeout.load()))
        {
            debug::log(2, FUNCTION, "Session expired for keyID=", optKeyID.value().SubString());
            mapSessionsByKey.Erase(optKeyID.value());
            return std::nullopt;
        }

        return data;
    }


    /** RemoveSession **/
    bool SessionRecoveryManager::RemoveSession(const uint256_t& hashKeyID)
    {
        auto optData = mapSessionsByKey.GetAndRemove(hashKeyID);
        if(!optData.has_value())
            return false;

        /* Also remove address mapping */
        if(!optData.value().strAddress.empty())
        {
            mapAddressToKey.Erase(optData.value().strAddress);
        }

        debug::log(2, FUNCTION, "Removed session for keyID=", hashKeyID.SubString());
        return true;
    }


    /** UpdateSession **/
    bool SessionRecoveryManager::UpdateSession(const MiningContext& context)
    {
        if(context.hashKeyID == 0)
            return false;

        auto optData = mapSessionsByKey.Get(context.hashKeyID);
        if(!optData.has_value())
            return false;

        SessionRecoveryData data = optData.value();
        data.nChannel = context.nChannel;
        data.nLastActivity = context.nTimestamp;
        data.fAuthenticated = context.fAuthenticated;

        mapSessionsByKey.Update(context.hashKeyID, data);
        return true;
    }


    /** SaveChaCha20State **/
    bool SessionRecoveryManager::SaveChaCha20State(
        const uint256_t& hashKeyID,
        const uint256_t& hashKey,
        uint64_t nNonce
    )
    {
        if(hashKeyID == 0)
            return false;

        auto optData = mapSessionsByKey.Get(hashKeyID);
        if(!optData.has_value())
            return false;

        SessionRecoveryData data = optData.value();
        data.hashChaCha20Key = hashKey;
        data.nChaCha20Nonce = nNonce;
        mapSessionsByKey.Update(hashKeyID, data);

        return true;
    }


    /** RestoreChaCha20State **/
    bool SessionRecoveryManager::RestoreChaCha20State(
        const uint256_t& hashKeyID,
        uint256_t& hashKey,
        uint64_t& nNonce
    )
    {
        if(hashKeyID == 0)
            return false;

        auto optData = mapSessionsByKey.Get(hashKeyID);
        if(!optData.has_value())
            return false;

        hashKey = optData.value().hashChaCha20Key;
        nNonce = optData.value().nChaCha20Nonce;
        return hashKey != 0;
    }


    /** SaveDisposableKey **/
    bool SessionRecoveryManager::SaveDisposableKey(
        const uint256_t& hashKeyID,
        const std::vector<uint8_t>& vPubKey,
        const uint256_t& hashDisposableKeyID
    )
    {
        if(hashKeyID == 0)
            return false;

        auto optData = mapSessionsByKey.Get(hashKeyID);
        if(!optData.has_value())
            return false;

        SessionRecoveryData data = optData.value();
        data.vDisposablePubKey = vPubKey;
        data.hashDisposableKeyID = hashDisposableKeyID;
        mapSessionsByKey.Update(hashKeyID, data);

        return true;
    }


    /** RestoreDisposableKey **/
    bool SessionRecoveryManager::RestoreDisposableKey(
        const uint256_t& hashKeyID,
        std::vector<uint8_t>& vPubKey,
        uint256_t& hashDisposableKeyID
    )
    {
        if(hashKeyID == 0)
            return false;

        auto optData = mapSessionsByKey.Get(hashKeyID);
        if(!optData.has_value())
            return false;

        vPubKey = optData.value().vDisposablePubKey;
        hashDisposableKeyID = optData.value().hashDisposableKeyID;
        return !vPubKey.empty();
    }


    /** UpdateLane **/
    bool SessionRecoveryManager::UpdateLane(
        const uint256_t& hashKeyID,
        uint8_t nNewLane
    )
    {
        if(hashKeyID == 0)
            return false;

        auto optData = mapSessionsByKey.Get(hashKeyID);
        if(!optData.has_value())
            return false;

        SessionRecoveryData data = optData.value();
        data.nLastLane = nNewLane;
        mapSessionsByKey.Update(hashKeyID, data);
        return true;
    }


    /** HasSession **/
    bool SessionRecoveryManager::HasSession(const uint256_t& hashKeyID) const
    {
        auto optData = mapSessionsByKey.Get(hashKeyID);
        if(!optData.has_value())
            return false;

        return !optData.value().IsExpired(nSessionTimeout.load());
    }


    /** CleanupExpired **/
    uint32_t SessionRecoveryManager::CleanupExpired(uint64_t nTimeoutSec)
    {
        uint32_t nRemoved = 0;
        auto pairs = mapSessionsByKey.GetAllPairs();

        for(const auto& pair : pairs)
        {
            if(pair.second.IsExpired(nTimeoutSec))
            {
                if(RemoveSession(pair.first))
                    ++nRemoved;
            }
        }

        if(nRemoved > 0)
        {
            debug::log(2, FUNCTION, "Cleaned up ", nRemoved, " expired sessions");
        }

        return nRemoved;
    }


    /** GetSessionCount **/
    size_t SessionRecoveryManager::GetSessionCount() const
    {
        return mapSessionsByKey.Size();
    }


    /** GetSessionTimeout **/
    uint64_t SessionRecoveryManager::GetSessionTimeout() const
    {
        return nSessionTimeout.load();
    }


    /** SetSessionTimeout **/
    void SessionRecoveryManager::SetSessionTimeout(uint64_t nTimeoutSec)
    {
        nSessionTimeout.store(nTimeoutSec);
    }


    /** GetMaxReconnects **/
    uint32_t SessionRecoveryManager::GetMaxReconnects() const
    {
        return nMaxReconnects.load();
    }


    /** SetMaxReconnects **/
    void SessionRecoveryManager::SetMaxReconnects(uint32_t nMax)
    {
        nMaxReconnects.store(nMax);
    }

} // namespace LLP
