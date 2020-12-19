/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <unit/catch2/catch.hpp>

#include <LLP/include/base_address.h>

#include <vector>
#include <string>
#include <iostream>

namespace
{

enum class TestIpVersion { v4, v6 };

struct TestIpInput 
{
    std::string str;
    std::vector<std::uint8_t> bin;
    TestIpVersion ver;
};

const std::vector<std::uint16_t> portInput = {{0, 1, std::numeric_limits<std::uint16_t>::max()}};
const std::vector<TestIpInput> ipInputV4 = 
{
    {"0.0.0.0", {0, 0, 0, 0}, TestIpVersion::v4},
    {"192.168.0.1", {192, 168, 0, 1}, TestIpVersion::v4},
    {"255.255.255.255", {255, 255, 255, 255}, TestIpVersion::v4}
};
const std::vector<TestIpInput> ipInputV6 = 
{
    {"::", {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, TestIpVersion::v6},
    {"::192.168.0.1", {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 192, 168, 0, 1}, TestIpVersion::v6},
    {"fd00::55AA", {0xfd, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x55, 0xaa}, TestIpVersion::v6},
    {"60:61:62:63:64:65:66:67",
     {0x00, 0x60, 0x00, 0x61, 0x00, 0x62, 0x00, 0x63, 0x00, 0x64, 0x00, 0x65, 0x00, 0x66, 0x00, 0x67},
     TestIpVersion::v6},
    {"feff:FFFF:Ffff:fFff:ffFf:fffF:ffff:ffff",
     {0xFe, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
     TestIpVersion::v6}
};
const std::vector<TestIpInput> ipInputInvalidV4 = {{"0.0", {0, 0}, TestIpVersion::v4}};


TEST_CASE( "LLP::BaseAddress", "[base_address]")
{
    LLP::BaseAddress a1{};

    a1.SetIP(std::string("192.168.0.1"));
    a1.SetPort(9325);

    SECTION( "operator==" ) 
    {
        REQUIRE(a1 == a1);
    }

    SECTION( "copy constructor" ) 
    {
        LLP::BaseAddress copyTest{a1};
        REQUIRE(copyTest == a1);
    }

    SECTION( "operator=" ) 
    {
        LLP::BaseAddress copyTest{};
        copyTest = a1;
        REQUIRE(copyTest == a1);
    }

    SECTION( "operator!=" ) 
    {
        LLP::BaseAddress a2{};
        a2.SetIP(std::string("192.168.0.2"));
        a2.SetPort(8325);

        REQUIRE(a1 != a2);
    }

    SECTION( "GetPort" ) 
    {
        for (auto const& nPort : portInput) 
        {
            LLP::BaseAddress defaultBaseAddress{};
            LLP::BaseAddress portTest{defaultBaseAddress, nPort};
            REQUIRE(portTest.GetPort() == nPort);
        }
    }

    SECTION( "SetPort" ) 
    {
        for (auto const& nPort : portInput) 
        {
            LLP::BaseAddress portTest{};
            portTest.SetPort(nPort);
            REQUIRE(portTest.GetPort() == nPort);
        }
    }

    SECTION( "ToStringPort" )
    {
        for (auto const& nPort : portInput) 
        {
            LLP::BaseAddress portTest{};
            portTest.SetPort(nPort);
            REQUIRE(portTest.ToStringPort() == std::to_string(nPort));
        }
    }

    SECTION( "ToStringIP" )
    {
        for (auto const& ip : ipInputV4)
        {
            LLP::BaseAddress ipString{ip.str};
            REQUIRE(ipString.ToStringIP() == ip.str);
        }
    }

    SECTION( "ToString" )
    {
        for (auto const& ip : ipInputV4)
        {
            LLP::BaseAddress ipString{ip.str, portInput.front()};
            REQUIRE(ipString.ToString() == ip.str + ":" + std::to_string(portInput.front()));
        }
    }

    SECTION( "IsIpV4" ) 
    {
        for (auto const& ip : ipInputV4)
        {
            LLP::BaseAddress ipV4{ip.str};
            REQUIRE(ipV4.IsIPv4() == true);
        }

        for (auto const& ip : ipInputInvalidV4)
        {
            LLP::BaseAddress ipV4{ip.str};
            REQUIRE(ipV4.IsIPv4() == false);
        }
    }

    SECTION( "GetByte runtime_exception" ) 
    {
        LLP::BaseAddress address{};
        REQUIRE_THROWS_AS(address.GetByte(16), std::runtime_error);
    }

    SECTION( "GetInAddr v4" ) 
    {
        for (auto const& ip : ipInputV4)
        {
            in_addr resultIpv4Addr, compareIpv4Addr;
            LLP::BaseAddress address{ip.str};
            REQUIRE(address.GetInAddr(&resultIpv4Addr) == true);

            inet_aton(ip.str.c_str(), &compareIpv4Addr);
            REQUIRE(resultIpv4Addr.s_addr == compareIpv4Addr.s_addr);           
        }
    }

    SECTION( "GetInAddr v4 failure" ) 
    {
        for (auto const& ip : ipInputInvalidV4)
        {
            in_addr resultIpv4Addr;
            LLP::BaseAddress address{ip.str};

            REQUIRE(address.GetInAddr(&resultIpv4Addr) == false);      
        }
    }

    SECTION( "GetIn6Addr v6" ) 
    {
        for (auto const& ip : ipInputV6)
        {
            in6_addr resultIpv6Addr, compareIpv6Addr;
            LLP::BaseAddress address{ip.str};
            REQUIRE(address.GetIn6Addr(&resultIpv6Addr) == true);

            inet_pton(AF_INET6, ip.str.c_str(), &compareIpv6Addr);

            REQUIRE(memcmp(&resultIpv6Addr, &compareIpv6Addr, sizeof(struct in6_addr)) == 0);         
        }
    }

    SECTION( "GetSockAddr" ) 
    {
        for (auto const& ip : ipInputV4)
        {
            sockaddr_in* pSockAddr = new sockaddr_in{};
            in_addr compareIpv4Addr;
            LLP::BaseAddress address{ip.str};

            REQUIRE(address.GetSockAddr(pSockAddr) == true); 
            REQUIRE(pSockAddr->sin_family == AF_INET);  

            inet_aton(ip.str.c_str(), &compareIpv4Addr);
            REQUIRE(pSockAddr->sin_addr.s_addr == compareIpv4Addr.s_addr);  

            delete pSockAddr;
        }
    }

    SECTION( "GetSockAddr nullptr failure" ) 
    {
        for (auto const& ip : ipInputV4)
        {
            sockaddr_in* pSockAddr{};
            LLP::BaseAddress address{};

            REQUIRE(address.GetSockAddr(pSockAddr) == false);  
        }
    }

    SECTION( "GetSockAddr v4 failure" ) 
    {
        for (auto const& ip : ipInputInvalidV4)
        {
            sockaddr_in* pSockAddr = new sockaddr_in{};
            LLP::BaseAddress address{ip.str};

            REQUIRE(address.GetSockAddr(pSockAddr) == false);    

            delete pSockAddr;  
        }
    }

    SECTION( "GetSockAddr6" ) 
    {
        for (auto const& ip : ipInputV6)
        {
            auto* pSockAddr = new sockaddr_in6{};
            in6_addr compareIpv6Addr;
            LLP::BaseAddress address{ip.str};

            REQUIRE(address.GetSockAddr6(pSockAddr) == true); 
            REQUIRE(pSockAddr->sin6_family == AF_INET6);  

            inet_pton(AF_INET6, ip.str.c_str(), &compareIpv6Addr);
            REQUIRE(memcmp(&pSockAddr->sin6_addr, &compareIpv6Addr, sizeof(struct in6_addr)) == 0);

            delete pSockAddr;
        }
    }

    SECTION( "GetSockAddr6 nullptr failure" ) 
    {
        for (auto const& ip : ipInputV6)
        {
            sockaddr_in6* pSockAddr{};
            LLP::BaseAddress address{};

            REQUIRE(address.GetSockAddr6(pSockAddr) == false);  
        }
    }

    SECTION( "constructor in_addr" ) 
    {
        for (auto const& ip : ipInputV4)
        {
            in_addr inputIpv4Addr, resultIpv4Addr;
            inet_aton(ip.str.c_str(), &inputIpv4Addr);

            LLP::BaseAddress address{inputIpv4Addr};

            REQUIRE(address.GetInAddr(&resultIpv4Addr) == true);            
            REQUIRE(inputIpv4Addr.s_addr == resultIpv4Addr.s_addr);           
        }
    }

    SECTION( "constructor in6_addr" ) 
    {
        for (auto const& ip : ipInputV6)
        {
            in6_addr inputIpv6Addr, resultIpv6Addr;
            inet_pton(AF_INET6, ip.str.c_str(), &inputIpv6Addr);

            LLP::BaseAddress address{inputIpv6Addr};

            REQUIRE(address.GetIn6Addr(&resultIpv6Addr) == true);            
            REQUIRE(memcmp(&inputIpv6Addr, &resultIpv6Addr, sizeof(struct in6_addr)) == 0);         
        }
    }


    

    
}

}