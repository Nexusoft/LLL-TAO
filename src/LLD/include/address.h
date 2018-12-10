/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_LLD_INCLUDE_ADDRESS_H
#define NEXUS_LLD_INCLUDE_ADDRESS_H

#include <LLD/include/version.h>
#include <LLD/templates/sector.h>

#include <LLD/cache/binary_lru.h>
#include <LLD/keychain/filemap.h>

#include <LLP/include/network.h>

namespace LLD
{
    class AddressDB : public SectorDatabase<BinaryFileMap, BinaryLRU>
    {
    public:
        /** The Database Constructor. To determine file location and the Bytes per Record. **/
        AddressDB(const char* pszMode="r+")
        : SectorDatabase("addr", pszMode) { }

        bool WriteAddress(uint32_t key, LLP::Address addr)
        {
            return Write(std::make_pair(std::string("address"), key), addr);
        }

        bool ReadAddress(uint32_t key, LLP::Address &addr)
        {
            return Read(std::make_pair(std::string("address"), key), addr);
        }

        bool WriteInfo(uint32_t key, LLP::AddressInfo info)
        {
            return Write(std::make_pair(std::string("info"), key), info);
        }

        bool ReadInfo(uint32_t key, LLP::AddressInfo &info)
        {
            return Read(std::make_pair(std::string("address"), key), info);
        }
    };
}

#endif
