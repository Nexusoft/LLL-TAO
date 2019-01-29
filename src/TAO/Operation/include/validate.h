/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_TAO_OPERATION_INCLUDE_VALIDATE_H
#define NEXUS_TAO_OPERATION_INCLUDE_VALIDATE_H

#include <LLC/types/uint1024.h>
#include <TAO/Opeartion/include/stream.h>

namespace TAO
{

    namespace Operation
    {

        /** Validate
         *
         *  Validates a given operation expression. This would evaluate to a boolean expression required to return true.
         *
         *  @param[in] regDB The register database to execute on
         *  @param[in] hashOwner The owner executing the register batch.
         *
         *  @return True if operations executed successfully, false otherwise.
         *
         **/
        bool Validate(std::vector<uint8_t> vchData, uint256_t hashOwner)
        {
            /* Create the operations stream to execute. */
            Stream stream = Stream(vchData);

            while(!stream.end())
            {
                uint8_t OP;
                stream >> OP;
                switch(OP)
                {

                }
            }
        }
    }
}

#endif
