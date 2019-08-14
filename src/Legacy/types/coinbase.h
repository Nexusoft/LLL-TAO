/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2019

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once

#include <map>
#include <cstdint>

namespace Legacy
{

	/** Coinbase
     *
     *  Class to encapsulate multiple coinbase recipients, such as for pool payouts.
     *
     **/
	class Coinbase
	{
	public:

        /** The Transaction Outputs to be Serialized to Mining LLP. **/
        std::map<std::string, uint64_t> vOutputs;


        /** The value of the current coinbase payout. **/
        uint64_t nMaxValue;


        /** The pool fee issued by the pool server. */
        uint64_t nPoolFee;


        /** Default constructor **/
        Coinbase();


        /** Constructor. **/
        Coinbase(const std::map<std::string, uint64_t>& vTxOutputs, uint64_t nValue, uint64_t nLocalFee);


        /** Copy constructor. **/
        Coinbase(const Coinbase& rhs);


        /** Assignment operator. **/
        Coinbase &operator=(const Coinbase& rhs);


		/** Destructor **/
		~Coinbase();


        /** SetNull
         *
         *  Set the coinbase to null state.
         *
         **/
        void SetNull();


        /** IsNull
		 *
		 *	Checks the objects null state.
		 *
		 **/
		bool IsNull() const;


        /** IsValid
         *
         *  Determines if the coinbase tx has been built successfully.
         *
         *  @return true if the coinbase tx is valid, otherwise false.
         *
         **/
        bool IsValid() const;


        /** Print
         *
         *  Writes the coinbase tx data to the log.
         *
         **/
        void Print() const;
	};
}
