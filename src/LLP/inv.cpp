/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2019

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLP/include/inv.h>
#include <Util/include/debug.h>
#include <Util/include/runtime.h>

namespace LLP
{

		/* table of differnt inventory type names */
		static const char* ppszTypeName[] =
		{
			  "ERROR",
			  "legacy tx",
			  "legacy block",
			  "tritium tx",
			  "tritium block",
		};

		static const uint32_t len = ARRAYLEN(ppszTypeName);


		/* Default Constructor */
		CInv::CInv()
		: hash(0)
		, type(0)
		{
		}


		/* Constructor */
		CInv::CInv(const uint1024_t& hashIn, const int32_t typeIn)
		: hash(hashIn)
		, type(typeIn)
		{
		}


		/* Constructor */
		CInv::CInv(const std::string& strType, const uint1024_t& hashIn)
		: hash(hashIn)
		{
				uint32_t i = 1;
				for (; i < len; ++i)
				{
					  if (strType == ppszTypeName[i])
					  {
						    type = i;
					    	break;
					  }
				}
				if (i == len)
				{
					  throw std::out_of_range(debug::safe_printstr(
						    "CInv::CInv(string, uint1024_t) : unknown type '", strType, "'"));
				}
		}


		/* Relational operator less than */
		bool operator<(const CInv& a, const CInv& b)
		{
			return (a.type < b.type || (a.type == b.type && a.hash < b.hash));
		}


		/* Determines if this inventory a known type. */
		bool CInv::IsKnownType() const
		{
			return (type >= 1 && type < static_cast<int32_t>(len));
		}


		/* Returns a command from this inventory object. */
		const char* CInv::GetCommand() const
		{
		  	if (!IsKnownType())
				  throw std::out_of_range(
			    		debug::safe_printstr("CInv::GetCommand() : type=", type, " unknown type"));

			  return ppszTypeName[type];
		}


		/* Returns data about this inventory as a string object. */
		std::string CInv::ToString() const
		{
				if(type == MSG_TX_LEGACY || type == MSG_TX_TRITIUM)
				{
						std::string invHash = hash.ToString();
						return debug::safe_printstr(GetCommand(), " ", invHash.substr(invHash.length() - 20, invHash.length()));
				}

				return debug::safe_printstr(GetCommand(), " ", hash.ToString().substr(0,20));
		}


		/* Prints this inventory data to the console window. */
		void CInv::Print() const
		{
				debug::log(0, "CInv(", ToString(), ")");
		}


		/* Returns the hash associated with this inventory. */
		uint1024_t CInv::GetHash() const
		{
				return hash;
		}


		/* Sets the hash for this inventory. */
		void CInv::SetHash(const uint1024_t &hashIn)
		{
				hash = hashIn;
		}


		/* Returns the type of this inventory. */
		int32_t CInv::GetType() const
		{
				return type;
		}


		/* Sets the type for this inventory. */
		void CInv::SetType(const int32_t typeIn)
		{
			type = typeIn;
		}
}
