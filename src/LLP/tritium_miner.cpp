/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People
____________________________________________________________________________________________*/


#include <LLP/types/tritium_miner.h>
#include <LLP/templates/events.h>
#include <LLP/templates/ddos.h>
#include <LLP/types/tritium.h>

#include <LLD/include/global.h>

#include <TAO/Ledger/include/constants.h>
#include <TAO/Ledger/include/supply.h>

#include <TAO/Ledger/include/create.h>
#include <TAO/Ledger/types/tritium.h>
#include <TAO/Ledger/types/sigchain.h>
#include <TAO/Ledger/types/transaction.h>
#include <TAO/Ledger/types/mempool.h>

#include <TAO/API/include/accounts.h>

#include <Util/include/convert.h>
#include <Util/include/args.h>

namespace LLP
{

    /** Default Constructor **/
    TritiumMiner::TritiumMiner()
    : BaseMiner()
    {
    }


    /** Constructor **/
    TritiumMiner::TritiumMiner(const Socket& SOCKET_IN, DDOS_Filter* DDOS_IN, bool isDDOS)
    : BaseMiner(SOCKET_IN, DDOS_IN, isDDOS)
    {
    }


    /** Constructor **/
    TritiumMiner::TritiumMiner(DDOS_Filter* DDOS_IN, bool isDDOS)
    : BaseMiner(DDOS_IN, isDDOS)
    {
    }


    /** Default Destructor **/
    TritiumMiner::~TritiumMiner()
    {
    }


    /** new_block
     *
     *  Adds a new block to the map.
     *
     **/
     TAO::Ledger::Block *TritiumMiner::new_block()
     {
         /*  make a copy of the base block before making the hash  unique for this requst*/
         uint1024_t proof_hash;
         uint32_t s = static_cast<uint32_t>(mapBlocks.size());

         /* Create a new Tritium Block. */
         TAO::Ledger::TritiumBlock *pBlock = new TAO::Ledger::TritiumBlock();

         /* Set it to a null state */
         pBlock->SetNull();

         /* Get the sigchain and the PIN. */
         uint64_t nSession;
         SecureString PIN;

         /* Attempt to unlock the account. */
         if(!TAO::API::accounts.Locked(nSession, PIN))
            debug::error(FUNCTION, "No unlocked account available");

         /* Ateempt to get the sigchain. */
         TAO::Ledger::SignatureChain* pSigChain;
         if(!TAO::API::accounts.GetAccount(nSession, pSigChain))
            debug::error(FUNCTION, "Couldn't get the unlocked sigchain");


         /* We need to make the block hash unique for each subsribed miner so that they are not
             duplicating their work.  To achieve this we take a copy of pBaseblock and then modify
             the scriptSig to be unique for each subscriber, before rebuilding the merkle tree.

             We need to drop into this for loop at least once to set the unique hash, but we will iterate
             indefinitely for the prime channel until the generated hash meets the min prime origins
             and is less than 1024 bits*/
         for(uint64_t i = s; ; ++i)
         {
             //pBlock->vtx[0].vin[0].scriptSig = (Legacy::Script() <<  (uint64_t)((i+1) * 513513512151));

             /* Rebuild the merkle tree. */
             //std::vector<uint512_t> vMerkleTree;
             //for(const auto& tx : pBlock->vtx)
             //    vMerkleTree.push_back(tx.GetHash());

             //pBlock->hashMerkleRoot = pBlock->BuildMerkleTree(vMerkleTree);

             //TODO:
             if(!TAO::Ledger::CreateBlock(pSigChain, PIN, nChannel, *pBlock, i))
                 debug::error(FUNCTION, "Failed to create a new Tritium Block.");

             /* Update the time. */
             pBlock->UpdateTime();

             /* skip if not prime channel or version less than 5 */
             if(nChannel != 1 || pBlock->nVersion >= 5)
                 break;

             proof_hash = pBlock->ProofHash();

             /* exit loop when the block is above minimum prime origins and less than
                 1024-bit hashes */
             if(proof_hash > TAO::Ledger::bnPrimeMinOrigins.getuint1024()
             && !proof_hash.high_bits(0x80000000))
                 break;
         }

         debug::log(2, FUNCTION, "Created new Tritium Block ",
             pBlock->hashMerkleRoot.ToString().substr(0, 20));

         /* Return a pointer to the heap memory */
         return pBlock;
     }


    /** validates the block for the derived miner class. **/
    bool TritiumMiner::validate_block(const uint512_t &merkle_root)
    {
        /* Create the pointer to the heap. */
        TAO::Ledger::TritiumBlock *pBlock = dynamic_cast<TAO::Ledger::TritiumBlock *>(mapBlocks[merkle_root]);

        /* Validate the block through the process method. */
        TritiumNode::Process(*pBlock, nullptr);

        return true;
    }


     /** validates the block for the derived miner class. **/
     bool TritiumMiner::sign_block(uint64_t nonce, const uint512_t &merkle_root)
     {
         /* Create the pointer to the heap. */
         TAO::Ledger::TritiumBlock *pBlock = dynamic_cast<TAO::Ledger::TritiumBlock *>(mapBlocks[merkle_root]);
         pBlock->nNonce = nonce;
         pBlock->UpdateTime();
         pBlock->print();

         /* Get the sigchain and the PIN. */
         uint64_t nSession;
         SecureString PIN;

         /* Attempt to unlock the account. */
         if(!TAO::API::accounts.Locked(nSession, PIN))
            return debug::error(FUNCTION, "No unlocked account available");

         /* Ateempt to get the sigchain. */
         TAO::Ledger::SignatureChain* pSigChain;
         if(!TAO::API::accounts.GetAccount(nSession, pSigChain))
            return debug::error(FUNCTION, "Couldn't get the unlocked sigchain");

         /* Sign the submitted block */
         std::vector<uint8_t> vBytes = pSigChain->Generate(pBlock->producer.nSequence, PIN).GetBytes();
         LLC::CSecret vchSecret(vBytes.begin(), vBytes.end());

         /* Generate the EC Key and new block signature. */
         LLC::ECKey key(LLC::BRAINPOOL_P512_T1, 64);
         if(!key.SetSecret(vchSecret, true)
         || !pBlock->GenerateSignature(key))
         {
             debug::log(2, "Unable to Sign Tritium Block ", merkle_root.ToString().substr(0, 20));
             return false;
         }

         return true;
     }


     /*  Determines if the mining wallet is unlocked. */
     bool TritiumMiner::is_locked()
     {
         /* Get the sigchain and the PIN. */
         uint64_t nSession;
         SecureString PIN;

         /* Attempt to unlock the account. */
         return !TAO::API::accounts.Locked(nSession, PIN);
     }

}
