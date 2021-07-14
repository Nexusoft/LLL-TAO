/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/types/commands/templates.h>
#include <TAO/API/types/exception.h>

#include <TAO/API/include/build.h>
#include <TAO/API/include/check.h>
#include <TAO/API/include/compare.h>
#include <TAO/API/include/extract.h>
#include <TAO/API/include/execute.h>
#include <TAO/API/include/filter.h>
#include <TAO/API/include/json.h>

#include <TAO/Operation/include/enum.h>

#include <TAO/Register/include/unpack.h>

#include <TAO/Ledger/types/transaction.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Lists all transactions for a given register. */
    encoding::json Templates::History(const encoding::json& jParams, const bool fHelp)
    {
        /* The register address of the object to get the transactions for. */
        const TAO::Register::Address hashRegister = ExtractAddress(jParams);

        /* Read the register from DB. */
        TAO::Register::State objCheck;
        if(!LLD::Register->ReadState(hashRegister, objCheck))
            throw Exception(-13, "Object not found");

        /* Use this asset to get our genesis-id adjusting if it is in system state. */
        uint256_t hashGenesis = objCheck.hashOwner;
        if(hashGenesis.GetType() == TAO::Ledger::GENESIS::SYSTEM)
            hashGenesis.SetType(TAO::Ledger::GENESIS::UserType());

        /* Number of results to return. */
        uint32_t nLimit = 100, nOffset = 0;

        /* Sort order to apply */
        std::string strOrder = "desc", strColumn = "modified";

        /* Get the params to apply to the response. */
        ExtractList(jParams, strOrder, strColumn, nLimit, nOffset);

        /* Get the last transaction. */
        uint512_t hashLast = 0;
        if(!LLD::Ledger->ReadLast(hashGenesis, hashLast))
            throw Exception(-144, "No transactions found");

        /* Build our object list and sort on insert. */
        std::set<encoding::json, CompareResults> setHistory({}, CompareResults(strOrder, strColumn));

        /* Loop until genesis. */
        while(hashLast != 0)
        {
            /* Get the transaction from disk. */
            TAO::Ledger::Transaction tx;
            if(!LLD::Ledger->ReadTx(hashLast, tx, TAO::Ledger::FLAGS::MEMPOOL))
                throw Exception(-108, "Failed to read transaction");

            /* Set the next last. */
            hashLast = !tx.IsFirst() ? tx.hashPrevTx : 0;

            /* Loop through our contracts to check if they match our address. */
            for(int32_t nContract = tx.Size() - 1; nContract >= 0; --nContract)
            {
                /* Get the contract. */
                const TAO::Operation::Contract& rContract = tx[nContract];

                /* Unpack the contract address. */
                uint256_t hashContract;
                if(!TAO::Register::Unpack(rContract, hashContract))
                    continue;

                /* Check that our address matches the contract. */
                if(hashRegister != hashContract)
                    continue;

                /* Now let's grab our OP so we can generate some JSON. */
                const uint8_t nPrimitive = rContract.Primitive();

                /* Grab our register's pre-state. */
                TAO::Register::Object tObject = ExecuteContract(rContract);

                /* Check if object needs to be parsed. */
                if(tObject.nType == TAO::Register::REGISTER::OBJECT)
                    tObject.Parse();

                /* Let's start building our json object. */
                encoding::json jRegister =
                    RegisterToJSON(tObject, hashContract);

                /* Now let's add some meta-data for given operation. */
                switch(nPrimitive)
                {
                    /* Handle for CREATE modifier type. */
                    case TAO::Operation::OP::CREATE:
                    {
                        jRegister["action"] = "CREATE";
                        break;
                    }

                    /* Handle for MODIFY modifier type. */
                    case TAO::Operation::OP::WRITE:
                    case TAO::Operation::OP::APPEND:
                    case TAO::Operation::OP::FEE:
                    {
                        jRegister["action"] = "MODIFY";
                        break;
                    }

                    /* Handle for TRANSFER modifier type. */
                    case TAO::Operation::OP::TRANSFER:
                    {
                        jRegister["action"] = "TRANSFER";
                        break;
                    }

                    /* Handle for CLAIM modifier type. */
                    case TAO::Operation::OP::CLAIM:
                    {
                        jRegister["action"] = "CLAIM";
                        break;
                    }

                    /* Handle for DEBIT modifier type. */
                    case TAO::Operation::OP::DEBIT:
                    case TAO::Operation::OP::LEGACY:
                    {
                        jRegister["action"] = "DEBIT";
                        break;
                    }

                    /* Handle for CREDIT modifier type. */
                    case TAO::Operation::OP::CREDIT:
                    {
                        jRegister["action"] = "CREDIT";
                        break;
                    }

                    /* Handle for TRUST modifier type. */
                    case TAO::Operation::OP::GENESIS:
                    case TAO::Operation::OP::TRUST:
                    case TAO::Operation::OP::MIGRATE:
                    {
                        jRegister["action"] = "TRUST";
                        break;
                    }
                }

                /* Check that we match our filters. */
                if(!FilterObject(jParams, jRegister, tObject))
                    continue;

                /* Filter out our expected fieldnames if specified. */
                FilterFieldname(jParams, jRegister);

                /* Insert into set and automatically sort. */
                setHistory.insert(jRegister);

                /* Jump sigchains on CLAIM. */
                if(nPrimitive == TAO::Operation::OP::CLAIM)
                {
                    /* Grab our tx-id now. */
                    rContract.SeekToPrimitive(false);
                    rContract >> hashLast;

                    break;
                }
            }
        }

        /* Build our return value. */
        encoding::json jRet = encoding::json::array();

        /* Handle paging and offsets. */
        uint32_t nTotal = 0;
        for(const auto& jRegister : setHistory)
        {
            /* Check the offset. */
            if(++nTotal <= nOffset)
                continue;

            /* Check the limit */
            if(jRet.size() == nLimit)
                break;

            jRet.push_back(jRegister);
        }

        return jRet;
    }
}
