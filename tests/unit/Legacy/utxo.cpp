/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <Legacy/include/create.h>

#include <Legacy/types/coinbase.h>

#include <Legacy/include/enum.h>

#include <TAO/Ledger/include/chainstate.h>

#include <unit/catch2/catch.hpp>

TEST_CASE("UTXO Unit Tests", "[UTXO]")
{
    {
        //reserve key from temp wallet
        Legacy::ReserveKey* pReserveKey = new Legacy::ReserveKey(&Legacy::Wallet::GetInstance());

        //dummy coinbase
        Legacy::Coinbase coinbase;

        //make the coinbase tx
        Legacy::Transaction tx;
        REQUIRE(Legacy::CreateCoinbase(*pReserveKey, coinbase, 1, 0, 7, tx));

        //add the data into input script
        tx.vin[0].scriptSig << uint32_t(555);

        //get inputs
        std::map<uint512_t, std::pair<uint8_t, DataStream> > inputs;
        REQUIRE(tx.FetchInputs(inputs));

        //get best
        TAO::Ledger::BlockState state = TAO::Ledger::ChainState::stateBest.load();

        //conect tx
        REQUIRE(tx.Connect(inputs, state, Legacy::FLAGS::BLOCK));

        //add to wallet
        Legacy::Wallet::GetInstance().AddToWalletIfInvolvingMe(tx, TAO::Ledger::ChainState::stateGenesis, true);

        //check balance
        REQUIRE(Legacy::Wallet::GetInstance().GetBalance() == 1000000);
    }
}
