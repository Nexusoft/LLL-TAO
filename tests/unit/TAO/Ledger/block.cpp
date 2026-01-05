/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/Ledger/types/block.h>
#include <TAO/Ledger/types/tritium.h>
#include <TAO/Ledger/types/state.h>

#include <unit/catch2/catch.hpp>

TEST_CASE( "Block primitive values", "[ledger]")
{
    TAO::Ledger::Block block;

    REQUIRE(block.IsNull());

    //test a base block constructor
    block = TAO::Ledger::Block(1, 0, 3, 5);
    REQUIRE(block.nVersion == 1);
    REQUIRE(block.hashPrevBlock == 0);
    REQUIRE(block.nChannel == 3);
    REQUIRE(block.nHeight  == 5);

    block.hashMerkleRoot = 555;
    block.nBits          = 333;
    block.nNonce         = 222;
    //block.nTime          = 999; // nTime removed from base block


    //test a copy from a base block
    TAO::Ledger::TritiumBlock block2 = TAO::Ledger::TritiumBlock(block);
    REQUIRE(block2.nVersion == 1);
    REQUIRE(block2.hashPrevBlock == 0);
    REQUIRE(block2.nChannel  == 3);
    REQUIRE(block2.nHeight   == 5);
    REQUIRE(block2.hashMerkleRoot == 555);
    REQUIRE(block2.nBits     == 333);
    REQUIRE(block2.nNonce    == 222);
    block2.nTime  = 999; // nTime set to current unifiedtimestamp on copy from base block so set it to testable value here


    //test a copy from tritium block
    TAO::Ledger::TritiumBlock block3 = TAO::Ledger::TritiumBlock(block2);
    REQUIRE(block3.nVersion == 1);
    REQUIRE(block3.hashPrevBlock == 0);
    REQUIRE(block3.nChannel  == 3);
    REQUIRE(block3.nHeight   == 5);
    REQUIRE(block3.hashMerkleRoot == 555);
    REQUIRE(block3.nBits     == 333);
    REQUIRE(block3.nNonce    == 222);
    REQUIRE(block3.nTime     == 999);

    //test for basic failed checks
    REQUIRE(block3.Check() == false);


    //test for channel overflow
    block3.nChannel = 0xffffffff;
    REQUIRE(block3.Check() == false);


    //test for invalid block version
    block3.nChannel = 1;
    block3.nVersion = 0;
    REQUIRE(block3.Check() == false);


    //test for invalid block version
    block3.nVersion = 7;
    REQUIRE(block3.Check() == false);


    TAO::Ledger::BlockState state = TAO::Ledger::BlockState(block3);
    REQUIRE(state.nVersion == 7);
    REQUIRE(state.hashPrevBlock == 0);
    REQUIRE(state.nChannel  == 1);
    REQUIRE(state.nHeight   == 5);
    REQUIRE(state.hashMerkleRoot == 555);
    REQUIRE(state.nBits     == 333);
    REQUIRE(state.nNonce    == 222);
    REQUIRE(state.nTime     == 999);

    //check chain params
    state.nChainTrust       = 32;
    state.nMoneySupply      = 9293;
    state.nMint             = 239;
    state.nChannelHeight    = 553;
    state.hashNextBlock     = block3.GetHash();
    state.hashCheckpoint    = block3.GetHash();


    //test the copy
    TAO::Ledger::BlockState state2 = state;
    REQUIRE(state2.nVersion == 7);
    REQUIRE(state2.hashPrevBlock == 0);
    REQUIRE(state2.nChannel  == 1);
    REQUIRE(state2.nHeight   == 5);
    REQUIRE(state2.hashMerkleRoot == 555);
    REQUIRE(state2.nBits     == 333);
    REQUIRE(state2.nNonce    == 222);
    REQUIRE(state2.nTime     == 999);
    REQUIRE(state2.nChainTrust       == 32);
    REQUIRE(state2.nMoneySupply      == 9293);
    REQUIRE(state2.nMint             == 239);
    REQUIRE(state2.nChannelHeight    == 553);
    REQUIRE(state2.hashNextBlock     == block3.GetHash());
    REQUIRE(state2.hashCheckpoint    == block3.GetHash());


}


TEST_CASE( "Prime Calculation - Nonce Endianness (PR #128)", "[ledger][prime][endianness]")
{
    SECTION("GetPrime() correctly handles little-endian nonce")
    {
        /* Create a block with known values */
        TAO::Ledger::Block block(1, 0, 1, 100);  // version=1, prev=0, channel=1 (prime), height=100
        
        /* Set a known nonce value in little-endian format */
        /* This represents the nonce 0x3400000207b56300 which is the value from the problem statement */
        block.nNonce = 0x3400000207b56300ULL;
        
        /* Calculate prime - this should use the nonce value as-is (little-endian) */
        uint1024_t nPrime1 = block.GetPrime();
        
        /* Calculate expected prime using explicit addition */
        uint1024_t nProofHash = block.ProofHash();
        uint1024_t nExpected = nProofHash + 0x3400000207b56300ULL;
        
        /* Verify that GetPrime produces the expected result */
        REQUIRE(nPrime1 == nExpected);
        
        /* Verify it's NOT equal to the big-endian interpretation */
        uint1024_t nWrongBE = nProofHash + 0x0063b50702000034ULL;  // Big-endian (WRONG!)
        REQUIRE(nPrime1 != nWrongBE);
    }
    
    SECTION("Different nonce values produce different primes")
    {
        TAO::Ledger::Block block1(1, 0, 1, 100);
        TAO::Ledger::Block block2(1, 0, 1, 100);
        
        /* Set different nonce values */
        block1.nNonce = 0x1234567890ABCDEFULL;
        block2.nNonce = 0xFEDCBA0987654321ULL;
        
        /* Calculate primes */
        uint1024_t nPrime1 = block1.GetPrime();
        uint1024_t nPrime2 = block2.GetPrime();
        
        /* They should be different */
        REQUIRE(nPrime1 != nPrime2);
        
        /* But the difference should be exactly the difference in nonce */
        uint1024_t nDiff = (nPrime1 > nPrime2) ? (nPrime1 - nPrime2) : (nPrime2 - nPrime1);
        uint64_t nNonceDiff = (block1.nNonce > block2.nNonce) ? 
            (block1.nNonce - block2.nNonce) : (block2.nNonce - block1.nNonce);
        
        REQUIRE(nDiff == nNonceDiff);
    }
    
    SECTION("Zero nonce produces base prime equal to ProofHash")
    {
        TAO::Ledger::Block block(1, 0, 1, 100);
        block.nNonce = 0;
        
        uint1024_t nPrime = block.GetPrime();
        uint1024_t nProofHash = block.ProofHash();
        
        /* With nonce=0, prime should equal ProofHash */
        REQUIRE(nPrime == nProofHash);
    }
}
