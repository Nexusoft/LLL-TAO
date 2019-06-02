/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/Operation/include/validate.h>
#include <TAO/Operation/types/contract.h>

/* Global TAO namespace. */
namespace TAO
{

    /* Operation Layer namespace. */
    namespace Operation
    {
         /* Commit validation proofs. **/
        bool Validate::Commit(const uint512_t& hashTx, const uint32_t nContract, const uint8_t nFlags)
        {
            /* Check that contract hasn't already been completed. */
            if(LLD::Contract->HasContract(std::make_pair(hashTx, nContract)))
                return debug::error(FUNCTION, "cannot validate when already fulfilled");

            return true;
        }


        /* Verify validation rules and caller. */
        bool Validate::Verify(const Contract& contract, const Contract& condition)
        {
            return true;
        }
    }
}
