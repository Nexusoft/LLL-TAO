/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/types/contracts/expiring.h>
#include <TAO/API/types/contracts/verify.h>

#include <TAO/Operation/include/enum.h>
#include <TAO/Operation/types/contract.h>

#include <Util/include/hex.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Verify that a given contract is wel formed and the binary data matches pattern. */
    bool Contracts::Verify(const std::vector<uint8_t> vByteCode, const TAO::Operation::Contract& rContract)
    {
        /* So we can save a lot of typing, only use 'using' in local scope source files. */
        using OP          = TAO::Operation::OP;
        using PLACEHOLDER = TAO::Operation::PLACEHOLDER;

        /* Grab a reference of our conditional contract. */
        const TAO::Operation::Stream ssContract =
            TAO::Operation::Stream(rContract.Conditions());

        /* Check our contract data byte by byte. */
        uint32_t nPos = 0; //so we can skip through conditions vector.
        while(!ssContract.end())
        {
            /* Get our current opcode. */
            const uint8_t nCode = vByteCode[nPos++];

            /* Check for a valid placeholder. */
            if(PLACEHOLDER::Valid(nCode))
                continue;

            /* Read our instruction from contract. */
            uint8_t nCheck;
            ssContract >> nCheck;

            /* Check our current op code. */
            if(nCheck != nCode)
                return debug::error("invalid instruction ", std::hex, uint32_t(nCheck), " expecting ", uint32_t(nCode));

            /* Check for a valid parameter type. */
            switch(nCode)
            {
                /* Handle for standard 8-bit unsigned integer. */
                case OP::TYPES::UINT8_T:
                {
                    ssContract.seek(1);
                    break;
                }

                /* Handle for standard 16-bit unsigned integer. */
                case OP::TYPES::UINT16_T:
                {
                    ssContract.seek(2);
                    break;
                }

                /* Handle for standard 32-bit unsigned integer. */
                case OP::TYPES::UINT32_T:
                case OP::SUBDATA: //subdata is just two shorts concatenated so handle on 32 bit case
                {
                    ssContract.seek(4);
                    break;
                }

                /* Handle for standard 64-bit unsigned integer. */
                case OP::TYPES::UINT64_T:
                {
                    ssContract.seek(8);
                    break;
                }

                /* Handle for standard 256-bit unsigned integer. */
                case OP::TYPES::UINT256_T:
                {
                    ssContract.seek(32);
                    break;
                }

                /* Handle for standard 512-bit unsigned integer. */
                case OP::TYPES::UINT512_T:
                {
                    ssContract.seek(64);
                    break;
                }

                /* Handle for standard 1024-bit unsigned integer. */
                case OP::TYPES::UINT1024_T:
                {
                    ssContract.seek(128);
                    break;
                }

                /* Handle for standard byte vector or strings. */
                case OP::TYPES::BYTES:
                case OP::TYPES::STRING:
                case OP::CALLER::PRESTATE::VALUE: //this instruction has a string parameter
                {
                    /* Get the size of our value. */
                    const uint64_t nSize =
                        ReadCompactSize(ssContract);

                    /* Seek over the binary data. */
                    ssContract.seek(nSize);
                    break;
                }

                /* Handle for register codes. */
                case OP::REGISTER::CREATED:
                case OP::REGISTER::MODIFIED:
                case OP::REGISTER::OWNER:
                case OP::REGISTER::TYPE:
                case OP::REGISTER::STATE:
                case OP::REGISTER::VALUE:
                {
                    /* A register code requires a 256-bit input parameter. */
                    ssContract.seek(32);

                    /* Register value requires string. */
                    if(nCode == OP::REGISTER::VALUE)
                    {
                        /* Get the size of our byte vector. */
                        const uint64_t nSize =
                            ReadCompactSize(ssContract);

                        /* Seek over the requested field now. */
                        ssContract.seek(nSize);
                    }

                    break;
                }
            }
        }

        return true;
    }
}
