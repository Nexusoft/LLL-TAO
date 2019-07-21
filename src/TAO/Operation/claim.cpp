/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/Operation/include/claim.h>
#include <TAO/Operation/include/enum.h>

#include <TAO/Register/include/constants.h>
#include <TAO/Register/include/enum.h>
#include <TAO/Register/include/reserved.h>
#include <TAO/Register/types/state.h>

/* Global TAO namespace. */
namespace TAO
{

    /* Operation Layer namespace. */
    namespace Operation
    {

        /* Commit the final state to disk. */
        bool Claim::Commit(const TAO::Register::State& state,
            const uint256_t& hashAddress, const uint512_t& hashTx, const uint32_t nContract, const uint8_t nFlags)
        {
            /* Check if this transfer is already claimed. */
            if(LLD::Ledger->HasProof(hashAddress, hashTx, nContract, nFlags))
                return debug::error(FUNCTION, "transfer is already claimed");

            /* Write the claimed proof. */
            if(!LLD::Ledger->WriteProof(hashAddress, hashTx, nContract, nFlags))
                return debug::error(FUNCTION, "transfer is already claimed");

            /* Attempt to write new state to disk. */
            if(!LLD::Register->WriteState(hashAddress, state, nFlags))
                return debug::error(FUNCTION, "failed to write post-state to disk");

            return true;
        }


        /* Claims a register from a transfer. */
        bool Claim::Execute(TAO::Register::State &state,
            const uint256_t& hashClaim, const uint64_t nTimestamp)
        {
            /* Make sure the register claim is in SYSTEM pending from a transfer. */
            if(state.hashOwner != 0)
                return debug::error(FUNCTION, "can't claim untransferred register");

            /* Set the new owner of the register. */
            state.hashOwner  = hashClaim;
            state.nModified = nTimestamp;
            state.SetChecksum();

            /* Check register for validity. */
            if(!state.IsValid())
                return debug::error(FUNCTION, "post-state is in invalid state");

            return true;
        }


        /* Verify claim validation rules and caller. */
        bool Claim::Verify(const Contract& contract, const Contract& claim)
        {
            /* Reset register streams. */
            claim.Reset(Contract::REGISTERS);

            /* Get operation byte. */
            uint8_t OP = 0;
            claim >> OP;

            /* Check for condition or validate. */
            switch(OP)
            {
                /* Handle a condition. */
                case OP::CONDITION:
                {
                    /* Get new OP. */
                    claim >> OP;

                    break;
                }

                /* Handle a validate. */
                case OP::VALIDATE:
                {
                    /* Seek past validate. */
                    claim.Seek(68);

                    /* Get new OP. */
                    claim >> OP;

                    break;
                }
            }

            /* Check operation byte. */
            if(OP != OP::TRANSFER)
                return debug::error(FUNCTION, "cannot claim a register with no transfer");

            /* Extract the address  */
            TAO::Register::Address hashAddress;
            claim >> hashAddress;

            /* Check for reserved values. */
            if(TAO::Register::Reserved(hashAddress))
                return debug::error(FUNCTION, "cannot claim register with reserved address");

            /* Read the register transfer recipient. */
            uint256_t hashTransfer;
            claim >> hashTransfer;

            /* Get the state byte. */
            uint8_t nState = 0; //RESERVED
            claim >>= nState;

            /* Check for the pre-state. */
            if(nState != TAO::Register::STATES::PRESTATE)
                return debug::error(FUNCTION, "register script not in pre-state");

            /* Get the pre-state. */
            TAO::Register::State state;
            claim >>= state;

            /* Check the addresses match. */
            if(state.hashOwner != contract.Caller() //claim to self
            && hashTransfer    != contract.Caller() //calim to transfer
            && hashTransfer    != TAO::Register::WILDCARD_ADDRESS)   //claim to wildcard (anyone)
                return debug::error(FUNCTION, "claim public-id mismatch with transfer address");

            /* Check that pre-state is valid. */
            if(!state.IsValid())
                return debug::error(FUNCTION, "pre-state is in invalid state");

            /* Reset the register streams. */
            claim.Reset(Contract::OPERATIONS | Contract::REGISTERS);

            return true;
        }
    }
}
