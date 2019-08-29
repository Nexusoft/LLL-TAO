/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2019

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <Legacy/include/trust.h>

#include <Legacy/include/enum.h>
#include <Legacy/include/evaluate.h>

#include <Legacy/types/transaction.h>
#include <Legacy/types/trustkey.h>
#include <Legacy/types/txin.h>

#include <LLD/include/global.h>

#include <TAO/Ledger/include/constants.h>
#include <TAO/Ledger/types/state.h>

#include <Util/include/debug.h>

#include <vector>

/* Global TAO namespace. */
namespace Legacy
{

    /* Find the last trust block of given key. */
    bool GetLastTrust(const TrustKey& trustKey, TAO::Ledger::BlockState& state)
    {
        /* Loop through all previous blocks looking for most recent trust block. */
        std::vector<uint8_t> vTrustKey;
        while(vTrustKey != trustKey.vchPubKey)
        {
            /* Get the last state. */
            state = state.Prev();
            if(!GetLastState(state, 0))
                return debug::error(FUNCTION, "couldn't find previous block");

            /* Check for genesis. */
            if(state.GetHash() == trustKey.hashGenesisBlock)
                return true;

            /* If serach block isn't proof of stake, return an error. */
            if(!state.IsProofOfStake())
                return debug::error(FUNCTION, "block is not proof of stake");

            /* Get the previous coinstake transaction. */
            Legacy::Transaction tx;
            if(!LLD::Legacy->ReadTx(state.vtx[0].second, tx))
                return debug::error(FUNCTION, "failed to read coinstake from legacy DB");

            /* Extract the trust key from coinstake. */
            if(!tx.TrustKey(vTrustKey))
                return debug::error(FUNCTION, "couldn't extract trust key");
        }

        return true;
    }


    /* Find the genesis block of given trust key. */
    bool FindGenesis(const uint576_t& cKey, const uint1024_t& hashTrustBlock, TrustKey& trustKey)
    {
        /* Debug output to monitor the calling of this function. */
        debug::error(FUNCTION, "no genesis found triggered. Recovering...");

        /* Create the state object. */
        TAO::Ledger::BlockState state;
        if(!LLD::Ledger->ReadBlock(hashTrustBlock, state))
            return debug::error(FUNCTION, "couldn't find hash trust block");

        /* Loop through all previous blocks looking for most recent trust block. */
        std::vector<uint8_t> vTrustKey;
        while(hashTrustBlock != TAO::Ledger::hashGenesis)
        {
            /* Get the last state. */
            state = state.Prev();
            if(!GetLastState(state, 0))
                return debug::error(FUNCTION, "couldn't find previous block");

            /* If serach block isn't proof of stake, return an error. */
            if(!state.IsProofOfStake())
                return debug::error(FUNCTION, "block is not proof of stake");

            /* Get the previous coinstake transaction. */
            Legacy::Transaction tx;
            if(!LLD::Legacy->ReadTx(state.vtx[0].second, tx))
                return debug::error(FUNCTION, "failed to read coinstake from legacy DB");

            /* Extract the trust key from coinstake. */
            if(!tx.TrustKey(vTrustKey))
                return debug::error(FUNCTION, "couldn't extract trust key");

            /* Check for genesis. */
            uint576_t keyTest;
            keyTest.SetBytes(vTrustKey);
            if(keyTest == cKey && tx.IsGenesis())
            {
                trustKey = TrustKey(vTrustKey, state.GetHash(), tx.GetHash(), state.nTime);

                return true;
            }
        }

        return false;
    }


    /* Extract the trust key being migrated from the Legacy migration transaction. */
    bool FindMigratedTrustKey(const Transaction& tx, TrustKey& trustKey)
    {
        std::vector<uint8_t> vchTrustKey;

        /* Check that all inputs are from the same key and extract the pub key for it.
         * Typical migration transaction will have one input, but it is feasible to have multiple.
         * If more than one, they all must be from the same key.
         */
        for(uint32_t nInput = 0; nInput < tx.vin.size(); ++nInput)
        {
            const Legacy::TxIn& txin = tx.vin[nInput];

            /* Get prevout for the txin */
            Legacy::Transaction txPrev;
            if(!LLD::Legacy->ReadTx(txin.prevout.hash, txPrev))
                return false;

            /* Extract from prevout */
            std::vector<std::vector<uint8_t> > vSolutions;
            Legacy::TransactionType whichType;
            if(!Solver(txPrev.vout[txin.prevout.n].scriptPubKey, whichType, vSolutions))
                return false;

            /* Check for public key type. */
            if(whichType == Legacy::TX_PUBKEY)
            {
                if(nInput == 0)
                    vchTrustKey = vSolutions[0]; //Save this as the pub key for the trust key

                else if(vchTrustKey != vSolutions[0])
                    return false; // Inputs not all from same address
            }
            else
                return false;
        }

        uint576_t cKey;
        cKey.SetBytes(vchTrustKey);

        return LLD::Trust->ReadTrustKey(cKey, trustKey);
    }

}
