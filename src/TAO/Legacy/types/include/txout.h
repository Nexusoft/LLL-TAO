/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2017] ++

			(c) Copyright The Nexus Developers 2014 - 2017

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"fides in stellis, virtus in numeris" - Faith in the Stars, Power in Numbers

____________________________________________________________________________________________*/

#ifndef NEXUS_TAO_LEGACY_TYPES_INCLUDE_TXIN_H
#define NEXUS_TAO_LEGACY_TYPES_INCLUDE_TXIN_H

#include "../../../Util/templates/serialize"

namespace Legacy
{
	namespace Types
	{
		class CScript;

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
			SERIALIZE_HEADER


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
    		LLC::uint512 GetHash() const;


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

}
