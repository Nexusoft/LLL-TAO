/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People
____________________________________________________________________________________________*/


#include <LLP/types/legacy_miner.h>
#include <LLP/types/legacy.h>
#include <LLP/templates/events.h>
#include <LLP/templates/ddos.h>

#include <LLD/include/global.h>

#include <TAO/Ledger/include/constants.h>
#include <TAO/Ledger/include/supply.h>

#include <Legacy/include/create.h>
#include <Legacy/types/legacy.h>
#include <Legacy/wallet/wallet.h>
#include <Legacy/wallet/reservekey.h>

#include <Util/include/convert.h>


namespace LLP
{

    /** Default Constructor **/
    LegacyMiner::LegacyMiner()
    : BaseMiner()
    , pMiningKey(nullptr)
    {
        pMiningKey = new Legacy::ReserveKey(&Legacy::Wallet::GetInstance());
    }


    /** Constructor **/
    LegacyMiner::LegacyMiner(const Socket& SOCKET_IN, DDOS_Filter* DDOS_IN, bool isDDOS)
    : BaseMiner(SOCKET_IN, DDOS_IN, isDDOS)
    , pMiningKey(nullptr)
    {
        pMiningKey = new Legacy::ReserveKey(&Legacy::Wallet::GetInstance());
    }


    /** Constructor **/
    LegacyMiner::LegacyMiner(DDOS_Filter* DDOS_IN, bool isDDOS)
    : BaseMiner(DDOS_IN, isDDOS)
    , pMiningKey(nullptr)
    {
        pMiningKey = new Legacy::ReserveKey(&Legacy::Wallet::GetInstance());
    }


    /** Default Destructor **/
    LegacyMiner::~LegacyMiner()
    {
        if(pMiningKey)
            delete pMiningKey;
    }


    /* Adds a new block to the map. */
    TAO::Ledger::Block *LegacyMiner::new_block()
    {
        /*  Make a copy of the base block before making the hash  unique for this requst*/
        uint1024_t hashProof;

        /* Create a new Legacy Block. */
        Legacy::LegacyBlock *pBlock = new Legacy::LegacyBlock();

        /* Set it to a null state */
        pBlock->SetNull();

        /* If the primemod flag is set, take the hashProof down to 1017-bit to maximize prime ratio as much as possible. */
        uint32_t nBitMask = config::GetBoolArg(std::string("-primemod"), false) ? 0xFE000000 : 0x80000000;

        /* We need to make the block hash unique for each subsribed miner so that they are not
           duplicating their work.  To achieve this we take a copy of pBaseblock and then modify
           the scriptSig to be unique for each subscriber, before rebuilding the merkle tree.

           We need to drop into this for loop at least once to set the unique hash, but we will iterate
           indefinitely for the prime channel until the generated hash meets the min prime origins
           and is less than 1024 bits */
        while(true)
        {
            if(!Legacy::CreateBlock(*pMiningKey, CoinbaseTx, nChannel.load(), ++nBlockIterator, *pBlock))
                debug::error(FUNCTION, "Failed to create a new Legacy Block.");

            /* Get the proof hash. */
            hashProof = pBlock->ProofHash();

            /* Skip if not prime channel or version less than 5 */
            if(nChannel.load() != 1 || pBlock->nVersion < 5)
                break;

             /* Exit loop when the block is above minimum prime origins and less than
                 1024-bit hashes */
             if(hashProof > TAO::Ledger::bnPrimeMinOrigins.getuint1024() && !hashProof.high_bits(nBitMask))
                 break;
        }

        debug::log(2, FUNCTION, "Created new Legacy Block ", hashProof.SubString(), " nVersion=", pBlock->nVersion);

        /* Return a pointer to the heap memory */
        return pBlock;
    }


    /** Validates the block for the derived miner class. **/
    bool LegacyMiner::sign_block(uint64_t nNonce, const uint512_t& hashMerkleRoot)
    {
        Legacy::LegacyBlock *pBlock = dynamic_cast<Legacy::LegacyBlock *>(mapBlocks[hashMerkleRoot]);

        pBlock->nNonce = nNonce;
        pBlock->UpdateTime();
        pBlock->print(); // print pre-signed block to log, will print signed block in Accept()

        if(!Legacy::SignBlock(*pBlock, Legacy::Wallet::GetInstance()))
        {
            debug::log(2, FUNCTION, "Unable to Sign Legacy Block ", hashMerkleRoot.SubString());
            return false;
        }

        return true;
    }


    /** Validates the block for the derived miner class. **/
    bool LegacyMiner::validate_block(const uint512_t& hashMerkleRoot)
    {
        Legacy::LegacyBlock *pBlock = dynamic_cast<Legacy::LegacyBlock *>(mapBlocks[hashMerkleRoot]);

        /* Check the Proof of Work for submitted block. */
        if(!Legacy::CheckWork(*pBlock, Legacy::Wallet::GetInstance()))
        {
            debug::log(2, FUNCTION, "Invalid Work for Legacy Block ", hashMerkleRoot.SubString());

            /* Block not valid - Return the key (will reserve a new one if start a new block). */
            pMiningKey->ReturnKey();

            return false;
        }

        /* Block is valid - Tell the wallet to keep this key. */
        pMiningKey->KeepKey();

        return true;
    }


    /*  Determines if the mining wallet is unlocked. */
    bool LegacyMiner::is_locked()
    {
        return Legacy::Wallet::GetInstance().IsLocked();
    }
}
