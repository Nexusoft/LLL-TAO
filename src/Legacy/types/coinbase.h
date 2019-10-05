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
	private:

        /** The Transaction Outputs to be Serialized to Mining LLP. **/
        std::map<std::string, uint64_t> vOutputs;


        /** The value of the current coinbase payout. **/
        uint64_t nMaxValue;


        /** The amount that will be sent to wallet operator. */
        uint64_t nWalletFee;

    public:

        /** Default constructor **/
        Coinbase();


        /** Copy constructor. **/
        Coinbase(const Coinbase& coinbase);


		/** Move constructor. **/
		Coinbase(Coinbase&& coinbase) noexcept;


        /** Copy assignment. **/
        Coinbase& operator=(const Coinbase& coinbase);


		/** Move assignment. **/
		Coinbase& operator=(Coinbase&& coinbase) noexcept;


		/** Destructor **/
		~Coinbase();


		/** Constructor. **/
		Coinbase(const std::map<std::string, uint64_t>& vTxOutputs, const uint64_t nValue, const uint64_t nLocalFee);


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


        /** WalletReward
         *
         *  Returns the reward payed out to wallet operator.
         *
         **/
        uint64_t WalletReward() const;


        /** Outputs
         *
         *  Returns a copy of the outputs used for this coinbase.
         *
         **/
        std::map<std::string, uint64_t> Outputs() const;

	};
}
