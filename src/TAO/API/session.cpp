/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/include/session.h>
#include <TAO/API/include/utils.h>

#include <TAO/Ledger/types/sigchain.h>

#include <LLC/include/random.h>

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {
        /* null sigchain reference for helper. */
        static memory::encrypted_ptr<TAO::Ledger::SignatureChain> null_ptr;

        /* Default Constructor. */
        Session::Session()
        : CREATE_MUTEX          ()
        , MUTEX                 ()
        , nID                   (0)
        , nStarted              (0)
        , nLastActive           (0)
        , pSigChain             ()
        , pActivePIN            ()
        , nNetworkKey           (0)
        {
        }


        /* Move constructor. */
        Session::Session(Session&& session) noexcept
        : nID                   (std::move(session.nID))
        , nStarted              (std::move(session.nStarted))
        , nLastActive           (std::move(session.nLastActive))
        , pSigChain             (std::move(session.pSigChain))
        , pActivePIN            (std::move(session.pActivePIN))
        , nNetworkKey           (std::move(session.nNetworkKey))
        {
        }


        /** Move assignment. **/
        Session& Session::operator=(Session&& session) noexcept
        {
            nID =               (std::move(session.nID));
            nStarted =          (std::move(session.nStarted));
            nLastActive =       (std::move(session.nLastActive));
            pSigChain =         (std::move(session.pSigChain));
            pActivePIN =        (std::move(session.pActivePIN));
            nNetworkKey =       (std::move(session.nNetworkKey));

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
        void Session::Initialize(const SecureString& strUsername, const SecureString& strPassword, const SecureString& strPin, const uint256_t nSessionID)
        {
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
            LOCK(MUTEX);
            return nID;
        }


        /* Returns the active PIN for this session. */
        const memory::encrypted_ptr<TAO::Ledger::PinUnlock>& Session::GetActivePIN() const
        {
            LOCK(MUTEX);
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
            /* Since we are changing the sig chain reference, we need to lock the CREATE mutex as another process might be using it */
            LOCK(CREATE_MUTEX);

            {
                LOCK(MUTEX);

                /* Get the existing username so that we can use it for the new sig chain */
                SecureString strUsername = pSigChain->UserName();
                /* Clear the existing sig chain pointer */
                pSigChain.free();

                /* Instantate a new one with the existing username and the new password */
                pSigChain = new TAO::Ledger::SignatureChain(strUsername, strPassword);
            }
        }


        /* Returns the cached network private key. */
        uint512_t Session::GetNetworkKey() const
        {
            LOCK(MUTEX);
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
            LOCK(MUTEX);
            return nLastActive;
        }


        


        /* Determine if the Users are locked. */
        bool Session::Locked() const
        {
            LOCK(MUTEX);

            if(config::fMultiuser.load() || pActivePIN.IsNull() || pActivePIN->PIN().empty())
                return true;

            return false;
        }


        /* In sessionless API mode this method checks that the active sig chain has
         * been unlocked to allow transactions.  If the account has not been specifically
         * unlocked then we assume that they ARE allowed to transact, since the PIN would
         * need to be provided in each API call */
        bool Session::CanTransact() const
        {
            LOCK(MUTEX);

            if(config::fMultiuser.load() || pActivePIN.IsNull() || pActivePIN->CanTransact())
                return true;

            return false;
        }


        /* In sessionless API mode this method checks that the active sig chain has
         *  been unlocked to allow mining */
        bool Session::CanMine() const
        {
            LOCK(MUTEX);

            if(config::fMultiuser.load() || (!pActivePIN.IsNull() && pActivePIN->CanMine()))
                return true;

            return false;
        }


        /* In sessionless API mode this method checks that the active sig chain has
         *  been unlocked to allow staking */
        bool Session::CanStake() const
        {
            LOCK(MUTEX);
            
            if(config::fMultiuser.load() || (!pActivePIN.IsNull() && pActivePIN->CanStake()))
                return true;

            return false;
        }

        /* In sessionless API mode this method checks that the active sig chain has
         *  been unlocked to allow notifications to be processed */
        bool Session::CanProcessNotifications() const
        {
            LOCK(MUTEX);
            
            if(config::fMultiuser.load() || (!pActivePIN.IsNull() && pActivePIN->ProcessNotifications()))
                return true;

            return false;
        }




        /*  Returns the sigchain the account logged in. */
        const memory::encrypted_ptr<TAO::Ledger::SignatureChain>& Session::GetAccount() const
        {
            LOCK(MUTEX);

            return pSigChain;
        }
        

    }
}
