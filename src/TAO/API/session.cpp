/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/types/exception.h>
#include <TAO/API/types/session.h>
#include <TAO/API/include/global.h>

#include <TAO/Ledger/types/sigchain.h>

#include <LLC/include/argon2.h>
#include <LLC/include/encrypt.h>
#include <LLC/include/random.h>

#include <LLD/include/global.h>

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {
        /* Default Constructor. */
        Session::Session()
        : CREATE_MUTEX          ()
        , hashAuth              ()
        , vProcessQueue         (new std::queue<TAO::Operation::Contract>())
        , vProcessed            (new std::vector<uint512_t>())
        , MUTEX                 ()
        , nID                   (0)
        , nStarted              (0)
        , nLastActive           (0)
        , nAuthAttempts         (0)
        , pSigChain             ()
        , pActivePIN            ()
        , nNetworkKey           (0)
        {
        }


        /* Move constructor. */
        Session::Session(Session&& session) noexcept
        : hashAuth              (std::move(session.hashAuth))
        , vProcessQueue         (std::move(session.vProcessQueue))
        , vProcessed            (std::move(session.vProcessed))
        , nID                   (std::move(session.nID))
        , nStarted              (std::move(session.nStarted))
        , nLastActive           (std::move(session.nLastActive))
        , nAuthAttempts         (session.nAuthAttempts.load())
        , pSigChain             (std::move(session.pSigChain))
        , pActivePIN            (std::move(session.pActivePIN))
        , nNetworkKey           (std::move(session.nNetworkKey))
        {
        }


        /** Move assignment. **/
        Session& Session::operator=(Session&& session) noexcept
        {
            hashAuth          = (std::move(session.hashAuth));
            vProcessQueue     = (std::move(session.vProcessQueue));
            vProcessed        = (std::move(session.vProcessed));
            nID               = (std::move(session.nID));
            nStarted          = (std::move(session.nStarted));
            nLastActive       = (std::move(session.nLastActive));
            nAuthAttempts     = (session.nAuthAttempts.load());
            pSigChain         = (std::move(session.pSigChain));
            pActivePIN        = (std::move(session.pActivePIN));
            nNetworkKey       = (std::move(session.nNetworkKey));

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
        void Session::Initialize(const TAO::Ledger::SignatureChain& pUser, const SecureString& strPin, const uint256_t& nSessionID)
        {
            {
                LOCK(MUTEX);

                /* Set the session ID */
                nID = nSessionID;

                /* Set the initial start time */
                nStarted = runtime::unifiedtimestamp();

                /* Set the last active time */
                nLastActive = runtime::unifiedtimestamp();

                /* Instantiate the sig chain */
                pSigChain = new TAO::Ledger::SignatureChain(pUser);
            }

            /* Cache the pin with no unlocked actions */
            UpdatePIN(strPin, TAO::Ledger::PinUnlock::UnlockActions::NONE);

        }


        /* Returns true if the session has not been initialised. */
        bool Session::IsNull() const
        {
            /* Simply check to see whether the started timestamp has been set */
            return nStarted == 0;
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
            //LOCK(MUTEX);

            /* Clean up the existing pin */
            if(!pActivePIN.IsNull())
                pActivePIN.free();
        }


        /* Updates the cached pin and its unlocked actions */
        void Session::UpdatePIN(const SecureString& strPin, uint8_t nUnlockedActions)
        {
            //LOCK(MUTEX);

            /* Clean up the existing pin */
            if(!pActivePIN.IsNull())
                pActivePIN.free();

            /* Instantiate new pin */
            pActivePIN = new TAO::Ledger::PinUnlock(strPin, nUnlockedActions);
        }


        /* Updates the password stored in the internal sig chain */
        void Session::UpdatePassword(const SecureString& strPassword)
        {
            //LOCK(MUTEX);

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
            /* Lazily generate the network key the first time it is requested */
            if(nNetworkKey.IsNull() && !pActivePIN.IsNull())
                /* Generate and cache the network private key */
                 nNetworkKey = new memory::encrypted_type<uint512_t>(pSigChain->Generate("network", 0, pActivePIN->PIN()));

            if(nNetworkKey.IsNull())
                return 0;

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
            //LOCK(MUTEX);

            if(pActivePIN.IsNull() || pActivePIN->PIN().empty())
                return true;

            return false;
        }


        /* Checks that the active sig chain has been unlocked to allow transactions. */
        bool Session::CanTransact() const
        {
            //LOCK(MUTEX);

            if(!pActivePIN.IsNull() && pActivePIN->CanTransact())
                return true;

            return false;
        }


        /* Checks that the active sig chain has been unlocked to allow mining */
        bool Session::CanMine() const
        {
            //LOCK(MUTEX);

            if(!config::fMultiuser.load() && (!pActivePIN.IsNull() && pActivePIN->CanMine()))
                return true;

            return false;
        }


        /* Checks that the active sig chain has been unlocked to allow staking */
        bool Session::CanStake() const
        {
            //LOCK(MUTEX);

            if(!config::fMultiuser.load() &&(!pActivePIN.IsNull() && pActivePIN->CanStake()))
                return true;

            return false;
        }

        /* Checks that the active sig chain has been unlocked to allow notifications to be processed */
        bool Session::CanProcessNotifications() const
        {
            //LOCK(MUTEX);

            if(!pActivePIN.IsNull() && pActivePIN->ProcessNotifications())
                return true;

            return false;
        }


        /*  Returns the sigchain the account logged in. */
        const memory::encrypted_ptr<TAO::Ledger::SignatureChain>& Session::GetAccount() const
        {
            return pSigChain;
        }


        /*  Returns the number of incorrect authentication attempts made in this session */
        uint16_t Session::GetAuthAttempts() const
        {
            return nAuthAttempts.load();
        }


        /*  Increments the number of incorrect authentication attempts made in this session */
        void Session::IncrementAuthAttempts()
        {
            /* If the number of failed auth attempts exceeds the configured allowed number then log this user out */
            if(++nAuthAttempts >= config::GetArg("-authattempts", 3))
            {
                /* Grab a reference of our session-id. */
                const uint256_t hashSession = ID();

                /* Log the user out and terminate all relevant actions. */
                Commands::Find<Users>()->TerminateSession(hashSession);
                throw Exception(-290, "Too many invalid password/pin attempts. Logging out user session: ", hashSession.ToString());
            }
        }


        /* Resets our auth counter to zero on successful login. */
        void Session::ResetAuthAttempts()
        {
            /* Reset our atomic counter to zero. */
            nAuthAttempts.store(0);
        }


        /* Encrypts the current session and saves it to the local database */
        void Session::Save(const SecureString& strPin) const
        {
            /* Use a data stream to help serialize */
            DataStream ssData(SER_LLD, 1);

            /* Serialize the session data */
            ssData << nStarted;
            ssData << nAuthAttempts.load();

            /* XXX: Assess why this is here, looks redundant. */
            if(!nNetworkKey.IsNull())
                ssData << nNetworkKey->DATA;
            else
                ssData << uint512_t(0);

            /* XXX: This looks redundant as well. */
            ssData << hashAuth;

            /* Sig Chain */
            ssData << pSigChain->UserName();
            ssData << pSigChain->Password();
            ssData << pActivePIN->UnlockedActions();

            /* Generate a symmetric key to encrypt it based on the genesis and pin */
            uint256_t hashGenesis = GetAccount()->Genesis();

            /* Add the pin */
            std::vector<uint8_t> vchKey = hashGenesis.GetBytes();
            vchKey.insert(vchKey.end(), strPin.begin(), strPin.end());

            /* For added security we don't directly use the genesis + PIN as the symmetric key, but a hash of it instead. */
            uint256_t hashKey = LLC::Argon2Fast_256(vchKey); //XXX: this shouldn't be a fast key

            /* Encrypt the data */
            std::vector<uint8_t> vchCipherText;
            if(!LLC::EncryptAES256(hashKey.GetBytes(), ssData.Bytes(), vchCipherText))
                throw Exception(-270, "Failed to encrypt data.");

            /* Write the session to local DB */
            LLD::Local->WriteSession(hashGenesis, vchCipherText);
        }


        /* Decrypts and loads an existing session from disk */
        void Session::Load(const uint256_t& nSessionID, const uint256_t& hashGenesis, const SecureString& strPin)
        {
            /* The encrypted session bytes */
            std::vector<uint8_t> vchEncrypted;

            /* Load the encrypted data from the local DB */
            if(!LLD::Local->ReadSession(hashGenesis, vchEncrypted))
                throw Exception(-309, "Error loading session.");

            /* Generate a symmetric key to encrypt it based on the session ID and pin */
            std::vector<uint8_t> vchKey;

            /* Add the genesis ID */
            vchKey = hashGenesis.GetBytes();

            /* Add the pin */
            vchKey.insert(vchKey.end(), strPin.begin(), strPin.end());

            /* For added security we don't directly use the session ID + PIN as the symmetric key, but a hash of it instead.
               NOTE: the AES256 function requires a 32-byte key, so we reduce the length if necessary by using
                the 256-bit version of the Argon2 hashing function */
            uint256_t nKey = LLC::Argon2Fast_256(vchKey);

            /* Put the key back into the vector */
            vchKey = nKey.GetBytes();

            /* The decrypted session bytes */
            std::vector<uint8_t> vchSession;

            /* Encrypt the data */
            if(!LLC::DecryptAES256(vchKey, vchEncrypted, vchSession))
                throw Exception(-309, "Error loading session."); // generic message callers can't brute force session id's

            try
            {
                /* Use a data stream to help deserialize */
                DataStream ssData(vchSession, SER_LLD, 1);

                /* Deserialize the session data*/
                ssData >> nStarted;

                //XXX: this is superfluous, just acting as dummy now. We Don't want to track auth attempts between session loads.
                uint16_t nAttempts;
                ssData >> nAttempts;

                /* Network key */
                uint512_t nKey;
                ssData >> nKey;
                if(nKey != 0)
                    nNetworkKey = new memory::encrypted_type<uint512_t>(nKey);

                ssData >> hashAuth;

                /* Sig Chain */
                SecureString strUsername;
                ssData >> strUsername;

                SecureString strPassword;
                ssData >> strPassword;

                /* Initialise the sig chain */
                pSigChain = new TAO::Ledger::SignatureChain(strUsername, strPassword);

                /* PinUnlock */
                uint8_t nUnlockActions;
                ssData >> nUnlockActions;

                /* Restore the unlock actions and cache pin */
                UpdatePIN(strPin, nUnlockActions);

                /* Update the last active time */
                nLastActive = runtime::unifiedtimestamp();

                /* Finally set the new session ID */
                nID = nSessionID;
            }
            catch(const std::exception& e)
            {
                throw Exception(-309, "Error loading session."); // generic message callers can't brute force session id's
            }
        }

    }
}
