/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2019

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_LEGACY_TYPES_OUTPOINT_H
#define NEXUS_LEGACY_TYPES_OUTPOINT_H

#include <LLC/types/uint1024.h>
#include <Util/templates/serialize.h>
#include <Util/templates/flatdata.h>

namespace Legacy
{

	/** An outpoint - a combination of a transaction hash and an index n into its vout */
	class OutPoint
	{
	public:

		/** The hash of the previous transaction being spent. **/
		uint512_t hash;


		/** The output number of the previous transaction being spent. **/
		uint32_t n;


		//the serizliation methods
		IMPLEMENT_SERIALIZE
		(
			READWRITE(FLATDATA(*this));
		)


		/** Constructor.
		 *
		 *	Set state to Null.
		 *
		 **/
		OutPoint() { SetNull(); }


		/** Constructor
		 *
		 *	@param[in] hashIn The input hash of previous transaction.
		 *	@param[in] nIn The output number of previous transaction.
		 *
		 **/
		OutPoint(uint512_t hashIn, uint32_t nIn) { hash = hashIn; n = nIn; }


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
		friend bool operator<(const OutPoint& a, const OutPoint& b)
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
		friend bool operator==(const OutPoint& a, const OutPoint& b)
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
		friend bool operator!=(const OutPoint& a, const OutPoint& b)
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

#endif
