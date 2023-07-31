/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2021

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LEGACY_TYPES_TXIN_H
#define NEXUS_LEGACY_TYPES_TXIN_H

#include <Legacy/types/outpoint.h>
#include <Legacy/types/script.h>

#include <Util/templates/serialize.h>

#include <climits>
#include <cstdint>


namespace Legacy
{

	/** An input of a transaction.  It contains the location of the previous
	 * transaction's output that it claims and a signature that matches the
	 * output's public key.
	 */
	class TxIn
	{
	public:

		/** The outpoint that contains the transaction previous output. **/
		OutPoint prevout;


		/** The Script that is inputed into this transaction. **/
		Script scriptSig; //need to forward declare


		/** The sequence number (to be deprecated). **/
		uint32_t nSequence;


		//for serialization macros
		IMPLEMENT_SERIALIZE
		(
			READWRITE(prevout);
			READWRITE(scriptSig);
			READWRITE(nSequence);
		)


		/** Default Constructor. **/
		TxIn();


		/** Copy Constructor. **/
		TxIn(const TxIn& in);


		/** Move Constructor. **/
		TxIn(TxIn&& in) noexcept;


		/** Copy assignment. **/
		TxIn& operator=(const TxIn& in);


		/** Move assignment. **/
		TxIn& operator=(TxIn&& in) noexcept;


		/** Default destructor. **/
		~TxIn();


		/** Constructor
		 *
		 *	@param[in] prevoutIn The previous output object (hash, n)
		 *	@param[in] scriptSigIn The script object to validate spend
		 *	@param[in] nSequenceIn The sequence number (default uint32_t numeric limits)
		 *
		 **/
		explicit TxIn(const OutPoint& prevoutIn, Script scriptSigIn=Script(), uint32_t nSequenceIn=std::numeric_limits<uint32_t>::max());


		/** Constructor
		 *
		 *	@param[in] hashPrevTx The previous transaction hash input is spending
		 *	@param[in] nOut The output number of previous transaction being spent
		 * 	@param[in] scriptSigIn The script object to validate spend
		 *	@param[in] nSequenceIn The sequence number (default uint32_t numeric limits)
		 *
		 **/
		TxIn(const uint512_t& hashPrevTx, const uint32_t nOut, Script scriptSigIn=Script(), uint32_t nSequenceIn=std::numeric_limits<uint32_t>::max());


		/** Comparison Operator overload
		 *
		 *	Compares two TxIn objects to one another
		 *
		 *	@return true if the objects are equal
		 *
		 **/
		friend bool operator==(const TxIn& a, const TxIn& b)
		{
			return (a.prevout   == b.prevout &&
					a.scriptSig == b.scriptSig &&
					a.nSequence == b.nSequence);
		}


		/** Not Operator overload
		 *
		 *	Compares two TxIn objects to one another
		 *
		 *	@return true if the objects are not equal
		 *
		 **/
		friend bool operator!=(const TxIn& a, const TxIn& b)
		{
			return !(a == b);
		}


		/** IsFinal
		 *
		 *	Flag to determine if the object sequence is in final state
		 *
		 *	@return true if the transaction is final
		 *
		 **/
		bool IsFinal() const
		{
			return (nSequence == std::numeric_limits<uint32_t>::max());
		}


		/** IsStakeSig
		 *
		 *	Flag to tell if this input is the flag for proof of stake Transactions
		 *
		 *	@return true if the scriptSig is fibonacci sequence
		 *
		 **/
		bool IsStakeSig() const;


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
