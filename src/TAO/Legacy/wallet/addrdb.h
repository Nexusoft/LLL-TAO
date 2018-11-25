/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++

			(c) Copyright The Nexus Developers 2014 - 2018

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_LEGACY_WALLET_ADDRDB_H
#define NEXUS_LEGACY_WALLET_ADDRDB_H

#include <string>

#include <TAO/Legacy/wallet/db.h>

namespace Legacy 
{

    /* forward declaration */    
    namespace Net
    {
        class CAddrMan;
    }

    namespace Wallet
    {
       /** @class CAddrDB
        * 
        *  Access to the (IP) address database (addr.dat) 
        **/
        class CAddrDB : public CDB
        {
        public:
            /** Defines name of address database file **/
            static const std::string ADDRESS_DB("addr.dat");


            /** Constructor
             *
             *  Initializes database access to address database for an access mode (see CDB for modes).
             *
             *  @param[in] pszMode A string containing one or more access mode characters
             *                     defaults to r+ (read and append). An empty or null string is
             *                     equivalent to read only.
             *
             **/
            CAddrDB(const char* pszMode="r+") : CDB(CAddrDB::ADDRESS_DB, pszMode) { }


        private:
            /** Copy constructor deleted. No copy allowed **/
            CAddrDB(const CAddrDB&) = delete;
 

            /** Copy assignment operator deleted. No assignment allowed **/
            void operator=(const CAddrDB&) = delete;


        public:
            /** WriteAddrman
             *
             *  Writes collection of addresses from CAddrMan to the address database
             *
             *  @param[in] addr The addresses to write
             *
             *  @return true if addresses written successfully
             *
             **/
            bool WriteAddrman(const Legacy::Net::CAddrMan& addr);


            /** LoadAddresses
             *
             *  Retrieves addresses from address database and loades them into Legacy::Net::addrman.
             *
             *  @return true if addresses loaded successfully
             *
             **/
            bool LoadAddresses();
        };


        /** LoadAddresses
         *
         *  A general function to load Legacy::Net::addrman
         *  Creates an instance of CAddrDB and uses it to load addresses.
         *
         **/
        bool LoadAddresses();

    }
}

#endif 
