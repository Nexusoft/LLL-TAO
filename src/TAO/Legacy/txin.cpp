/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2018

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include "include/outpoint.h"

namespace Legacy
{

	namespace Types
	{

		/* Serizliation Method Body. */
		SERIALIZE_SOURCE
		(
			CTxIn,

			READWRITE(prevout);
			READWRITE(scriptSig);
			READWRITE(nSequence);
		)


		/* Basic Constructor. */
		CTxIn::CTxIn(uint512_t hashPrevTx, uint32_t nOut, Wallet::CScript scriptSigIn=CScript(), uint32_t nSequenceIn=std::numeric_limits<uint32_t>::max())
		{
			prevout = COutPoint(hashPrevTx, nOut);
			scriptSig = scriptSigIn;
			nSequence = nSequenceIn;
		}


		/* Flag to tell if this input is the flag for proof of stake Transactions */
		bool CTxIn::IsStakeSig() const
		{
			if( scriptSig.size() != 8)
				return false;

			if( scriptSig[0] != 1 || scriptSig[1] != 2 || scriptSig[2] != 3 || scriptSig[3] != 5 ||
				scriptSig[4] != 8 || scriptSig[5] != 13 || scriptSig[6] != 21 || scriptSig[7] != 34)
				return false;

			return true;
		}


		/* Short Hand debug output of the object (hash, n) */
		std::string CTxIn::ToStringShort() const
        {
            return strprintf(" %s %d", prevout.hash.ToString().c_str(), prevout.n);
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
                    str += strprintf(", genesis %s", HexStr(scriptSig).c_str());
                else
                    str += strprintf(", coinbase %s", HexStr(scriptSig).c_str());
            }
            else if(IsStakeSig())
                str += strprintf(", trust %s", HexStr(scriptSig).c_str());
            else
                str += strprintf(", scriptSig=%s", scriptSig.ToString().substr(0,24).c_str());
            if (nSequence != std::numeric_limits<uint32_t>::max())
                str += strprintf(", nSequence=%u", nSequence);
            str += ")";
            return str;
        }


		/* Dump the full object to the console (stdout) */
        void CTxIn::print() const
        {
            debug::log(0, "%s\n", ToString().c_str());
        }
	}
}
