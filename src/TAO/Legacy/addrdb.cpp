/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++

			(c) Copyright The Nexus Developers 2014 - 2018

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <memory>
#include <string>
#include <vector>

#include <db_cxx.h> /* Berkeley DB header */

#include <LLD/include/version.h>

#include <LLP/include/network.h>

#include <TAO/Legacy/net/addrman.h>
#include <TAO/Legacy/wallet/addrdb.h>

#include <Util/include/serialize.h>


namespace Legacy
{
    /* WriteAddrman */
    bool CAddrDB::WriteAddrman(const Legacy::Net::CAddrMan& addrman)
    {
        return Write(std::string("addrman"), addrman);
    }


    /* LoadAddresses */
    bool CAddrDB::LoadAddresses()
    {
        if (Read(std::string("addrman"), Legacy::Net::addrman))
        {
            printf("Loaded %i addresses\n", Legacy::Net::addrman.size());
            return true;
        }

        // Read pre-0.6 addr records

        std:vector<Net::CAddress> vAddr;

        // Get cursor
        std::shared_ptr<Dbc> pcursor = GetCursor();
        if (pcursor == nullptr)
            return false;

        loop() {
            // Read next record
            CDataStream ssKey(SER_DISK, LLD::DATABASE_VERSION);
            CDataStream ssValue(SER_DISK, LLD::DATABASE_VERSION);

            int ret = ReadAtCursor(pcursor, ssKey, ssValue);

            if (ret == DB_NOTFOUND)
                break;
            else if (ret != 0)
                return false;

            // Unserialize
            std::string strType;
            ssKey >> strType;

            if (strType == "addr")
            {
                Net::CAddress addr;
                ssValue >> addr;
                vAddr.push_back(addr);
            }
        }

        pcursor->close();

        Legacy::Net::addrman.Add(vAddr, Net::CNetAddr("0.0.0.0"));
        printf("Loaded %i addresses\n", Legacy::Net::addrman.size());

        // Note: old records left; we ran into hangs-on-startup
        // bugs for some users who (we think) were running after
        // an unclean shutdown.

        return true;
    }


    /* LoadAddresses - function */
    bool LoadAddresses()
    {
        return CAddrDB("cr+").LoadAddresses();
    }
}
