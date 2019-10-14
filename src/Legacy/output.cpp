/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2019

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <Legacy/types/output.h>
#include <Legacy/types/wallettx.h>

#include <Util/include/debug.h>
#include <Util/include/string.h> /* for FormatMoney() */

namespace Legacy
{

    /* Copy Constructor. */
    Output::Output(const Output& out)
    : walletTx (out.walletTx)
    , i        (out.i)
    , nDepth   (out.nDepth)
    {
    }


    /* Move Constructor. */
    Output::Output(Output&& out) noexcept
    : walletTx (out.walletTx)
    , i        (std::move(out.i))
    , nDepth   (std::move(out.nDepth))
    {
    }


    /* Initializes this Output with the provided parameter values */
    Output::Output(const WalletTx& walletTxIn, const uint32_t iIn, const uint32_t nDepthIn)
    : walletTx (walletTxIn)
    , i        (iIn)
    , nDepth   (nDepthIn)
    {
    }


    /* Destructor */
    Output::~Output()
    {
    }


    /* Generate a string representation of this output. */
    std::string Output::ToString() const
    {
        return debug::safe_printstr("Output(", walletTx.GetHash().SubString(10), ", ", i, ", ", nDepth, ") [", FormatMoney(walletTx.vout[i].nValue), "]");
    }


    /* Print a string representation of this output */
    void Output::print() const
    {
        debug::log(0, ToString());
    }
}
