/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2017] ++

            (c) Copyright The Nexus Developers 2014 - 2017

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "fides in stellis, virtus in numeris" - Faith in the Stars, Power in Numbers

____________________________________________________________________________________________*/

#ifndef NEXUS_TAO_LEDGER_TYPES_BLOCK_H
#define NEXUS_TAO_LEDGER_TYPES_BLOCK_H

#include "legacy_tx.h"
#include "../../../../LLC/hash/macro.h"

namespace TAO
{
    namespace Ledger
    {

        /** LEGACY: TO BE DEPRECATED:
         *
         * Describes a place in the block chain to another node such that if the
         * other node doesn't have the same branch, it can find a recent common trunk.
         * The further back it is, the further before the fork it may be.
         */
        class CBlockLocator
        {
        protected:
            std::vector<uint1024> vHave;

        public:

            IMPLEMENT_SERIALIZE
            (
                if (!(nSerType & SER_GETHASH))
                    READWRITE(nVersion);
                READWRITE(vHave);
            )

            CBlockLocator()
            {
            }

            explicit CBlockLocator(const CBlockIndex* pindex)
            {
                Set(pindex);
            }

            explicit CBlockLocator(uint1024 hashBlock)
            {
                std::map<uint1024, CBlockIndex*>::iterator mi = mapBlockIndex.find(hashBlock);
                if (mi != mapBlockIndex.end())
                    Set((*mi).second);
            }


            /* Set by List of Vectors. */
            CBlockLocator(const std::vector<uint1024>& vHaveIn)
            {
                vHave = vHaveIn;
            }


            /* Set the State of Object to nullptr. */
            void SetNull()
            {
                vHave.clear();
            }


            /* Check the State of Object as nullptr. */
            bool IsNull()
            {
                return vHave.empty();
            }


            /* Set from Block Index Object. */
            void Set(const CBlockIndex* pindex);


            /* Find the total blocks back locator determined. */
            int GetDistanceBack();


            /* Get the Index object stored in Locator. */
            CBlockIndex* GetBlockIndex();


            /* Get the hash of block. */
            uint1024 GetBlockHash();


            /* bdg note: GetHeight is never used. */
            /* Get the Height of the Locator. */
            int GetHeight();

        };

    }

}

#endif
