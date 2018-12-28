/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/Ledger/include/create.h>
#include <TAO/Ledger/include/constants.h>
#include <TAO/Ledger/include/timelocks.h>

#include <Util/include/runtime.h>
#include <Util/include/args.h>

namespace TAO::Ledger
{

    /* Create a new transaction object from signature chain. */
    bool CreateTransaction(TAO::Ledger::SignatureChain* user, TAO::Ledger::Transaction& tx)
    {

    }


    /* Create a new block object from the chain. */
    bool CreateBlock(TAO::Ledger::SignatureChain* user, TAO::Ledger::TritiumBlock& block)
    {
        /* Set the block to null. */
        block.SetNull();


        /* Modulate the Block Versions if they correspond to their proper time stamp */
        if(runtime::UnifiedTimestamp() >= (config::fTestNet ?
            TESTNET_VERSION_TIMELOCK[TESTNET_BLOCK_CURRENT_VERSION - 2] :
            NETWORK_VERSION_TIMELOCK[NETWORK_BLOCK_CURRENT_VERSION - 2]))
            block.nVersion = config::fTestNet ?
            TESTNET_BLOCK_CURRENT_VERSION :
            NETWORK_BLOCK_CURRENT_VERSION; // --> New Block Versin Activation Switch
        else
            block.nVersion = config::fTestNet ?
            TESTNET_BLOCK_CURRENT_VERSION - 1 :
            NETWORK_BLOCK_CURRENT_VERSION - 1;


        /** Create the Coinbase / Coinstake Transaction. **/
        CTransaction txNew;
        txNew.vin.resize(1);
        txNew.vin[0].prevout.SetNull();

        /** Set the First output to Reserve Key. **/
        txNew.vout.resize(1);


        /** Create the Coinstake Transaction if on Proof of Stake Channel. **/
        if (nChannel == 0)
        {
            /** Mark the Coinstake Transaction with First Input Byte Signature. **/
            txNew.vin[0].scriptSig.resize(8);
            txNew.vin[0].scriptSig[0] = 1;
            txNew.vin[0].scriptSig[1] = 2;
            txNew.vin[0].scriptSig[2] = 3;
            txNew.vin[0].scriptSig[3] = 5;
            txNew.vin[0].scriptSig[4] = 8;
            txNew.vin[0].scriptSig[5] = 13;
            txNew.vin[0].scriptSig[6] = 21;
            txNew.vin[0].scriptSig[7] = 34;

            /** Update the Coinstake Timestamp. **/
            txNew.nTime = pindexPrev->GetBlockTime() + 1;
        }

        /** Create the Coinbase Transaction if the Channel specifies. **/
        else
        {
            /** Set the Coinbase Public Key. **/
            txNew.vout[0].scriptPubKey << reservekey.GetReservedKey() << Wallet::OP_CHECKSIG;

            /** Set the Proof of Work Script Signature. **/
            txNew.vin[0].scriptSig = (Wallet::CScript() << nID * 513513512151);

            /** Customized Coinbase Transaction if Submitted. **/
            if(pCoinbase)
            {

                /** Dummy Transaction to Allow the Block to be Signed by Pool Wallet. [For Now] **/
                txNew.vout[0].nValue = pCoinbase->nPoolFee;

                unsigned int nTx = 1;
                txNew.vout.resize(pCoinbase->vOutputs.size() + 1);
                for(std::map<std::string, uint64>::iterator nIterator = pCoinbase->vOutputs.begin(); nIterator != pCoinbase->vOutputs.end(); nIterator++)
                {
                    /** Set the Appropriate Outputs. **/
                    txNew.vout[nTx].scriptPubKey.SetNexusAddress(nIterator->first);
                    txNew.vout[nTx].nValue = nIterator->second;

                    nTx++;
                }

                int64 nMiningReward = 0;
                for(int nIndex = 0; nIndex < txNew.vout.size(); nIndex++)
                    nMiningReward += txNew.vout[nIndex].nValue;

                /** Double Check the Coinbase Transaction Fits in the Maximum Value. **/
                if(nMiningReward != GetCoinbaseReward(pindexPrev, nChannel, 0))
                    return cBlock;

            }
            else
                txNew.vout[0].nValue = GetCoinbaseReward(pindexPrev, nChannel, 0);

            /* Make coinbase counter mod 13 of height. */
            int nCoinbaseCounter = pindexPrev->nHeight % 13;

            /** Set the Proper Addresses for the Coinbase Transaction. **/
            txNew.vout.resize(txNew.vout.size() + 2);
            txNew.vout[txNew.vout.size() - 2].scriptPubKey.SetNexusAddress(fTestNet ? (cBlock.nVersion < 5 ? TESTNET_DUMMY_ADDRESS : TESTNET_DUMMY_AMBASSADOR_RECYCLED) : (cBlock.nVersion < 5 ? CHANNEL_ADDRESSES[nCoinbaseCounter] : AMBASSADOR_ADDRESSES_RECYCLED[nCoinbaseCounter]));

            txNew.vout[txNew.vout.size() - 1].scriptPubKey.SetNexusAddress(fTestNet ? (cBlock.nVersion < 5 ? TESTNET_DUMMY_ADDRESS : TESTNET_DUMMY_DEVELOPER_RECYCLED) : (cBlock.nVersion < 5 ? DEVELOPER_ADDRESSES[nCoinbaseCounter] : DEVELOPER_ADDRESSES_RECYCLED[nCoinbaseCounter]));

            /* Set the Proper Coinbase Output Amounts for Recyclers and Developers. */
            txNew.vout[txNew.vout.size() - 2].nValue = GetCoinbaseReward(pindexPrev, nChannel, 1);
            txNew.vout[txNew.vout.size() - 1].nValue = GetCoinbaseReward(pindexPrev, nChannel, 2);
        }

        /** Add our Coinbase / Coinstake Transaction. **/
        cBlock.vtx.push_back(txNew);

        /** Add in the Transaction from Memory Pool only if it is not a Genesis. **/
        if(nChannel > 0)
            AddTransactions(cBlock.vtx, pindexPrev);

        /** Populate the Block Data. **/
        cBlock.hashPrevBlock  = pindexPrev->GetBlockHash();
        cBlock.hashMerkleRoot = cBlock.BuildMerkleTree();
        cBlock.nChannel       = nChannel;
        cBlock.nHeight        = pindexPrev->nHeight + 1;
        cBlock.nBits          = GetNextTargetRequired(pindexPrev, cBlock.GetChannel(), false);
        cBlock.nNonce         = 1;

        cBlock.UpdateTime();

        return cBlock;
    }

}
