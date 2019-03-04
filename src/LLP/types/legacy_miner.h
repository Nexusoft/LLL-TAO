/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People
____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLP_TYPES_LEGACY_MINER_H
#define NEXUS_LLP_TYPES_LEGACY_MINER_H

#include <LLP/types/base_miner.h>

namespace Legacy
{
    class ReserveKey;
    class LegacyBlock;
}

namespace LLP
{

    /** LegacyMiner
     *
     *  Connection class that handles requests and responses from miners.
     *
     **/
    class LegacyMiner : public BaseMiner
    {

    private:

        /** the mining key for block rewards to send **/
        Legacy::ReserveKey *pMiningKey;

    public:

        /** Name
         *
         *  Returns a string for the name of this type of Node.
         *
         **/
         static std::string Name()
         {
             return "LegacyMiner";
         }


        /** Default Constructor **/
        LegacyMiner();


        /** Constructor **/
        LegacyMiner(const Socket& SOCKET_IN, DDOS_Filter* DDOS_IN, bool isDDOS = false);


        /** Constructor **/
        LegacyMiner(DDOS_Filter* DDOS_IN, bool isDDOS = false);


        /** Default Destructor **/
        virtual ~LegacyMiner();


        /** SerializeBlock
         *
         *  Convert the Header of a Block into a Byte Stream for
         *  Reading and Writing Across Sockets.
         *
         *  @param[in] BLOCK A block to serialize.
         *
         **/
        std::vector<uint8_t> SerializeBlock(const TAO::Ledger::Block &BLOCK);


    private:


        /** new_block
         *
         *  Adds a new block to the map.
         *
         **/
         virtual TAO::Ledger::Block *new_block() override;


        /** validate_block
         *
         *  validates the block for the derived miner class.
         *
         **/
         virtual bool validate_block(const uint512_t &merkle_root) override;


         /** sign_block
          *
          *  validates the block for the derived miner class.
          *
          **/
          virtual bool sign_block(uint64_t nonce, const uint512_t &merkle_root) override;


          /** is_locked
           *
           *  determine if the mining wallet is unlocked
           *
           **/
          virtual bool is_locked() override;


    };
}

#endif
