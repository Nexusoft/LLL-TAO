/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/types/session.h>
#include <TAO/API/include/utils.h>

#include <TAO/Ledger/types/sigchain.h>

#include <Util/include/random.h>

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {
        /* Default Constructor. */
        Session::Session()
        : CREATE_MUTEX          ()
        , MUTEX                 ()
        , nID                   (0)
        , nStarted              (0)
        , nLastActive           (0)
        , nAuthAttempts          (0)
        , pSigChain             ()
        , pActivePIN            ()
        , nNetworkKey           (0)
        , vP2PIncoming          ()
        , vP2POutgoing          ()
        {
        }


        /* Move constructor. */
        Session::Session(Session&& session) noexcept
        : nID                   (std::move(session.nID))
        , nStarted              (std::move(session.nStarted))
        , nLastActive           (std::move(session.nLastActive))
        , nAuthAttempts          (std::move(session.nAuthAttempts))
        , pSigChain             (std::move(session.pSigChain))
        , pActivePIN            (std::move(session.pActivePIN))
        , nNetworkKey           (std::move(session.nNetworkKey))
        , vP2PIncoming          (std::move(session.vP2PIncoming))
        , vP2POutgoing          (std::move(session.vP2POutgoing))
        {
        }


        /** Move assignment. **/
        Session& Session::operator=(Session&& session) noexcept
        {
            nID =               (std::move(session.nID));
            nStarted =          (std::move(session.nStarted));
            nLastActive =       (std::move(session.nLastActive));
            nAuthAttempts =      (std::move(session.nAuthAttempts));
            pSigChain =         (std::move(session.pSigChain));
            pActivePIN =        (std::move(session.pActivePIN));
            nNetworkKey =       (std::move(session.nNetworkKey));
            vP2PIncoming =      (std::move(session.vP2PIncoming));
            vP2POutgoing =      (std::move(session.vP2POutgoing));

            return *this;
        }

        /* Default Destructor. */
        Session::~Session()
        {
            /* Free up values in encrypted memory */
            if(!pSigChain.IsNull())
                pSigChain.free();
            
            if(!pActivePIN.IsNull())
                pActivePIN.free();

            if(!nNetworkKey.IsNull())
                nNetworkKey.free();
        }

        /* Initializes the session from username / password / pin */
        void Session::Initialize(const SecureString& strUsername, const SecureString& strPassword, const SecureString& strPin, const uint256_t& nSessionID)
        {
            LOCK(MUTEX);

            /* Set the session ID */
            nID = nSessionID;

            /* Set the initial start time */
            nStarted = runtime::unifiedtimestamp();

            /* Set the last active time */
            nLastActive = runtime::unifiedtimestamp();

            /* Instantiate the sig chain */
            pSigChain = new TAO::Ledger::SignatureChain(strUsername, strPassword);

            /* Generate and cache the network private key */
            nNetworkKey = new memory::encrypted_type<uint512_t>(pSigChain->Generate("network", 0, strPin));
        }


        /* Returns the session ID for this session */
        uint256_t Session::ID() const
        {
            return nID;
        }


        /* Returns the active PIN for this session. */
        const memory::encrypted_ptr<TAO::Ledger::PinUnlock>& Session::GetActivePIN() const
        {
            return pActivePIN;
        }


        /* Clears the active PIN for this session. */
        void Session::ClearActivePIN()
        {
            LOCK(MUTEX);

            /* Clean up the existing pin */
            if(!pActivePIN.IsNull())
                pActivePIN.free();
        }


        /* Updates the cached pin and its unlocked actions */
        void Session::UpdatePIN(const SecureString& strPin, uint8_t nUnlockedActions)
        {
            LOCK(MUTEX);

            /* Clean up the existing pin */
            if(!pActivePIN.IsNull())
                pActivePIN.free();

            /* Instantiate new pin */
            pActivePIN = new TAO::Ledger::PinUnlock(strPin, nUnlockedActions);
        }


        /* Updates the password stored in the internal sig chain */
        void Session::UpdatePassword(const SecureString& strPassword)
        {
            LOCK(MUTEX);

            /* Get the existing username so that we can use it for the new sig chain */
            SecureString strUsername = pSigChain->UserName();
            /* Clear the existing sig chain pointer */
            pSigChain.free();

            /* Instantate a new one with the existing username and the new password */
            pSigChain = new TAO::Ledger::SignatureChain(strUsername, strPassword);
        }


        /* Returns the cached network private key. */
        uint512_t Session::GetNetworkKey() const
        {
            return nNetworkKey->DATA;
        }


        /* Logs activity by updating the last active timestamp. */
        void Session::SetLastActive()
        {
            LOCK(MUTEX);
            nLastActive = runtime::unifiedtimestamp();
        }


        /* Gets last active timestamp. */
        uint64_t Session::GetLastActive() const
        {
            return nLastActive;
        }


        


        /* Determine if the Users are locked. */
        bool Session::Locked() const
        {
            LOCK(MUTEX);

            if(pActivePIN.IsNull() || pActivePIN->PIN().empty())
                return true;

            return false;
        }


        /* Checks that the active sig chain has been unlocked to allow transactions. */
        bool Session::CanTransact() const
        {
            LOCK(MUTEX);

            if(pActivePIN.IsNull() && pActivePIN->CanTransact())
                return true;

            return false;
        }


        /* Checks that the active sig chain has been unlocked to allow mining */
        bool Session::CanMine() const
        {
            LOCK(MUTEX);

            if(!config::fMultiuser.load() && (!pActivePIN.IsNull() && pActivePIN->CanMine()))
                return true;

            return false;
        }


        /* Checks that the active sig chain has been unlocked to allow staking */
        bool Session::CanStake() const
        {
            LOCK(MUTEX);
            
            if(!config::fMultiuser.load() &&(!pActivePIN.IsNull() && pActivePIN->CanStake()))
                return true;

            return false;
        }

        /* Checks that the active sig chain has been unlocked to allow notifications to be processed */
        bool Session::CanProcessNotifications() const
        {
            LOCK(MUTEX);
            
            if(!pActivePIN.IsNull() && pActivePIN->ProcessNotifications())
                return true;

            return false;
        }


        /*  Returns the sigchain the account logged in. */
        const memory::encrypted_ptr<TAO::Ledger::SignatureChain>& Session::GetAccount() const
        {
            return pSigChain;
        }


        /* Adds a new P2P Request. */
        void Session::AddP2PRequest(const LLP::P2P::ConnectionRequest& request, bool fIncoming)
        {
            /* Lock mutex so p2p request vector can't be accessed by another thread */
            LOCK(MUTEX);

            /* Reference to the vector to work on, based on the fIncoming flag */
            std::vector<LLP::P2P::ConnectionRequest>& vP2PRequests = fIncoming ? vP2PIncoming : vP2POutgoing;

            /* Add the request */
            vP2PRequests.push_back(request);
        }


        /* Gets P2P Request matching the app id / hashPeer criteria. */
        LLP::P2P::ConnectionRequest Session::GetP2PRequest(const std::string& strAppID, const uint256_t& hashPeer, bool fIncoming) const
        {
            /* Lock mutex so p2p request vector can't be accessed by another thread */
            LOCK(MUTEX);

            /* Reference to the vector to work on, based on the fIncoming flag */
            const std::vector<LLP::P2P::ConnectionRequest>& vP2PRequests = fIncoming ? vP2PIncoming : vP2POutgoing;

            /* Check each request */
            for(const auto request : vP2PRequests)
            {
                /* If the request matches the appID and hashPeer then return it*/
                if(request.strAppID == strAppID && request.hashPeer == hashPeer)
                    return request;
            }

            /* If we haven't found one then return an empty connection request */
            return LLP::P2P::ConnectionRequest();

        }


        /* Checks to see if a P2P Request matching the app id / hashPeer criteria exists. */
        bool Session::HasP2PRequest(const std::string& strAppID, const uint256_t& hashPeer, bool fIncoming) const
        {
            /* Lock mutex so p2p request vector can't be accessed by another thread */
            LOCK(MUTEX);

            /* Reference to the vector to work on, based on the fIncoming flag */
            const std::vector<LLP::P2P::ConnectionRequest>& vP2PRequests = fIncoming ? vP2PIncoming : vP2POutgoing;

            /* Check each request */
            for(const auto request : vP2PRequests)
            {
                /* Check the appID and hashPeer */
                if(request.strAppID == strAppID && request.hashPeer == hashPeer)
                    return true;
            }

            /* No match found */
            return false;
        }


        /* Deletes the P2P Request matching the app id / hashPeer criteria exists. */
        void Session::DeleteP2PRequest(const std::string& strAppID, const uint256_t& hashPeer, bool fIncoming)
        {
            /* Lock mutex so p2p request vector can't be accessed by another thread */
            LOCK(MUTEX);

            /* Reference to the vector to work on, based on the fIncoming flag */
            std::vector<LLP::P2P::ConnectionRequest>& vP2PRequests = fIncoming ? vP2PIncoming : vP2POutgoing;

            vP2PRequests.erase
            (
                /* Use std remove_if function to return the iterator to erase. This allows us to pass in a lambda function,
                which itself can check to see if a match exists */
                std::remove_if(vP2PRequests.begin(), vP2PRequests.end(), 
                [&](const LLP::P2P::ConnectionRequest& request)
                {
                    return (request.strAppID == strAppID && request.hashPeer == hashPeer);
                }), 
                vP2PRequests.end()
            );
        }


        /* Returns a vector of all connection requests. NOTE: This will copy the internal vector to protect against 
           direct manipulation by calling code */
        const std::vector<LLP::P2P::ConnectionRequest> Session::GetP2PRequests(bool fIncoming) const
        {
            /* Lock mutex so p2p request vector can't be accessed by another thread */
            LOCK(MUTEX);

            /* Reference to the vector to work on, based on the fIncoming flag */
            const std::vector<LLP::P2P::ConnectionRequest>& vP2PRequests = fIncoming ? vP2PIncoming : vP2POutgoing;

            /* Return the required vector. */
            return vP2PRequests;
        }


        /*  Returns the number of incorrect authentication attempts made in this session */
        uint8_t Session::GetAuthAttempts() const
        {
            return nAuthAttempts;
        }


        /*  Increments the number of incorrect authentication attempts made in this session */
        void Session::IncrementAuthAttempts() 
        {
            /* Lock mutex so two threads can't increment at the same time */
            LOCK(MUTEX);

            nAuthAttempts++;
        }
        

    }
}
