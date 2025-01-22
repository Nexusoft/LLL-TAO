/*__________________________________________________________________________________________

			Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

			(c) Copyright The Nexus Developers 2014 - 2023

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To The Voice of The People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>
#include <LLP/include/global.h>

#include <TAO/API/types/authentication.h>
#include <TAO/API/types/indexing.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Sync's a user's indexing entries. */
    void Indexing::DownloadIndexes(const uint256_t& hashSession)
    {
        /* This is only for -client mode. */
        if(!config::fClient.load())
            return;

        /* Get our current genesis-id to start initialization. */
        const uint256_t hashGenesis =
            Authentication::Caller(hashSession);

        /* Broadcast our unconfirmed transactions first. */
        BroadcastUnconfirmed(hashGenesis);

        /* Process our sigchain events now. */
        DownloadNotifications(hashGenesis);
        DownloadSigchain(hashGenesis);
    }


    /*  Refresh our events and transactions for a given sigchain. */
    void Indexing::DownloadSigchain(const uint256_t& hashGenesis)
    {
        /* Check for client mode. */
        if(!config::fClient.load())
            return;

        /* Check for genesis. */
        if(LLP::TRITIUM_SERVER)
        {
            /* Find an active connection to sync from. */
            std::shared_ptr<LLP::TritiumNode> pNode = LLP::TRITIUM_SERVER->GetConnection();
            if(pNode != nullptr)
            {
                debug::log(0, FUNCTION, "CLIENT MODE: Synchronizing Sigchain");

                /* Get the last txid in sigchain. */
                uint512_t hashLast;
                LLD::Sessions->ReadLastConfirmed(hashGenesis, hashLast);

                do
                {
                    /* Request the sig chain. */
                    debug::log(0, FUNCTION, "CLIENT MODE: Requesting LIST::SIGCHAIN for ", hashGenesis.SubString());
                    LLP::TritiumNode::BlockingMessage
                    (
                        30000,
                        pNode.get(), LLP::TritiumNode::ACTION::LIST,
                        uint8_t(LLP::TritiumNode::TYPES::SIGCHAIN), hashGenesis, hashLast
                    );
                    debug::log(0, FUNCTION, "CLIENT MODE: LIST::SIGCHAIN received for ", hashGenesis.SubString());

                    /* Check for shutdown. */
                    if(config::fShutdown.load())
                        break;

                    uint512_t hashCurrent;
                    LLD::Sessions->ReadLastConfirmed(hashGenesis, hashCurrent);

                    if(hashCurrent == hashLast)
                    {
                        debug::log(0, FUNCTION, "CLIENT MODE: LIST::SIGCHAIN completed for ", hashGenesis.SubString());
                        break;
                    }
                }
                while(LLD::Sessions->ReadLastConfirmed(hashGenesis, hashLast));
            }
            else
                debug::error(FUNCTION, "no connections available...");
        }
    }


    /* Refresh our notifications for a given sigchain. */
    void Indexing::DownloadNotifications(const uint256_t& hashGenesis)
    {
        /* Check for client mode. */
        if(!config::fClient.load())
            return;

        /* Check for genesis. */
        if(LLP::TRITIUM_SERVER)
        {
            /* Find an active connection to sync from. */
            std::shared_ptr<LLP::TritiumNode> pNode = LLP::TRITIUM_SERVER->GetConnection();
            if(pNode != nullptr)
            {
                debug::log(0, FUNCTION, "CLIENT MODE: Synchronizing Notifications");

                /* Get our current tritium events sequence now. */
                uint32_t nTritiumSequence = 0;
                LLD::Sessions->ReadTritiumSequence(hashGenesis, nTritiumSequence);

                /* Loop until we have received all of our events. */
                do
                {
                    /* Request the sig chain. */
                    debug::log(0, FUNCTION, "CLIENT MODE: Requesting LIST::NOTIFICATION from ", nTritiumSequence, " for ", hashGenesis.SubString());
                    LLP::TritiumNode::BlockingMessage
                    (
                        30000,
                        pNode.get(), LLP::TritiumNode::ACTION::LIST,
                        uint8_t(LLP::TritiumNode::TYPES::NOTIFICATION), hashGenesis, nTritiumSequence
                    );
                    debug::log(0, FUNCTION, "CLIENT MODE: LIST::NOTIFICATION received for ", hashGenesis.SubString());

                    /* Check for shutdown. */
                    if(config::fShutdown.load())
                        break;

                    /* Cache our current sequence to see if we got any new events while waiting. */
                    uint32_t nCurrentSequence = 0;
                    LLD::Sessions->ReadTritiumSequence(hashGenesis, nCurrentSequence);

                    /* Check that are starting and current sequences match. */
                    if(nCurrentSequence == nTritiumSequence)
                    {
                        debug::log(0, FUNCTION, "CLIENT MODE: LIST::NOTIFICATION completed for ", hashGenesis.SubString());
                        break;
                    }
                }
                while(LLD::Sessions->ReadTritiumSequence(hashGenesis, nTritiumSequence));

                /* Get our current legacy events sequence now. */
                uint32_t nLegacySequence = 0;
                LLD::Sessions->ReadLegacySequence(hashGenesis, nLegacySequence);

                /* Loop until we have received all of our events. */
                do
                {
                    /* Request the sig chain. */
                    debug::log(0, FUNCTION, "CLIENT MODE: Requesting LIST::LEGACY::NOTIFICATION from ", nLegacySequence, " for ", hashGenesis.SubString());
                    LLP::TritiumNode::BlockingMessage
                    (
                        30000,
                        pNode.get(), LLP::TritiumNode::ACTION::LIST,
                        uint8_t(LLP::TritiumNode::SPECIFIER::LEGACY), uint8_t(LLP::TritiumNode::TYPES::NOTIFICATION),
                        hashGenesis, nLegacySequence
                    );
                    debug::log(0, FUNCTION, "CLIENT MODE: LIST::LEGACY::NOTIFICATION received for ", hashGenesis.SubString());

                    /* Check for shutdown. */
                    if(config::fShutdown.load())
                        break;

                    /* Cache our current sequence to see if we got any new events while waiting. */
                    uint32_t nCurrentSequence = 0;
                    LLD::Sessions->ReadLegacySequence(hashGenesis, nCurrentSequence);

                    /* Check that are starting and current sequences match. */
                    if(nCurrentSequence == nLegacySequence)
                    {
                        debug::log(0, FUNCTION, "CLIENT MODE: LIST::LEGACY::NOTIFICATION completed for ", hashGenesis.SubString());
                        break;
                    }
                }
                while(LLD::Sessions->ReadLegacySequence(hashGenesis, nLegacySequence));
            }
            else
                debug::error(FUNCTION, "no connections available...");
        }
    }
}
