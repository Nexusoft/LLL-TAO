/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2019

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LEGACY_TYPES_TXOUT_H
#define NEXUS_LEGACY_TYPES_TXOUT_H

#include <Util/templates/serialize.h>
#include <Legacy/types/script.h>

#include <cstdint>
#include <cstring>


namespace Legacy
{

    /** An output of a transaction.  It contains the public key that the next input
	 * must be able to sign with to claim it.
	 */
	class TxOut
	{
	public:

		/** The amount of NXS being transferred into output. **/
		int64_t nValue;


		/** The output script required to evaluate to true to be spent. **/
		Script scriptPubKey;


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
		TxOut()
        : nValue(-1)
        , scriptPubKey()
		{
		}


		/** Constructor
		 *
		 *	@param[in] nValueIn The value to be transferred in output.
		 *	@param[in] scriptPubKeyIn The script to be evaluated on spend.
		 *
		 **/
		TxOut(int64_t nValueIn, Script scriptPubKeyIn)
        : nValue(nValueIn)
        , scriptPubKey(scriptPubKeyIn)
		{
		}


		/** Destructor **/
		~TxOut()
		{
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
		bool IsNull() const;


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
		friend bool operator==(const TxOut& a, const TxOut& b)
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
		friend bool operator!=(const TxOut& a, const TxOut& b)
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
