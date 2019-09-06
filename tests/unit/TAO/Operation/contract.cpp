/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/include/random.h>

#include <LLD/include/global.h>

#include <TAO/Operation/include/enum.h>
#include <TAO/Operation/include/execute.h>
#include <TAO/Operation/include/enum.h>

#include <TAO/Operation/types/stream.h>

#include <TAO/Register/include/enum.h>

#include <TAO/Ledger/types/transaction.h>

#include <unit/catch2/catch.hpp>

using namespace TAO::Register;
using namespace TAO::Operation;
using namespace TAO::Ledger;

TEST_CASE( "Contract::Bind", "[operation]" )
{
    //build contract
    Contract contract;

    Transaction tx;

    tx.hashGenesis = LLC::GetRand256();
    tx.nTimestamp = runtime::timestamp();

    // check for expected output
    contract.Bind(&tx);

    REQUIRE(contract.Hash() == tx.GetHash());
    REQUIRE(contract.Timestamp() == tx.nTimestamp);
    REQUIRE(contract.Caller() == tx.hashGenesis);

    // check for erroneous input
    REQUIRE_THROWS( contract.Bind(nullptr) );

}


TEST_CASE( "Contract Tests", "[operation]" )
{

    {
        //build contract
        Contract contract;

        contract << uint8_t(OP::TRANSFER);

        //get op
        uint8_t nOP = 0;
        contract >> nOP;

        //test op
        REQUIRE(nOP == OP::TRANSFER);

        //test reset
        contract.Reset(Contract::OPERATIONS);

        //get op
        nOP = 0;
        contract >> nOP;

        //check op
        REQUIRE(nOP == OP::TRANSFER);

        //make pre-state
        contract <<= uint8_t(STATES::PRESTATE);

        //create register
        State state;
        contract <<= state;

        {
            //get pre-state
            uint8_t nType = 0;
            contract >>= nType;

            //check state
            REQUIRE(nType == STATES::PRESTATE);

            //get register
            State state2;
            contract >>= state2;

            //check states
            REQUIRE(state == state2);
        }

        //reset
        contract.Reset(Contract::REGISTERS);

        {
            //get pre-state
            uint8_t nType = 0;
            contract >>= nType;

            //check state
            REQUIRE(nType == STATES::PRESTATE);

            //get register
            State state2;
            contract >>= state2;

            //check states
            REQUIRE(state == state2);
        }
    }
}
