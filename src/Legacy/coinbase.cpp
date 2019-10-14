/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2019

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <Util/include/debug.h>
#include <Legacy/types/coinbase.h>

namespace Legacy
{

    /* Default constructor */
    Coinbase::Coinbase()
    : vOutputs()
	, nMaxValue(0)
	, nWalletFee(0)
	{
	}


    /* Copy constructor. */
    Coinbase::Coinbase(const Coinbase& coinbase)
    : vOutputs   (coinbase.vOutputs)
    , nMaxValue  (coinbase.nMaxValue)
    , nWalletFee (coinbase.nWalletFee)
	{
	}


    /** Move constructor. **/
    Coinbase::Coinbase(Coinbase&& coinbase) noexcept
    : vOutputs   (std::move(coinbase.vOutputs))
    , nMaxValue  (std::move(coinbase.nMaxValue))
    , nWalletFee (std::move(coinbase.nWalletFee))
    {
    }


    /* Copy assignment operator. */
    Coinbase& Coinbase::operator=(const Coinbase& coinbase)
	{
        vOutputs   = coinbase.vOutputs;
        nMaxValue  = coinbase.nMaxValue;
        nWalletFee = coinbase.nWalletFee;

        return *this;
	}


    /** Move assignment. **/
    Coinbase& Coinbase::operator=(Coinbase&& coinbase) noexcept
    {
        vOutputs   = std::move(coinbase.vOutputs);
        nMaxValue  = std::move(coinbase.nMaxValue);
        nWalletFee = std::move(coinbase.nWalletFee);

        return *this;
    }


	/* Destructor */
	Coinbase::~Coinbase()
	{
	}


    /* Constructor. */
    Coinbase::Coinbase(const std::map<std::string, uint64_t>& vTxOutputs, const uint64_t nValue, const uint64_t nLocalFee)
    : vOutputs   (vTxOutputs)
    , nMaxValue  (nValue)
    , nWalletFee (nLocalFee)
    {
    }


    /** SetNull
     *
     *  Set the coinbase to null state.
     *
     **/
    void Coinbase::SetNull()
    {
        vOutputs.clear();
        nMaxValue = 0;
        nWalletFee = 0;
    }


    /** IsNull
	 *
	 *	Checks the objects null state.
	 *
	 **/
	bool Coinbase::IsNull() const
	{
		return vOutputs.empty() && nWalletFee == 0;
	}


    /** IsValid
     *
     *  Determines if the coinbase tx has been built successfully.
     *
     *  @return true if the coinbase tx is valid, otherwise false.
     *
     **/
    bool Coinbase::IsValid() const
    {
        uint64_t nCurrentValue = nWalletFee;
        for(const auto& entry : vOutputs)
            nCurrentValue += entry.second;

        return nCurrentValue == nMaxValue;
    }


    /** Print
     *
     *  Writes the coinbase tx data to the log.
     *
     **/
    void Coinbase::Print() const
    {
        debug::log(0, "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++");
        uint64_t nTotal = 0;
        for(const auto& entry : vOutputs)
        {
            debug::log(0, entry.first, ":", static_cast<double>(entry.second) / 1000000.0);
            nTotal += entry.second;
        }

        debug::log(0, "Total Value of Coinbase = ", static_cast<double>(nTotal) / 1000000.0);
        debug::log(0, "Set Value of Coinbase = ", static_cast<double>(nMaxValue) / 1000000.0);
        debug::log(0, "WalletFee in Coinbase ", static_cast<double>(nWalletFee) / 1000000.0);
        debug::log(0, "Is Complete: ", IsValid() ? "TRUE" : "FALSE");
        debug::log(0, "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++");
    }


    /* Returns the reward payed out to wallet operator. */
    uint64_t Coinbase::WalletReward() const
    {
        return nWalletFee;
    }


    /* Returns a copy of the outputs used for this coinbase. */
    std::map<std::string, uint64_t> Coinbase::Outputs() const
    {
        return vOutputs;
    }

}
