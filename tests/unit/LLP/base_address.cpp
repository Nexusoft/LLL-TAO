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
const std::vector<TestIpInput> ipInputInvalidV4 = {{"0.0", {0, 0}, TestIpVersion::v4}};

sockaddr_in StringToSockAddr(const std::string& ipAddress)
{
    struct sockaddr_in sa;
    inet_pton(AF_INET, ipAddress.c_str(), &(sa.sin_addr));
    return sa;
}


TEST_CASE( "LLP::BaseAddress", "[base_address]")
{
    LLP::BaseAddress a1, a2;

    a1.SetIP(std::string("192.168.0.1"));
    a1.SetPort(9325);

    a2.SetIP(std::string("192.168.0.2"));
    a2.SetPort(8325);

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
}

}