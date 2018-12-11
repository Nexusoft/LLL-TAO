/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/Operation/include/operations.h>
#include <TAO/Register/include/state.h>
#include <TAO/Register/include/enum.h>

namespace TAO::Operation
{

    /* Writes data to a register. */
    bool Authorize(uint512_t hashTx, uint256_t hashProof, uint256_t hashCaller)
    {

        //read the proof
        //read the transaction
        //check that transaction operation is requiring a proof by checking the register owner to a token.
        //have a register handle the votes tally. Only transactions with proofs can tally the authorization
        //if the proofs are above a threshold on the register writes, allow the validation logic to succeed

        return true;
    }

}
