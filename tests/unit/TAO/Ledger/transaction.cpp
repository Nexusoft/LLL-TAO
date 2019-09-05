/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/Ledger/types/transaction.h>

#include <unit/catch2/catch.hpp>

//test greater than operator
TEST_CASE( "Transaction::operator>", "[ledger]" )
{
    TAO::Ledger::Transaction tx1, tx2;

    tx1.nSequence = 1;
    tx2.nSequence = 2;

    REQUIRE_FALSE(tx1 > tx2);
    REQUIRE(tx2 > tx1);
}


//test less than operator
TEST_CASE( "Transaction::operator<", "[ledger]" )
{
    TAO::Ledger::Transaction tx1, tx2;

    tx1.nSequence = 1;
    tx2.nSequence = 2;

    REQUIRE(tx1 < tx2);
    REQUIRE_FALSE(tx2 < tx1);
}
