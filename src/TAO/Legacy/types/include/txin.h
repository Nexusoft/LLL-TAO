/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++

			(c) Copyright The Nexus Developers 2014 - 2018

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_TAO_LEGACY_TYPES_INCLUDE_TXIN_H
#define NEXUS_TAO_LEGACY_TYPES_INCLUDE_TXIN_H


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
			uint32_t nSequence;


			//for serizliation macros
			SERIALIZE_HEADER


			/** Default Constructor
			 *
			 *	Sets the sequence to numeric limits of 32 bit uint32_t
			 *
			 **/
			CTxIn()
			{
				nSequence = std::numeric_limits<uint32_t>::max();
			}


			/** Constructor
			 *
			 *	@param[in] prevoutIn The previous output object (hash, n)
			 *	@param[in] scriptSigIn The script object to validate spend
			 *	@param[in] nSequenceIn The sequence number (default uint32_t numeric limits)
			 *
			 **/
			explicit CTxIn(COutPoint prevoutIn, Wallet::CScript scriptSigIn=CScript(), uint32_t nSequenceIn=std::numeric_limits<uint32_t>::max())
			{
				prevout = prevoutIn;
				scriptSig = scriptSigIn;
				nSequence = nSequenceIn;
			}


			/** Constructor
			 *
			 *	@param[in] hashPrevTx The previous transaction hash input is spending
			 *	@param[in] nOut The output number of previous transaction being spent
			 * 	@param[in] scriptSigIn The script object to validate spend
			 *	@param[in] nSequenceIn The sequence number (default uint32_t numeric limits)
			 *
			 **/
			CTxIn(uint512_t hashPrevTx, uint32_t nOut, Wallet::CScript scriptSigIn=CScript(), uint32_t nSequenceIn=std::numeric_limits<uint32_t>::max());

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
				return (nSequence == std::numeric_limits<uint32_t>::max());
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
