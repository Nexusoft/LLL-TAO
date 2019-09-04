/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once

#include <Util/include/allocators.h>
#include <string>


/* Global TAO namespace. */
namespace TAO
{

    /* Ledger Layer namespace. */
    namespace Ledger
    {


        /** PinUnlock
         *
         *  Encapsulates the PIN to unlock and allowable actions that can be performed on a signature chain
         *
         */
        class PinUnlock : public memory::encrypted
        {
        public:

            /* Enumeration of allowable actions that can be performed on an unlocked signature chain */
            enum UnlockActions
            {
                TRANSACTIONS    = (1 << 1),
                MINING          = (1 << 2),
                STAKING         = (1 << 3),
                NOTIFICATIONS   = (1 << 4),
                NONE            = (0 << 0),

                ALL = TRANSACTIONS | MINING | STAKING | NOTIFICATIONS
            };


            /** Default constructor. **/
            PinUnlock()
            : nUnlockedActions(UnlockActions::NONE)
            , strPIN()
            {
            }


            /** Constructor with pin and actions. **/
            PinUnlock(const SecureString& PIN, uint8_t UnlockedActions)
            : nUnlockedActions(UnlockedActions)
            , strPIN(PIN)
            {
            }


            /** CanTransact
             *
             *  Determins if the PIN can be used for transactions.
             *
             *  @return True if the PIN can be used for transactions.
             *
             **/
            bool CanTransact() const
            {
                return nUnlockedActions & TRANSACTIONS;
            }


            /** CanMine
             *
             *  Determins if the PIN can be used for mining.
             *
             *  @return True if the PIN can be used for mining.
             *
             **/
            bool CanMine() const
            {
                return nUnlockedActions & MINING;
            }


            /** CanStake
             *
             *  Determins if the PIN can be used for staking.
             *
             *  @return True if the PIN can be used for staking.
             *
             **/
            bool CanStake() const
            {
                return nUnlockedActions & STAKING;
            }


            /** ProcessNotifications
             *
             *  Determins if the PIN can be used for processing notifications.
             *
             *  @return True if the PIN can be used for processing notifications.
             *
             **/
            bool ProcessNotifications() const
            {
                return nUnlockedActions & NOTIFICATIONS;
            }


            /** PIN
             *
             *  Accessor for the PIN string.
             *
             *  @return the PIN.
             *
             **/
            SecureString PIN() const
            {
                return strPIN;
            }


            /** Encrypt
             *
             *  Special method for encrypting specific data types inside class.
             *
             **/
            void Encrypt()
            {
                encrypt(strPIN);
            }


            /** UnlockedActions
             *
             *  Provides access to the current unlocked actions set on this PIN
             *
             **/
            uint8_t UnlockedActions()
            {
                return nUnlockedActions;
            }


        protected:

            /* Bitmask of actions that can be performed on the sigchain when unlocked  */
            uint8_t nUnlockedActions;

            /** The PIN to unlock a signature chain. **/
            SecureString strPIN;

        };
    }
}
