/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_TAO_OPERATION_INCLUDE_ENUM_H
#define NEXUS_TAO_OPERATION_INCLUDE_ENUM_H

namespace TAO
{
    namespace Operation
    {
        //NETF - ADS - Application Development Standard - Document to define new applicaqtion programming interface calls required by developers
        //NETF - NOS - Nexus Operation Standard - Document to define operation needs, formal design, and byte slot, and NETF engineers to develop the CASE statement
        //NETF - ORS - Object Register Standard - Document to define a specific object register for purpose of ADS standards, with NOS standards being capable of supporting methods


        /** Operation Layer Byte Code. **/
        enum
        {
            //register operations
            OP_WRITE      = 0x01, //OP_WRITE <vchRegAddress> <vchData> return fSuccess
            OP_READ       = 0x02, //OP_READ <vchRegAddress> return <vchData>
            OP_REGISTER   = 0x03, //OP_REGISTER <vchRegData> return fSuccess
            OP_AUTHORIZE  = 0x04, //OP_AUTHORIZE OP_GETHASH <vchPubKey> return fSuccess
            OP_TRANSFER   = 0x05, //OP_TRANSFER <vchRegAddress> <vchGenesisID> return fSuccess

            //financial operations
            OP_DEBIT      = 0x10,
            OP_CREDIT     = 0x11,
            OP_EXCHANGE   = 0x12,

            //crypto operations
            OP_SIGNATURE  = 0x20,

            //consensus operations
            OP_VOTE       = 0x50, //OP_VOTE <vchData> <bool> return fSuccess - vote for or against a memory location (piece of data)


            OP_METHOD     = 0xff //OP_METHOD <vchAddress> return <vchData>
        };

    }
}

#endif
