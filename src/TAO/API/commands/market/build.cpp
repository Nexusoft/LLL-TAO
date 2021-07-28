/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/types/commands/market.h>
#include <TAO/API/include/execute.h>

#include <TAO/Operation/include/enum.h>
#include <TAO/Operation/types/contract.h>

#include <TAO/Register/include/unpack.h>
#include <TAO/Register/types/object.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Generic handler for creating new indexes for this specific command-set. */
    void Market::BuildIndexes(const TAO::Operation::Contract& rContract)
    {
        /* Start our stream at 0. */
        rContract.Reset();

        /* Get the operation byte. */
        uint8_t nType = 0;
        rContract >> nType;

        debug::log(0, FUNCTION, "market is live");

        /* Switch based on type. */
        switch(nType)
        {
            /* Check that transaction has a condition. */
            case TAO::Operation::OP::CONDITION:
            {
                /* Get the next OP. */
                rContract.Seek(4, TAO::Operation::Contract::CONDITIONS);

                /* Get the comparison bytes. */
                std::vector<uint8_t> vBytes;
                rContract >= vBytes;

                debug::warning(HexStr(vBytes.begin(), vBytes.end()));

                /* Get the next OP. */
                rContract.Seek(2, TAO::Operation::Contract::CONDITIONS);

                /* Grab our pre-state token-id. */
                std::string strToken;
                rContract >= strToken;

                debug::log(0, VARIABLE(strToken));

                /* Extract the data from the bytes. */
                TAO::Operation::Stream ssCompare(vBytes);
                ssCompare.seek(33);

                /* Get the address to. */
                uint256_t hashTo;
                ssCompare >> hashTo;

                /* Get the amount requested. */
                uint64_t nAmount = 0;
                ssCompare >> nAmount;

                break;
            }
        }
    }
}
