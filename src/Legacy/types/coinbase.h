/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2019

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once

#include <map>
#include <LLC/types/uint1024.h>
#include <Util/include/debug.h>

namespace Legacy
{

	/* Class to encapsulate multiple coinbase recipients, such as for pool payouts*/
	class Coinbase
	{
	public:

        /* Default constructor*/
        Coinbase()
        : vOutputs()
				, nMaxValue(0)
				, nPoolFee(0)
				{
				}

        /** Constructor when set from incoming data **/
        Coinbase(std::map<std::string, uint64_t> vTxOutputs, uint64_t nValue, uint64_t nLocalFee)
        : vOutputs(vTxOutputs)
				, nMaxValue(nValue)
				, nPoolFee(nLocalFee)
				{
				}


				/** Destructor **/
				~Coinbase()
				{
				}


        /** The Transaction Outputs to be Serialized to Mining LLP. **/
        std::map<std::string, uint64_t> vOutputs;


        /** The Value of this current Coinbase Payout. **/
        uint64_t nMaxValue, nPoolFee;


        /** SetNull
         *
         *  Set the coinbase to Null state.
         *
         **/
        void SetNull()
        {
            vOutputs.clear();
            nMaxValue = 0;
            nPoolFee = 0;
        }


        /** Is Null
				 *
				 *	Checks the objects null state.
				 *
				 **/
				bool IsNull() const
				{
						return vOutputs.empty() && nPoolFee == 0;
				}


        /** IsValid
        *
        *  Determines if the Coinbase Tx has been built Successfully
        *
        *  @return true if the Coinbase Tx is valid, otherwise false.
        *
        **/
        bool IsValid() const
        {
            uint64_t nCurrentValue = nPoolFee;
            for(const auto& entry : vOutputs)
                nCurrentValue += entry.second;

            return nCurrentValue == nMaxValue;
        }

        /** Print
        *
        *  Writes the coinbase tx data to the log
        *
        **/
        /** Output the Transactions in the Coinbase Container. **/
        void Print() const
        {
            debug::log(0, "\n\n +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ \n");
            uint64_t nTotal = 0;
            for(const auto& entry : vOutputs)
            {
                debug::log(0, entry.first, ":", static_cast<double>(entry.second) / 1000000.0);
                nTotal += entry.second;
            }

            debug::log(0, "Total Value of Coinbase = ", static_cast<double>(nTotal) / 1000000.0);
            debug::log(0, "Set Value of Coinbase = ", static_cast<double>(nMaxValue) / 1000000.0);
            debug::log(0, "PoolFee in Coinbase ", static_cast<double>(nPoolFee) / 1000000.0);
            debug::log(0, "\n\nIs Complete: ", IsValid() ? "TRUE" : "FALSE");
            debug::log(0, "\n\n +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ \n");
        }

	};
}
