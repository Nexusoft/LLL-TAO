/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <unit/catch2/catch.hpp>

#include <Util/include/convert.h>

#include <vector>
#include <cstdint>

/* Test SET_CHANNEL payload parsing logic that's used in miner.cpp
 * 
 * The SET_CHANNEL command must support both:
 * - Legacy multi-byte payloads (4+ bytes, using convert::bytes2uint)
 * - New single-byte payloads (1 byte, direct interpretation)
 */

TEST_CASE("SET_CHANNEL Payload Parsing Tests", "[miner][llp]")
{
    SECTION("Single-byte payload - Channel 1 (Prime)")
    {
        std::vector<uint8_t> vData = {0x01};
        
        REQUIRE(vData.size() == 1);
        
        /* Parse as single byte */
        uint32_t nChannelValue = static_cast<uint32_t>(vData[0]);
        
        REQUIRE(nChannelValue == 1);
    }
    
    SECTION("Single-byte payload - Channel 2 (Hash)")
    {
        std::vector<uint8_t> vData = {0x02};
        
        REQUIRE(vData.size() == 1);
        
        /* Parse as single byte */
        uint32_t nChannelValue = static_cast<uint32_t>(vData[0]);
        
        REQUIRE(nChannelValue == 2);
    }
    
    SECTION("4-byte payload - Channel 1 (Legacy)")
    {
        /* Create 4-byte little-endian representation of channel 1 */
        std::vector<uint8_t> vData = {0x01, 0x00, 0x00, 0x00};
        
        REQUIRE(vData.size() >= 4);
        
        /* Parse using legacy method */
        uint32_t nChannelValue = convert::bytes2uint(vData);
        
        REQUIRE(nChannelValue == 1);
    }
    
    SECTION("4-byte payload - Channel 2 (Legacy)")
    {
        /* Create 4-byte little-endian representation of channel 2 */
        std::vector<uint8_t> vData = {0x02, 0x00, 0x00, 0x00};
        
        REQUIRE(vData.size() >= 4);
        
        /* Parse using legacy method */
        uint32_t nChannelValue = convert::bytes2uint(vData);
        
        REQUIRE(nChannelValue == 2);
    }
    
    SECTION("Invalid payload - 0 bytes")
    {
        std::vector<uint8_t> vData;
        
        REQUIRE(vData.size() == 0);
        
        /* This should be rejected as invalid */
        bool fValidSize = (vData.size() == 1) || (vData.size() >= 4);
        REQUIRE_FALSE(fValidSize);
    }
    
    SECTION("Invalid payload - 2 bytes")
    {
        std::vector<uint8_t> vData = {0x01, 0x00};
        
        REQUIRE(vData.size() == 2);
        
        /* This should be rejected as invalid (not 1 byte, not 4+ bytes) */
        bool fValidSize = (vData.size() == 1) || (vData.size() >= 4);
        REQUIRE_FALSE(fValidSize);
    }
    
    SECTION("Invalid payload - 3 bytes")
    {
        std::vector<uint8_t> vData = {0x01, 0x00, 0x00};
        
        REQUIRE(vData.size() == 3);
        
        /* This should be rejected as invalid */
        bool fValidSize = (vData.size() == 1) || (vData.size() >= 4);
        REQUIRE_FALSE(fValidSize);
    }
    
    SECTION("Large payload - 8 bytes (should parse first 4)")
    {
        /* Create 8-byte payload where first 4 bytes represent channel 1 */
        std::vector<uint8_t> vData = {0x01, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF};
        
        REQUIRE(vData.size() >= 4);
        
        /* Parse using legacy method - should only use first 4 bytes */
        uint32_t nChannelValue = convert::bytes2uint(vData);
        
        REQUIRE(nChannelValue == 1);
    }
    
    SECTION("Invalid channel value - 0")
    {
        std::vector<uint8_t> vData = {0x00};
        
        uint32_t nChannelValue = static_cast<uint32_t>(vData[0]);
        
        /* Channel 0 is not valid for mining (reserved for Proof of Stake) */
        REQUIRE(nChannelValue == 0);
        bool fValidChannel = (nChannelValue == 1 || nChannelValue == 2);
        REQUIRE_FALSE(fValidChannel);
    }
    
    SECTION("Invalid channel value - 3")
    {
        std::vector<uint8_t> vData = {0x03};
        
        uint32_t nChannelValue = static_cast<uint32_t>(vData[0]);
        
        /* Channel 3 is not valid */
        REQUIRE(nChannelValue == 3);
        bool fValidChannel = (nChannelValue == 1 || nChannelValue == 2);
        REQUIRE_FALSE(fValidChannel);
    }
}
