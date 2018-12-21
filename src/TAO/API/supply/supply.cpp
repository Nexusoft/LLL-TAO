/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/include/random.h>

#include <LLD/include/global.h>

#include <TAO/API/include/supply.h>

#include <TAO/Ledger/types/sigchain.h>

#include <TAO/Operation/include/execute.h>

#include <TAO/Register/include/enum.h>

namespace TAO::API
{
    /* Declaration of the api */
    Supply supply;


    /* Standard initialization function. */
    void Supply::Initialize()
    {
        mapFunctions["getitem"]             = Function(std::bind(&Supply::GetItem,   this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["transfer"]            = Function(std::bind(&Supply::Transfer,  this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["submit"]              = Function(std::bind(&Supply::Submit,    this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["history"]             = Function(std::bind(&Supply::History,   this, std::placeholders::_1, std::placeholders::_2));
    }


    /* Get's the description of an item. */
    json::json Supply::GetItem(const json::json& params, bool fHelp)
    {
        json::json ret = {"getitem", params};

        //uint256_t hashAddress;
        //TAO::Register::State state;
        //LLD::regDB->ReadState(hashAddress, state);

        return ret;
    }


    /* Transfers an item. */
    json::json Supply::Transfer(const json::json& params, bool fHelp)
    {
        json::json ret = {"transfer", params};

        //TAO::Ledger tx;
        //tx << OP::TRANSFER << hashRegister << hashAddressTo;

        return ret;
    }


    /* Submits an item. */
    json::json Supply::Submit(const json::json& params, bool fHelp)
    {
        json::json ret;

        /* Check for username parameter. */
        if(params.find("username") == params.end())
            throw APIException(-23, "Missing Username");

        /* Check for password parameter. */
        if(params.find("password") == params.end())
            throw APIException(-24, "Missing Password");

        /* Check for pin parameter. */
        if(params.find("pin") == params.end())
            throw APIException(-25, "Missing PIN");

        /* Generate the signature chain. */
        TAO::Ledger::SignatureChain user(params["username"], params["password"]);

        /* Get the Genesis ID. */
        uint256_t hashGenesis = LLC::SK256(user.Generate(0, params["pin"]).GetBytes());

        /* Get the last transaction. */
        uint512_t hashLast;
        if(!LLD::locDB->ReadLast(hashGenesis, hashLast))
            throw APIException(-28, "No transactions found");

        /* Get previous transaction */
        TAO::Ledger::Transaction txPrev;
        if(!LLD::legDB->ReadTx(hashLast, txPrev))
            throw APIException(-29, "Failed to read previous transaction");

        /* Build new transaction object. */
        TAO::Ledger::Transaction tx;
        tx.nSequence   = txPrev.nSequence + 1;
        tx.hashGenesis = txPrev.hashGenesis;
        tx.hashPrevTx  = hashLast;
        tx.NextHash(user.Generate(tx.nSequence + 1, params["pin"]));

        /* Submit the transaction payload. */
        std::vector<uint8_t> vchPayload(50, 0);
        tx << TAO::Operation::OP::REGISTER << LLC::GetRand256() << TAO::Register::OBJECT::READONLY << vchPayload;

        /* Sign the transaction. */
        if(!tx.Sign(user.Generate(tx.nSequence, params["pin"])))
            throw APIException(-26, "Failed to sign transaction");

        /* Check that the transaction is valid. */
        if(!tx.IsValid())
            throw APIException(-26, "Invalid Transaction");

        /* Execute the operations layer. */
        //if(!TAO::Operation::Execute(tx.vchLedgerData, hashGenesis))
        //    throw APIException(-26, "Operations failed to execute");

        /* Write transaction to local database. */
        LLD::legDB->WriteTx(tx.GetHash(), tx);
        LLD::locDB->WriteLast(hashGenesis, tx.GetHash());

        /* Build a JSON response object. */
        ret["version"]   = tx.nVersion;
        ret["sequence"]  = tx.nSequence;
        ret["timestamp"] = tx.nTimestamp;
        ret["genesis"]   = tx.hashGenesis.ToString();
        ret["nexthash"]  = tx.hashNext.ToString();
        ret["prevhash"]  = tx.hashPrevTx.ToString();
        ret["pubkey"]    = HexStr(tx.vchPubKey.begin(), tx.vchPubKey.end());
        ret["signature"] = HexStr(tx.vchSig.begin(),    tx.vchSig.end());
        ret["payload"]   = HexStr(tx.vchLedgerData.begin(), tx.vchLedgerData.end());
        ret["hash"]      = tx.GetHash().ToString();

        return ret;
    }


    /* Gets the history of an item. */
    json::json Supply::History(const json::json& params, bool fHelp)
    {
        json::json ret = {"history", params};

        return ret;
    }
}
