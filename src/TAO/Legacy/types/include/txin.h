/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2017] ++

			(c) Copyright The Nexus Developers 2014 - 2017

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"fides in stellis, virtus in numeris" - Faith in the Stars, Power in Numbers

____________________________________________________________________________________________*/

#ifndef NEXUS_TAO_LEGACY_TYPES_INCLUDE_TRANSACTION_H
#define NEXUS_TAO_LEGACY_TYPES_INCLUDE_TRANSACTION_H


namespace LLD
{
	class CIndexDB;
}

namespace Legacy
{

	namespace Types
	{


		/** An input of a transaction.  It contains the location of the previous
		 * transaction's output that it claims and a signature that matches the
		 * output's public key.
		 */
		class CTxIn
		{
		public:

			/** The outpoint that contains the transaction previous output. **/
			COutPoint prevout;


			/** The Script that is inputed into this transaction. **/
			Wallet::CScript scriptSig; //need to forward declare


			/** The sequence number (to be deprecated). **/
			unsigned int nSequence;


			//for serizliation macros
			SERIALIZE_HEADER


			/** Default Constructor
			 *
			 *	Sets the sequence to numeric limits of 32 bit unsigned int
			 *
			 **/
			CTxIn()
			{
				nSequence = std::numeric_limits<unsigned int>::max();
			}


			/** Explicit Constructor
			 *
			 *	@param[in] prevoutIn The previous output object (hash, n)
			 *	@param[in] scriptSigIn The script object to validate spend
			 *	@param[in] nSequenceIn The sequence number (default unsigned int numeric limits)
			 *
			 **/
			explicit CTxIn(COutPoint prevoutIn, Wallet::CScript scriptSigIn=CScript(), unsigned int nSequenceIn=std::numeric_limits<unsigned int>::max())
			{
				prevout = prevoutIn;
				scriptSig = scriptSigIn;
				nSequence = nSequenceIn;
			}


			/** Explicit Constructor
			 *
			 *	@param[in] hashPrevTx The previous transaction hash input is spending
			 *	@param[in] nOut The output number of previous transaction being spent
			 * 	@param[in] scriptSigIn The script object to validate spend
			 *	@param[in] nSequenceIn The sequence number (default unsigned int numeric limits)
			 *
			 **/
			CTxIn(uint512 hashPrevTx, unsigned int nOut, Wallet::CScript scriptSigIn=CScript(), unsigned int nSequenceIn=std::numeric_limits<unsigned int>::max())
			{
				prevout = COutPoint(hashPrevTx, nOut);
				scriptSig = scriptSigIn;
				nSequence = nSequenceIn;
			}

			/** Comparison Operator overload
			 *
			 *	Compares two CTxIn objects to one another
			 *
			 *	@return true if the objects are equal
			 *
			 **/
			friend bool operator==(const CTxIn& a, const CTxIn& b)
			{
				return (a.prevout   == b.prevout &&
						a.scriptSig == b.scriptSig &&
						a.nSequence == b.nSequence);
			}

			/** Not Operator overload
			 *
			 *	Compares two CTxIn objects to one another
			 *
			 *	@return true if the objects are not equal
			 *
			 **/
			friend bool operator!=(const CTxIn& a, const CTxIn& b)
			{
				return !(a == b);
			}

			/** IsFinal
			 *
			 *	Flag to determin if the object sequence is in final state
			 *
			 *	@return true if the transaction is final
			 *
			 **/
			bool IsFinal() const
			{
				return (nSequence == std::numeric_limits<unsigned int>::max());
			}


			/** IsStakeSig
			 *
			 *	Flag to tell if this input is the flag for proof of stake Transactions
			 *
			 *	@return true if the scriptSig is fibanacci sequence
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

}
