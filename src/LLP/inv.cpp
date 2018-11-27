/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++

			(c) Copyright The Nexus Developers 2014 - 2018

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLP/include/inv.h>
#include <Util/include/debug.h>
#include <Util/include/runtime.h>

namespace LLP
{

	static const char* ppszTypeName[] =
	{
		"ERROR",
		"tx",
		"block",
	};


	/* Check the Inventory to See if a given hash is found.
		TODO: Rebuild this Inventory System. Much Better Ways to Do it.
		*/

	CInv::CInv()
	{
		type = 0;
		hash = 0;
	}


	CInv::CInv(int typeIn, const uint1024_t& hashIn)
	{
		type = typeIn;
		hash = hashIn;
	}


	CInv::CInv(const std::string& strType, const uint1024_t& hashIn)
	{
		uint32_t i;
		for (i = 1; i < ARRAYLEN(ppszTypeName); i++)
		{
			if (strType == ppszTypeName[i])
			{
				type = i;
				break;
			}
		}
		if (i == ARRAYLEN(ppszTypeName))
			throw std::out_of_range(debug::strprintf("CInv::CInv(string, uint1024_t) : unknown type '%s'", strType.c_str()));
		hash = hashIn;
	}


	bool operator<(const CInv& a, const CInv& b)
	{
		return (a.type < b.type || (a.type == b.type && a.hash < b.hash));
	}


	bool CInv::IsKnownType() const
	{
		return (type >= 1 && type < (int)ARRAYLEN(ppszTypeName));
	}


	const char* CInv::GetCommand() const
	{
		if (!IsKnownType())
			throw std::out_of_range(debug::strprintf("CInv::GetCommand() : type=%d unknown type", type));

		return ppszTypeName[type];
	}


	std::string CInv::ToString() const
	{
		if(GetCommand() == (const char*)"tx")
		{
			std::string invHash = hash.ToString();
			return debug::strprintf("tx %s", invHash.substr(invHash.length() - 20, invHash.length()).c_str());
		}

		return debug::strprintf("%s %s", GetCommand(), hash.ToString().substr(0,20).c_str());
	}


	void CInv::print() const
	{
		debug::log("CInv(%s)\n", ToString().c_str());
	}


}
