/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2017] ++

			(c) Copyright The Nexus Developers 2014 - 2017

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"fides in stellis, virtus in numeris" - Faith in the Stars, Power in Numbers

____________________________________________________________________________________________*/

#ifndef NEXUS_TAO_LEGACY_TYPES_INCLUDE_INPOINT_H
#define NEXUS_TAO_LEGACY_TYPES_INCLUDE_INPOINT_H


namespace Legacy
{

	namespace Types
	{

		/** An inpoint - a combination of a transaction and an index n into its vin */
		class CInPoint
		{
		public:
			CTransaction* ptx;
			unsigned int n;

			CInPoint() { SetNull(); }
			CInPoint(CTransaction* ptxIn, unsigned int nIn) { ptx = ptxIn; n = nIn; }
			void SetNull() { ptx = NULL; n = -1; }
			bool IsNull() const { return (ptx == NULL && n == -1); }
		};

	}
}
