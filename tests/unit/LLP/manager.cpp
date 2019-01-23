/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/
#include <tests/unit/LLP/include/manager.h>
#include <src/Util/include/debug.h>

namespace unit
{
    void test_address_manager()
    {
        LLP::AddressManager m(8888);
        LLP::BaseAddress b1, b2;
        LLP::TrustAddress a1, a2;

        std::vector<LLP::TrustAddress> vA;
        std::vector<LLP::BaseAddress> vB;

        debug::log(0, "size of BaseAddress: ", sizeof(LLP::BaseAddress));


        debug::log(0, "size of TrustAddress: ", sizeof(LLP::TrustAddress));

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
        m.AddAddress(a1, LLP::ConnectState::NEW);

        /* select an address */
        m.StochasticSelect(b1);
        debug::log(0, "Manager Selected ", b1.ToString());

        /* get count */
        debug::log(0, "Count ", m.Count());

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
        vB.push_back("158.222.168.204");
        vB.push_back("176.120.43.212");
        vB.push_back("70.95.81.133");
        vB.push_back("75.157.46.127");
        vB.push_back("66.189.172.36");
        vB.push_back("100.36.84.193");
        vB.push_back("95.156.243.20");

        /* add multiple addresses */
        debug::log(0, "Adding ", vB.size(), " Addresses");
        m.AddAddresses(vB);

        /* get count */
        debug::log(0, "Count ", m.Count());

        /* select an address */
        m.StochasticSelect(b1);
        debug::log(0, "Manager Selected ", b1.ToString());

        /* get base addresses */
        debug::log(0, "GetAddresses");
        vB = m.GetAddresses();

        /* set the port for addresses */
        debug::log(0, "Setting Ports to ", 9323);
        m.SetPort(9323);

        /* get address info and print */
        debug::log(0, "GetAddresses");
        m.GetAddresses(vA);
        for(auto it = vA.begin(); it != vA.end(); ++it)
         debug::log(0, it->ToString());

        /* print */
        m.PrintStats();

        /* write to database */
        debug::log(0, "Writing to database");
        m.WriteDatabase();

        /* read from database */
        debug::log(0, "Read from database");
        m.ReadDatabase();

        /* get address info and print */
        debug::log(0, "GetAddresses");
        m.GetAddresses(vA);
        for(auto it = vA.begin(); it != vA.end(); ++it)
         debug::log(0, it->ToString());
    }
}
