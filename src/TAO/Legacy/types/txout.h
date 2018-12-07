/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2018

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_TAO_LEGACY_TYPES_TXOUT_H
#define NEXUS_TAO_LEGACY_TYPES_TXOUT_H

#include <cstdint>
#include <cstring>

#include <Util/templates/serialize.h>
#include <TAO/Legacy/types/script.h>

namespace Legacy
{

    /** An output of a transaction.  It contains the public key that the next input
	 * must be able to sign with to claim it.
	 */
	class CTxOut
	{
	public:

		/** The amount of NXS being transferred into output. **/
		int64_t nValue;


		/** The output script required to evaluate to true to be spent. **/
		CScript scriptPubKey;


		//the serialization methods
		IMPLEMENT_SERIALIZE
		(
			READWRITE(nValue);
			READWRITE(scriptPubKey);
		)


		/** Default constructor
		 *
		 *	Sets object to null state.
		 *
		 **/
		CTxOut()
		{
			SetNull();
		}


		/** Constructor
		 *
		 *	@param[in] nValueIn The value to be transferred in output.
		 *	@param[in] scriptPubKeyIn The script to be evaluated on spend.
		 *
		 **/
		CTxOut(int64_t nValueIn, CScript scriptPubKeyIn)
		{
			nValue = nValueIn;
			scriptPubKey = scriptPubKeyIn;
		}


		/** SetNull
		 *
		 *	Set the object to null state.
		 *
		 **/
		void SetNull();


		/** IsNull
		 *
		 *	Determine if the object is in a null state.
		 *
		 **/
		bool IsNull();


		/** SetEmpty
		 *
		 *	Clear the object and reset value to 0.
		 *
		 **/
		void SetEmpty();


		/** IsEmpty
		 *
		 *	Determine if the object is in an empty state.
		 *
		 **/
		bool IsEmpty() const;


		/** GetHash
		 *
		 *	Get the hash of the object.
		 *
		 **/
		uint512_t GetHash() const;


		/** Equals operator overload
		 *
		 *	@param[in] a The first object to compare
		 *	@param[in] b The second object to compare
		 *
		 *	@return true if the objects are equivilent.
		 *
		 **/
		friend bool operator==(const CTxOut& a, const CTxOut& b)
		{
			return (a.nValue       == b.nValue &&
					a.scriptPubKey == b.scriptPubKey);
		}


		/** Not Equals operator overload
		 *
		 *	@param[in] a The first object to compare
		 *	@param[in] b The second object to compare
		 *
		 *	@return true if the objects are not equivilent.
		 *
		 **/
		friend bool operator!=(const CTxOut& a, const CTxOut& b)
		{
			return !(a == b);
		}


		/** ToStringShort
		 *
		 *	Short Hand debug output of the object (hash, n)
		 *
		 *	@return string containing the shorthand output
		 *
		 **/
		std::string ToStringShort() const;


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
