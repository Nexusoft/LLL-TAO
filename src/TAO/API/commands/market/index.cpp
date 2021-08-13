/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/types/commands/market.h>
#include <TAO/API/types/commands/names.h>
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
                try //in case de-serialization fails from non-standard contracts
                {
                    /* Get the next OP. */
                    rContract.Seek(4, TAO::Operation::Contract::CONDITIONS);

                    /* Get the comparison bytes. */
                    std::vector<uint8_t> vBytes;
                    rContract >= vBytes;

                    /* Get the next OP. */
                    rContract.Seek(2, TAO::Operation::Contract::CONDITIONS);

                    /* Grab our pre-state token-id. */
                    std::string strToken;
                    rContract >= strToken;

                    /* Get the next OP. */
                    rContract.Seek(2, TAO::Operation::Contract::CONDITIONS);

                    /* Grab our token-id now. */
                    uint256_t hashFirst;
                    rContract >= hashFirst;

                    /* Grab our other token from pre-state. */
                    TAO::Register::Object tPreState = rContract.PreState();

                    /* Skip over non objects for now. */
                    if(tPreState.nType != TAO::Register::REGISTER::OBJECT)
                        return;

                    /* Parse pre-state if needed. */
                    tPreState.Parse();

                    /* Grab the rhs token. */
                    const uint256_t hashSecond =
                        tPreState.get<uint256_t>("token");

                    /* Create our market-pair. */
                    const std::pair<uint256_t, uint256_t> pairMarket =
                        std::make_pair(hashFirst, hashSecond);

                    /* Write the order to logical database. */
                    if(!LLD::Contract->HasContract(std::make_pair(rContract.Hash(), nContract)))
                        LLD::Logical->PushOrder(pairMarket, rContract.Hash(), nContract);
                }
                catch(const std::exception& e)
                {
                    debug::warning(e.what());
                }

                break;
            }
        }
    }
}
