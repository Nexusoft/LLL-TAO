/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/include/random.h>

#include <LLD/include/global.h>

#include <TAO/Ledger/types/sigchain.h>

#include <Util/include/args.h>
#include <Util/include/config.h>
#include <Util/include/signals.h>
#include <Util/include/convert.h>
#include <Util/include/runtime.h>
#include <Util/include/filesystem.h>

#include <TAO/API/include/cmd.h>
#include <LLP/types/corenode.h>
#include <LLP/types/rpcnode.h>
#include <TAO/API/include/rpc.h>

#include <LLP/include/global.h>

#include <TAO/Ledger/types/mempool.h>
#include <TAO/Ledger/include/chainstate.h>

#include <TAO/API/include/supply.h>
#include <TAO/API/include/accounts.h>

#include <Legacy/wallet/wallet.h>
#include <Legacy/wallet/walletdb.h>

#include <TAO/Operation/include/execute.h>

#include <LLP/types/miner.h>

#include <iostream>
#include <sstream>

#include <TAO/Ledger/include/create.h>
#include <TAO/Ledger/include/constants.h>



/* Declare the Global LLD Instances. */
namespace LLD
{
    RegisterDB* regDB;
    LedgerDB*   legDB;
    LocalDB*    locDB;

    //for legacy objects.
    LegacyDB*   legacyDB;
}


/* Declare the Global LLP Instances. */
namespace LLP
{
    Server<TritiumNode>* TRITIUM_SERVER;
    Server<LegacyNode> * LEGACY_SERVER;
}

#include <LLP/include/baseaddress.h>
#include <LLP/include/addressinfo.h>
#include <LLP/include/manager.h>
using namespace LLP;
int main(int argc, char** argv)
{
   AddressManager m(8888);
   BaseAddress b1, b2;
   AddressInfo a1, a2;

   std::vector<AddressInfo> vA;
   std::vector<BaseAddress> vB;


   debug::log(0, "size of BaseAddress: ", sizeof(BaseAddress));


   debug::log(0, "size of AddressInfo: ", sizeof(AddressInfo));

   /*default intialized */
   debug::log(0, a1.ToString());

   /* set ip */
   a1.SetIP("192.168.0.1");
   debug::log(0, "a1.SetIP ", a1.ToString());

  /* set port */
   a1.SetPort(9323);
   debug::log(0, "a1.SetPort ", a1.ToString());

   /* add single address */
   debug::log(0, "Adding Address ", a1.ToString());
   m.AddAddress(a1, ConnectState::NEW);

   /* select an address */
   m.StochasticSelect(b1);
   debug::log(0, "Manager Selected ", b1.ToString());

   /* get count */
   debug::log(0, "GetInfoCount ", m.GetInfoCount());

   /* set ip */
   b2.SetIP("192.168.0.2");
   debug::log(0, "b2.SetIP ", b2.ToString());

  /* set port */
   b2.SetPort(8323);
   debug::log(0, "b2.SetPort ", b2.ToString());

   /* add single address */
   //debug::log(0, "Adding Address ", b2.ToString());
   //m.AddAddress(b2, ConnectState::NEW);

   /* select an address */
   m.StochasticSelect(b1);
   debug::log(0, "Manager Selected ", b1.ToString());

   /* create 23 addresses */


   vB.push_back("188.25.62.40");
   vB.push_back("66.231.19.15");
   vB.push_back("89.17.150.169");
   vB.push_back("86.128.67.48");
   vB.push_back("59.15.19.170");
   vB.push_back("83.27.246.52");
   vB.push_back("101.183.92.90");
   vB.push_back("86.139.222.80");
   vB.push_back("73.68.17.62");
   vB.push_back("110.78.173.250");
   vB.push_back("181.208.99.210");
   vB.push_back("79.185.49.82");
   vB.push_back("37.248.79.235");
   vB.push_back("188.255.113.22");


   vB.push_back("104.250.163.34");
   vB.push_back("90.202.124.33");
   //vB.push_back("158.222.168.204");
   //vB.push_back("176.120.43.212");
   //vB.push_back("70.95.81.133");
   //vB.push_back("75.157.46.127");
   //vB.push_back("66.189.172.36");
   //vB.push_back("100.36.84.193");
   //vB.push_back("95.156.243.20");

   /* add multiple addresses */
   m.AddAddresses(vB);

   /* select an address */
   m.StochasticSelect(b1);
   debug::log(0, "Manager Selected ", b1.ToString());

   /* get base addresses */
   debug::log(0, "GetAddresses");
   vB = m.GetAddresses();

   /* get address info */
   debug::log(0, "GetInfo");
   m.GetInfo(vA);

   /* get count */
   debug::log(0, "GetInfoCount ", m.GetInfoCount());

   /* print */
   m.PrintStats();

  return 0;
}
