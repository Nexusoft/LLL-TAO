/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2019

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <Legacy/types/outpoint.h>
#include <Legacy/types/script.h>
#include <Legacy/types/address.h>
#include <Legacy/types/txin.h>

#include <Util/include/hex.h>

namespace Legacy
{

	/* Basic Constructor. */
	CTxIn::CTxIn(uint512_t hashPrevTx, uint32_t nOut, CScript scriptSigIn, uint32_t nSequenceIn)
	{
		prevout = COutPoint(hashPrevTx, nOut);
		scriptSig = scriptSigIn;
		nSequence = nSequenceIn;
	}


	/* Flag to tell if this input is the flag for proof of stake Transactions */
	bool CTxIn::IsStakeSig() const
	{
		if( scriptSig.size() < 8)
			return false;

		if( scriptSig[0] != 1 || scriptSig[1] != 2 || scriptSig[2] != 3 || scriptSig[3] != 5 ||
			scriptSig[4] != 8 || scriptSig[5] != 13 || scriptSig[6] != 21 || scriptSig[7] != 34)
			return false;

		return true;
	}


	/* Short Hand debug output of the object (hash, n) */
	std::string CTxIn::ToStringShort() const
    {
        return debug::strprintf(" %s %d", prevout.hash.ToString().c_str(), prevout.n);
    }


	/* Full object debug output as std::string. */
    std::string CTxIn::ToString() const
    {
        std::string str;
        str += "CTxIn(";
        str += prevout.ToString();
        if (prevout.IsNull())
        {
            if(IsStakeSig())
                str += debug::strprintf(", genesis %s", HexStr(scriptSig).c_str());
            else
                str += debug::strprintf(", coinbase %s", HexStr(scriptSig).c_str());
        }
        else if(IsStakeSig())
            str += debug::strprintf(", trust %s", HexStr(scriptSig).c_str());
        else
            str += debug::strprintf(", scriptSig=%s", scriptSig.ToString().substr(0,24).c_str());
        if (nSequence != std::numeric_limits<uint32_t>::max())
            str += debug::strprintf(", nSequence=%u", nSequence);
        str += ")";
        return str;
    }


	/* Dump the full object to the console (stdout) */
    void CTxIn::print() const
    {
        debug::log(0, ToString());
    }
}
