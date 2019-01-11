/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/Ledger/include/constants.h>
#include <TAO/Ledger/types/tritium.h>
#include <TAO/Ledger/types/trustkey.h>

#include <Util/include/debug.h>
#include <Util/include/runtime.h>

/* Global TAO namespace. */
namespace TAO
{

    /* Ledger Layer namespace. */
    namespace Ledger
    {

        /* Initializes a null Trust Key. */
        TrustKey::TrustKey()
        {
            SetNull();
        }


        /* Initializes a Trust Key. */
        TrustKey::TrustKey(std::vector<uint8_t> vchPubKeyIn, uint1024_t hashBlockIn, uint512_t hashTxIn, int32_t nTimeIn)
        {
            SetNull();

            nVersion               = 1;
            vchPubKey              = vchPubKeyIn;
            hashGenesisBlock       = hashBlockIn;
            hashGenesisTx          = hashTxIn;
            nGenesisTime           = nTimeIn;
        }


        /* Set the Trust Key data to Null (uninitialized) values. */
        void TrustKey::SetNull()
        {
            nVersion             = 1;
            hashGenesisBlock     = 0;
            hashGenesisTx        = 0;
            nGenesisTime         = 0;

            hashPrevBlocks.clear();
            vchPubKey.clear();
        }


        /* Retrieve how old the Trust Key is at a given point in time. */
        uint64_t TrustKey::GetAge(const uint32_t nTime) const
        {
            if (nTime < nGenesisTime)
            {
                return 0;
            }
            else
            {
                return (uint64_t)(nTime - nGenesisTime);
            }
        }


        /* Retrieve the time since this Trust Key last generated a Proof of Stake block. */
        uint64_t TrustKey::GetBlockAge(const uint32_t nTime) const
        {
            //TODO requires implementation
            return 0; //temp until implementation completed
        }


        /* Determine if a key is expired at a given point in time. */
        bool TrustKey::IsExpired(uint32_t nTime) const
        {
            if (GetBlockAge(nTime) > TRUST_KEY_EXPIRE)
                return true;

            return false;
        }


        /* Check the Genesis Transaction of this Trust Key. */
        bool TrustKey::CheckGenesis(TritiumBlock block) const
        {
            /* Invalid if Null. */
            if(IsNull())
                return false;

            /* Trust Keys must be created from only Proof of Stake Blocks. */
            if(!block.IsProofOfStake())
                return debug::error("TrustKey::CheckGenesis() : genesis has to be proof of stake");

            /* Trust Key Timestamp must be the same as Genesis Key Block Timestamp. */
            if(nGenesisTime != block.nTime)
                return debug::error("TrustKey::CheckGenesis() : genesis time mismatch");

            /* Check the genesis block hash. */
            if(block.GetHash() != hashGenesisBlock)
                return debug::error("TrustKey::CheckGenesis() : genesis hash mismatch");

            /* Genesis Transaction must match Trust Key Genesis Hash. */
            if (block.vtx.size() == 0)
                return debug::error("TrustKey::CheckGenesis() : genesis coinstake not present");

            uint512_t txHash = block.vtx[0].second;

            if(txHash != hashGenesisTx)
                return debug::error("TrustKey::CheckGenesis() : genesis coinstake hash mismatch");

            return true;
        }


        /* Generate a string representation of this Trust Key. */
        std::string TrustKey::ToString()
        {
            uint576_t cKey;
            cKey.SetBytes(vchPubKey);

            return debug::strprintf("hash=%s, key=%s, genesis=%s, tx=%s, time=%u, age=%u",
                                    GetHash().ToString().c_str(),
                                    cKey.ToString().c_str(),
                                    hashGenesisBlock.ToString().c_str(),
                                    hashGenesisTx.ToString().c_str(),
                                    nGenesisTime,
                                    GetAge(runtime::unifiedtimestamp()));
        }


        /* Print a string representation of this Trust Key to the debug log. */
        void TrustKey::Print()
        {
            debug::log(0, "TrustKey(", ToString(), ")");
        }
    }
}
