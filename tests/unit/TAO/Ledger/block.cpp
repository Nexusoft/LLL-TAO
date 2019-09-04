/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

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
