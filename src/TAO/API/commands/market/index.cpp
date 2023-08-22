/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2023

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/types/commands/market.h>
#include <TAO/API/types/commands/names.h>
#include <TAO/API/types/contracts/exchange.h>
#include <TAO/API/include/contracts/verify.h>
#include <TAO/API/include/execute.h>

#include <TAO/Operation/include/enum.h>
#include <TAO/Operation/types/contract.h>

#include <TAO/Register/include/unpack.h>
#include <TAO/Register/types/object.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Generic handler for creating new indexes for this specific command-set. */
    void Market::Index(const TAO::Operation::Contract& rContract, const uint32_t nContract)
    {
        /* Start our stream at 0. */
        rContract.Reset();

        /* Get the operation byte. */
        uint8_t nType = 0;
        rContract >> nType;

        /* Switch based on type. */
        switch(nType)
        {
            /* Check that transaction has a condition. */
            case TAO::Operation::OP::CONDITION:
            {
                /* Check for valid exchange contract. */
                if(Contracts::Verify(Contracts::Exchange::Token[0], rContract)) //checking for version 1
                {
                    try //in case de-serialization fails from non-standard contracts
                    {
                        /* Get the next OP. */
                        rContract.Seek(4, TAO::Operation::Contract::CONDITIONS);

                        /* Get the comparison bytes. */
                        TAO::Operation::Stream ssBytes;
                        rContract >= ssBytes;

                        /* Skip ahead to our token-id. */
                        ssBytes.seek(33, STREAM::BEGIN);

                        /* Grab our deposit token-id now. */
                        uint256_t hashDeposit;
                        ssBytes >> hashDeposit;

                        /* Read the object to get token-id. */
                        TAO::Register::Object oDeposit;
                        if(!LLD::Register->ReadObject(hashDeposit, oDeposit))
                            return;

                        /* Grab our other withdraw token-id from pre-state. */
                        TAO::Register::Object oWithdraw =
                            rContract.PreState();

                        /* Skip over non objects for now. */
                        if(oWithdraw.nType != TAO::Register::REGISTER::OBJECT)
                            return;

                        /* Parse pre-state if needed. */
                        oWithdraw.Parse();

                        /* Create our market-pair. */
                        const std::pair<uint256_t, uint256_t> pairMarket =
                            std::make_pair(oDeposit.get<uint256_t>("token"), oWithdraw.get<uint256_t>("token"));

                        /* Write the order to logical database. */
                        if(!LLD::Logical->PushOrder(pairMarket, rContract, nContract))
                            debug::warning(FUNCTION, "Indexing failed for tx ", rContract.Hash().SubString());

                        /* Give a verbose=3 debug log for the indexing entry. */
                        if(config::nVerbose >= 3)
                        {
                            /* This will hold our market name. */
                            std::string strMarket =
                                (TAO::Register::Address(pairMarket.first).ToString() + "/");

                            /* Build our market-pair. */
                            std::string strName;
                            if(Names::ReverseLookup(pairMarket.first, strName))
                                strMarket = strName + "/";

                            /* Now add our second pair. */
                            if(!Names::ReverseLookup(pairMarket.second, strName))
                                strMarket += TAO::Register::Address(pairMarket.second).ToString();
                            else
                                strMarket += strName;

                            /* Output our new debug info. */
                            debug::log(3, "Market ", strMarket, " record created for ", rContract.Hash().SubString(), " txid");
                        }

                    }
                    catch(const std::exception& e)
                    {
                        debug::warning(e.what());
                    }
                }

                break;
            }
        }
    }
}
