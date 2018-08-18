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
#include "../../../Util/templates/serialize"

namespace Legacy
{

	namespace Types
	{
		/** An outpoint - a combination of a transaction hash and an index n into its vout */
		class COutPoint
		{
		public:

			/** The hash of the previous transaction being spent. **/
			uint512 hash;


			/** The output number of the previous transaction being spent. **/
			unsigned int n;


			//the serizliation methods
			SERIALIZE_HEADER


			/** Constructor.
			 *
			 *	Set state to Null.
			 *
			 **/
			COutPoint() { SetNull(); }


			/** Constructor
			 *
			 *	@param[in] hashIn The input hash of previous transaction.
			 *	@param[in] nIn The output number of previous transaction.
			 *
			 **/
			COutPoint(uint512 hashIn, unsigned int nIn) { hash = hashIn; n = nIn; }


			/** SetNull
			 *
			 *	Set state to Null.
			 *
			 **/
			void SetNull()
			{
				hash = 0;
				n = -1;
			}


			/** IsNull
			 *
			 *	Check state is null
			 *
			 *	@return true if the object is in a null state.
			 *
			 **/
			bool IsNull() const
			{
				return (hash == 0 && n == -1);
			}


			/** Less Than Operator Overload
			 *
			 *	@param[in] a First object to be compared
			 *	@param[in] b Second object to be compared
			 *
			 *	@return true if a < b
			 *
			 **/
			friend bool operator<(const COutPoint& a, const COutPoint& b)
			{
				return (a.hash < b.hash || (a.hash == b.hash && a.n < b.n));
			}


			/** Equals Operator Overload
			 *
			 *	@param[in] a First object to be compared
			 *	@param[in] b Second object to be compared
			 *
			 *	@return true if a == b
			 *
			 **/
			friend bool operator==(const COutPoint& a, const COutPoint& b)
			{
				return (a.hash == b.hash && a.n == b.n);
			}


			/** Not Equals Operator Overload
			 *
			 *	@param[in] a First object to be compared
			 *	@param[in] b Second object to be compared
			 *
			 *	@return true if a != b
			 *
			 **/
			friend bool operator!=(const COutPoint& a, const COutPoint& b)
			{
				return !(a == b);
			}


			/** ToString
			 *
			 *	Full object debug output
			 *
			 *	@return string containing the object output.
			 *
			 **/
			std::string ToString() const;


			/** print
			 *
			 *	Dump the full object to the console (stdout)
			 *
			 **/
			void print() const;
		};
    }
}
