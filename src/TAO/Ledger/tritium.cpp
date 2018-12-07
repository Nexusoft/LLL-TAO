/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2018

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/Ledger/types/tritium.h>

#include <Util/include/hex.h>

namespace TAO::Ledger
{


	/* For debugging Purposes seeing block state data dump */
	std::string TritiumBlock::ToString() const
    {
        return debug::strprintf(ANSI_COLOR_BRIGHT_WHITE "Block" ANSI_COLOR_RESET "(" ANSI_COLOR_BRIGHT_WHITE "nVersion" ANSI_COLOR_RESET " = %u, " ANSI_COLOR_BRIGHT_WHITE "hashPrevBlock" ANSI_COLOR_RESET " = %s, " ANSI_COLOR_BRIGHT_WHITE "hashMerkleRoot" ANSI_COLOR_RESET " = %s, " ANSI_COLOR_BRIGHT_WHITE "nChannel" ANSI_COLOR_RESET " = %u, " ANSI_COLOR_BRIGHT_WHITE "nHeight" ANSI_COLOR_RESET " = %u, " ANSI_COLOR_BRIGHT_WHITE "nBits" ANSI_COLOR_RESET " = %u, " ANSI_COLOR_BRIGHT_WHITE "nNonce" ANSI_COLOR_RESET " = %" PRIu64 ", " ANSI_COLOR_BRIGHT_WHITE "nTime" ANSI_COLOR_RESET " = %u, " ANSI_COLOR_BRIGHT_WHITE "vchBlockSig" ANSI_COLOR_RESET " = %s, " ANSI_COLOR_BRIGHT_WHITE "vtx.size()" ANSI_COLOR_RESET " = %u)",
        nVersion, hashPrevBlock.ToString().substr(0, 20).c_str(), hashMerkleRoot.ToString().substr(0, 20).c_str(), nChannel, nBits, nNonce, nTime, HexStr(vchBlockSig.begin(), vchBlockSig.end()).c_str(), vtx.size());
    }

	/* For debugging purposes, printing the block to stdout */
	void TritiumBlock::print() const
    {
        debug::log(0, ToString().c_str());
    }
}
