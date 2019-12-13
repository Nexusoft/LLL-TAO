/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/Ledger/include/stake_change.h>

#include <LLC/hash/SK.h>
#include <LLP/include/version.h>
#include <Util/include/runtime.h>
#include <Util/templates/datastream.h>

/* Global TAO namespace. */
namespace TAO
{

    /* Ledger Layer namespace. */
    namespace Ledger
    {

        /* Default Constructor. */
        StakeChange::StakeChange()
        : nVersion    (1)
        , hashGenesis (0)
        , nAmount     (0)
        , hashLast    (0)
        , nTime       (runtime::unifiedtimestamp())
        , nExpires    (0)
        , fProcessed  (false)
        , hashTx      (0)
        , vchPubKey   ( )
        , vchSig      ( )
        {
        }


        /* Copy constructor. */
        StakeChange::StakeChange(const StakeChange& stakeChange)
        : nVersion    (stakeChange.nVersion)
        , hashGenesis (stakeChange.hashGenesis)
        , nAmount     (stakeChange.nAmount)
        , hashLast    (stakeChange.hashLast)
        , nTime       (stakeChange.nTime)
        , nExpires    (stakeChange.nExpires)
        , fProcessed  (stakeChange.fProcessed)
        , hashTx      (stakeChange.hashTx)
        , vchPubKey   (stakeChange.vchPubKey)
        , vchSig      (stakeChange.vchSig)
        {
        }


        /* Move constructor. */
        StakeChange::StakeChange(StakeChange&& stakeChange) noexcept
        : nVersion    (std::move(stakeChange.nVersion))
        , hashGenesis (std::move(stakeChange.hashGenesis))
        , nAmount     (std::move(stakeChange.nAmount))
        , hashLast    (std::move(stakeChange.hashLast))
        , nTime       (std::move(stakeChange.nTime))
        , nExpires    (std::move(stakeChange.nExpires))
        , fProcessed  (std::move(stakeChange.fProcessed))
        , hashTx      (std::move(stakeChange.hashTx))
        , vchPubKey   (std::move(stakeChange.vchPubKey))
        , vchSig      (std::move(stakeChange.vchSig))
        {
        }


        /* Copy assignment. */
        StakeChange& StakeChange::operator=(const StakeChange& stakeChange)
        {
            nVersion    = stakeChange.nVersion;
            hashGenesis = stakeChange.hashGenesis;
            nAmount     = stakeChange.nAmount;
            hashLast    = stakeChange.hashLast;
            nTime       = stakeChange.nTime;
            nExpires    = stakeChange.nExpires;
            fProcessed  = stakeChange.fProcessed;
            hashTx      = stakeChange.hashTx;
            vchPubKey   = stakeChange.vchPubKey;
            vchSig      = stakeChange.vchSig;

            return *this;
        }


        /* Move assignment. */
        StakeChange& StakeChange::operator=(StakeChange&& stakeChange) noexcept
        {
            nVersion    = std::move(stakeChange.nVersion);
            hashGenesis = std::move(stakeChange.hashGenesis);
            nAmount     = std::move(stakeChange.nAmount);
            hashLast    = std::move(stakeChange.hashLast);
            nTime       = std::move(stakeChange.nTime);
            nExpires    = std::move(stakeChange.nExpires);
            fProcessed  = std::move(stakeChange.fProcessed);
            hashTx      = std::move(stakeChange.hashTx);
            vchPubKey   = std::move(stakeChange.vchPubKey);
            vchSig      = std::move(stakeChange.vchSig);

            return *this;
        }


        /* Resets all data in this stake change request. */
        void StakeChange::SetNull()
        {
            hashGenesis = 0;
            nAmount = 0;
            hashLast = 0;
            nTime = runtime::unifiedtimestamp();
            nExpires = 0;
            fProcessed = false;
            hashTx = 0;
            vchPubKey.clear();
            vchSig.clear();
        }


        /* Get the hash for the stake change request. */
        uint256_t StakeChange::GetHash() const
        {
            DataStream ss(SER_GETHASH, LLP::PROTOCOL_VERSION);

            /* Hash the static data for the change request */
            ss << nVersion << hashGenesis << nAmount << hashLast << nTime << nExpires;

            /* Get the hash. */
            uint256_t hash = LLC::SK256(ss.begin(), ss.end());

            return hash;
        }
    }
}
