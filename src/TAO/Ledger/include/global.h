/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2018

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To The Voice of The People

____________________________________________________________________________________________*/

#ifndef NEXUS_TAO_LEDGER_INCLUDE_GLOBAL_H
#define NEXUS_TAO_LEDGER_INCLUDE_GLOBAL_H

#include <LLC/types/bignum.h>

namespace TAO::Ledger
{

	/** Very first block hash in the blockchain. **/
	extern const uint1024_t hashGenesisMainnet;


	/** Hash to start the Testnet Blockchain. **/
	extern const uint1024_t hashGenesisTestnet;


	/** Minimum channels difficulty. **/
	extern const LLC::CBigNum bnProofOfWorkLimit[];


	/** Starting channels difficulty. **/
	extern const LLC::CBigNum bnProofOfWorkStart[];


	/** Minimum prime zero bits (1016-bits). **/
	extern const LLC::CBigNum bnPrimeMinOrigins;

}


#endif
