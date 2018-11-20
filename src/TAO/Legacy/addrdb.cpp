/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++

			(c) Copyright The Nexus Developers 2014 - 2018

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include "db.h"
#include "../util/util.h"
#include "../core/core.h"
#include <boost/version.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>

#include "../LLD/index.h"

#ifndef WIN32
#include "sys/stat.h"
#endif

using namespace std;
using namespace boost;


namespace Legacy
{
    //
    // CAddrDB
    //

    bool CAddrDB::WriteAddrman(const Legacy::Net::CAddrMan& addrman)
    {
        return Write(string("addrman"), addrman);
    }

    bool CAddrDB::LoadAddresses()
    {
        if (Read(string("addrman"), Net::addrman))
        {
            printf("Loaded %i addresses\n", Net::addrman.size());
            return true;
        }

        // Read pre-0.6 addr records

        vector<Net::CAddress> vAddr;
        vector<vector<uint8_t> > vDelete;

        // Get cursor
        Dbc* pcursor = GetCursor();
        if (!pcursor)
            return false;

        loop() {
            // Read next record
            CDataStream ssKey(SER_DISK, DATABASE_VERSION);
            CDataStream ssValue(SER_DISK, DATABASE_VERSION);
            int ret = ReadAtCursor(pcursor, ssKey, ssValue);
            if (ret == DB_NOTFOUND)
                break;
            else if (ret != 0)
                return false;

            // Unserialize
            string strType;
            ssKey >> strType;
            if (strType == "addr")
            {
                Net::CAddress addr;
                ssValue >> addr;
                vAddr.push_back(addr);
            }
        }
        pcursor->close();

        Net::addrman.Add(vAddr, Net::CNetAddr("0.0.0.0"));
        printf("Loaded %i addresses\n", Net::addrman.size());

        // Note: old records left; we ran into hangs-on-startup
        // bugs for some users who (we think) were running after
        // an unclean shutdown.

        return true;
    }

    bool LoadAddresses()
    {
        return CAddrDB("cr+").LoadAddresses();
    }
}
