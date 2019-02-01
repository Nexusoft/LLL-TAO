/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

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

#include <LLP/include/trustaddress.h>

namespace LLD
{

    /** AddressDB
     *
     *  The database class for peer addresses to determine trust relationships.
     *
     **/
    class AddressDB : public SectorDatabase<BinaryFileMap, BinaryLRU>
    {
    public:


        /** The Database Constructor. To determine file location and the Bytes per Record. **/
        AddressDB(uint16_t port, uint8_t nFlags = FLAGS::CREATE | FLAGS::FORCE)
        : SectorDatabase("addr/" + std::to_string(port), nFlags) { }


        /** WriteTrustAddress
         *
         *  Writes the Trust Address to the database.
         *
         *  @param[in] key The key for the data location.
         *  @param[in] addr The TrustAddress object to serialize to disk.
         *
         *  @return True if the write is successful, false otherwise.
         *
         **/
        bool WriteTrustAddress(uint64_t key, const LLP::TrustAddress &addr)
        {
            return Write(std::make_pair(std::string("addr"), key), addr);
        }


        /** ReadTrustAddress
         *
         *  Reads the Trust Address from the database.
         *
         *  @param[in] key The key for the data location.
         *  @param[in] addr The TrustAddress object to deserialize from disk.
         *
         *  @return True if the read is successful, false otherwise.
         *
         **/
        bool ReadTrustAddress(uint64_t key, LLP::TrustAddress &addr)
        {
            return Read(std::make_pair(std::string("addr"), key), addr);
        }


        /** WriteThisAddress
         *
         *  Writes this address to the database.
         *
         *  @param[in] key The key for the data location.
         *  @param[in] this The TrustAddress object to serialize to disk.
         *
         *  @return True if the write is successful, false otherwise.
         *
         **/
        bool WriteThisAddress(uint64_t key, const LLP::BaseAddress &this_addr)
        {
            return Write(std::make_pair(std::string("this"), key), this_addr);
        }


        /** ReadThisAddress
         *
         *  Reads the this address from the database.
         *
         *  @param[in] key The key for the data location.
         *  @param[in] this_addr The TrustAddress object to deserialize from disk.
         *
         *  @return True if the read is successful, false otherwise.
         *
         **/
        bool ReadTrustAddress(uint64_t key, LLP::BaseAddress &this_addr)
        {
            return Read(std::make_pair(std::string("this"), key), this_addr);
        }
    };
}

#endif
