/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_TAO_OPERATION_INCLUDE_ENUM_H
#define NEXUS_TAO_OPERATION_INCLUDE_ENUM_H

/* Global TAO namespace. */
namespace TAO
{

    /* Operation Layer namespace. */
    namespace Operation
    {
        
        //NETF - ADS - Application Development Standard - Document to define new applicaqtion programming interface calls required by developers
        //NETF - NOS - Nexus Operation Standard - Document to define operation needs, formal design, and byte slot, and NETF engineers to develop the CASE statement
        //NETF - ORS - Object Register Standard - Document to define a specific object register for purpose of ADS standards, with NOS standards being capable of supporting methods


        /** Operation Layer Byte Code. **/
        enum OP
        {
            //register operations
            WRITE      = 0x01, //OP_WRITE <vchRegAddress> <vchData> return fSuccess
            REGISTER   = 0x02, //OP_REGISTER <vchRegData> return fSuccess
            AUTHORIZE  = 0x03, //OP_AUTHORIZE OP_GETHASH <vchPubKey> return fSuccess
            TRANSFER   = 0x04, //OP_TRANSFER <vchRegAddress> <vchGenesisID> return fSuccess
            REQUIRE    = 0x05, //OP_REQUIRE <boolean=expression> must validate to true.
            APPEND     = 0x06,

            //financial operations
            DEBIT      = 0x10,
            CREDIT     = 0x11,
            COINBASE   = 0x12,
            TRUST      = 0x13, //for proof of stake


            //internal funding
            AMBASSADOR = 0x20,
            DEVELOPER  = 0x21,

            //crypto operations
            SIGNATURE  = 0x30,

            //consensus operations
            VOTE       = 0x50, //OP_VOTE <vchData> <bool> return fSuccess - vote for or against a memory location (piece of data)
        };


        //Exchange would be:
        //OP_DEBIT <hash-from-a> <%hash-to%> 100 OP_REQUIRE OP_DEBIT <hash-from-b> 500 <token-id> OP_IF VAL_TIMESTAMP OP_LESSTHAN <future=time> OP_OR OP_CREDIT <%txid%> <hash-proof=0xff> <hash-to-a> 100

        //second debit:
        //OP_VALIDATE <txid> OP_DEBIT <hash-from-b> <hash-to-my-token> = 0xff> 500 <token-id>
        //---> OP_VALIDATE executes txid, if OP_REQUIRE satisfied

        //return to self
        //OP_VALIDATE <txid> OP_CREDIT <txid> <hash-proof=0xff> <hash-to-a> 100


        //claims
        //OP_CREDIT <txid of your debit> <hash-proof=0xff> <hash-to-b> 100
        //---> check state of locking transaction

        //OP_CREDIT <txid of your debit> <hash-proof=0xff> <hash-to-a> 500

        /** Validation Byte Code. **/
        enum VALIDATE
        {
            //register operations
            IF         = 0xf0,
            ELSE       = 0xf1,
            ENDIF      = 0xf2,
            AND        = 0xf3,
            OR         = 0xf4,
            EQUALS     = 0xf5,
            NOT        = 0xf6,

            LESSTHAN   = 0xf7,
            GREATTHAN  = 0xf8,

            RETURN     = 0xff
        };


        /** Values. **/
        enum
        {
            VAL_TIMESTAMP      = 0xe0

        };

    }
}

#endif
