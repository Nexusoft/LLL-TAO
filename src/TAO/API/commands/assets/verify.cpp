/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/types/authentication.h>
#include <TAO/API/types/commands/assets.h>

#include <TAO/API/include/extract.h>
#include <TAO/API/include/json.h>

#include <TAO/Operation/include/enum.h>

#include <TAO/Ledger/types/transaction.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Generic method to list object registers by sig chain*/
    encoding::json Assets::Verify(const encoding::json& jParams, const bool fHelp)
    {
        /* Get the Register address. */
        const TAO::Register::Address hashToken = ExtractToken(jParams);

        /* Check if we have a transfer event for this token. */
        encoding::json jRet;
        jRet["valid"] = false;

        /* Loop through contracts to find the source event. */
        TAO::Ledger::Transaction tx;
        if(LLD::Ledger->ReadEvent(hashToken, 0, tx))
        {
            /* Check our contracts now. */
            for(uint32_t nContract = 0; nContract < tx.Size(); nContract++)
            {
                /* Get a reference of the transaction contract. */
                const TAO::Operation::Contract& rEvent = tx[nContract];

                /* Reset the op stream */
                rEvent.SeekToPrimitive();

                /* The operation */
                uint8_t nOp;
                rEvent >> nOp;

                /* Skip over non transfers. */
                if(nOp != TAO::Operation::OP::TRANSFER)
                    continue;

                /* The register address being transferred */
                uint256_t hashTransfer;
                rEvent >> hashTransfer;

                /* Get the new owner hash */
                uint256_t hashTokenized;
                rEvent >> hashTokenized;

                /* Check that the recipient of the transfer is the token */
                if(hashTokenized != hashToken)
                    continue;

                /* Read the force transfer flag */
                uint8_t nType = 0;
                rEvent >> nType;

                /* Ensure this was a forced transfer (which tokenized asset transfers must be) */
                if(nType != TAO::Operation::TRANSFER::FORCE)
                    continue;

                /* Populate our asset information here. */
                jRet["asset"] =
                    AddressToJSON(hashTransfer, rEvent);

                /* Set that this is a valid tokenized asset. */
                jRet["valid"] = true;

                break; //we can exit now
            }
        }

        return jRet;
    }
}
