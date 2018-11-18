/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++

			(c) Copyright The Nexus Developers 2014 - 2018

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_LEGACY_WALLET_TXDB_H
#define NEXUS_LEGACY_WALLET_TXDB_H

#include "../core/core.h"

#include <string>
#include <vector>

#include <LLC/types/uint1024.h>

#include <TAO/Legacy/wallet/db.h>

namespace Legacy
{
    
    /* forward declarations */    
    namespace Types
    {
        class CDiskBlockIndex;
        class CDiskTxPos;
        class COutPoint;
        class CTransaction;
        class CTxIndex;
    }

    namespace Wallet
    {
        /** Access to the transaction database (blkindex.dat) */
        class CTxDB : public CDB
        {
        public:
            CTxDB(const char* pszMode="r+") : CDB("blkindex.dat", pszMode) { }

        private:
            CTxDB(const CTxDB&);
            void operator=(const CTxDB&);

        public:
            bool ReadTxIndex(uint512_t hash, Legacy::Types::CTxIndex& txindex);
            bool UpdateTxIndex(uint512_t hash, const Legacy::Types::CTxIndex& txindex);
            bool AddTxIndex(const Legacy::Types::CTransaction& tx, const Legacy::Types::CDiskTxPos& pos, int nHeight);
            bool EraseTxIndex(const Legacy::Types::CTransaction& tx);
            bool ContainsTx(uint512_t hash);
            bool ReadOwnerTxes(uint512_t hash, int nHeight, std::vector<Legacy::Types::CTransaction>& vtx);
            bool ReadDiskTx(uint512_t hash, Legacy::Types::CTransaction& tx, Legacy::Types::CTxIndex& txindex);
            bool ReadDiskTx(uint512_t hash, Legacy::Types::CTransaction& tx);
            bool ReadDiskTx(Legacy::Types::COutPoint outpoint, Legacy::Types::CTransaction& tx, Legacy::Types::CTxIndex& txindex);
            bool ReadDiskTx(Legacy::Types::COutPoint outpoint, Legacy::Types::CTransaction& tx);
            bool WriteBlockIndex(const Legacy::Types::CDiskBlockIndex& blockindex);
            bool EraseBlockIndex(uint1024_t hash);
            bool ReadHashBestChain(uint1024_t& hashBestChain);
            bool WriteHashBestChain(uint1024_t hashBestChain);
            bool ReadCheckpointPubKey(std::string& strPubKey);
            bool WriteCheckpointPubKey(const std::string& strPubKey);
        };

    }
}

#endif
