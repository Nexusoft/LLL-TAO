/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2017] ++

			(c) Copyright The Nexus Developers 2014 - 2017

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"fides in stellis, virtus in numeris" - Faith in the Stars, Power in Numbers

____________________________________________________________________________________________*/

#ifndef NEXUS_TAO_LEGACY_TYPES_INCLUDE_OUTPOINT_H
#define NEXUS_TAO_LEGACY_TYPES_INCLUDE_OUTPOINT_H

#include "../../../LLC/types/uint1024.h"

namespace Legacy
{

	namespace Types
	{
		/** An outpoint - a combination of a transaction hash and an index n into its vout */
		class COutPoint
		{
		public:
			uint512 hash;
			unsigned int n;

			COutPoint() { SetNull(); }
			COutPoint(uint512 hashIn, unsigned int nIn) { hash = hashIn; n = nIn; }
			IMPLEMENT_SERIALIZE( READWRITE(FLATDATA(*this)); )

			void SetNull() { hash = 0; n = -1; }
			bool IsNull() const { return (hash == 0 && n == -1); }

			friend bool operator<(const COutPoint& a, const COutPoint& b)
			{
				return (a.hash < b.hash || (a.hash == b.hash && a.n < b.n));
			}

			friend bool operator==(const COutPoint& a, const COutPoint& b)
			{
				return (a.hash == b.hash && a.n == b.n);
			}

			friend bool operator!=(const COutPoint& a, const COutPoint& b)
			{
				return !(a == b);
			}

			std::string ToString() const;

			void print() const;
		};
    }
}
