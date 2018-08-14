/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2017] ++
            
            (c) Copyright The Nexus Developers 2014 - 2017
            
            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.
            
            "fides in stellis, virtus in numeris" - Faith in the Stars, Power in Numbers

____________________________________________________________________________________________*/

#ifndef NEXUS_TAO_REGISTER_INCLUDE_STATE_H
#define NEXUS_TAO_REGISTER_INCLUDE_STATE_H


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
            //OP_GETHASH    = 0x06, //OP_GETHASH <vchData> return vchHashData NOTE: Not sure if we want these low level ops on this layer
            //0x07 - 0x0f UNASSIGNED


            //conditional operations
            OP_IF         = 0x10, //OP_IF <boolean expression>
            OP_ELSE       = 0x11, //OP_ELSE
            OP_ENDIF      = 0x12, //OP_ENDIF
            OP_NOT        = 0x13, //OP_NOT <bool expression>
            OP_EQUALS     = 0x14, //OP_EQUALS <vchData1> <VchData2>
            //0x15 - 0x1f UNASSIGNED


            //financial operators
            OP_ACCOUNT    = 0x20, //OP_ACCOUNT <vchAddress> <vchIdentifier> return fSuccess - create account
            OP_CREDIT     = 0x21, //OP_CREDIT <hashTransaction> <nAmount> return fSuccess
            OP_DEBIT      = 0x22, //OP_DEBIT <vchAccount> <nAmount> return fSuccess
            OP_BALANCE    = 0x23, //OP_BALANCE <vchAccount> <vchIdentifier> return nBalance
            OP_EXPIRE     = 0x24, //OP_EXPIRE <nTimestamp> return fExpire
            OP_CREATE     = 0x25, //OP_CREATE create the tokens
            //0x25 - 0x2f UNASSIGNED


            //joint ownership TODO (I own 50% of this copyright, you own 50%, when a royalty transaction hits, disperse to accounts)
            OP_LICENSE    = 0x30, //OP_LICENSE <vchRegAddress>
            //0x31 - 0x3f UNASSIGNED


            //chain state operations
            OP_HEIGHT     = 0x40, //OP_HEIGHT return nHeight
            OP_TIMESTAMP  = 0x41, //OP_TIMESTAMP return UnifiedTimestamp()
            OP_TXID       = 0x42, //OP_TXID return GetHash() - callers transaction hash
            //0x43 - 0x4f UNASSIGNED


            //consensus operations
            OP_VOTE       = 0x50, //OP_VOTE <vchData> <bool> return fSuccess - vote for or against a memory location (piece of data)
            OP_REQUIRE    = 0x51, //OP_REQUIRE <boolean expression> return fExpression - require something to be true to evaltue script

            //object register methods TODO: assess how we will handle pointers, current thoughts are through LISP IPv11 database clusters, where TCP/IP address is the pointer reference location (&), so pointers in the contract code will be hashes to represent the address space which can be located through opening up a TCP/IP socket to that reference location and getting the data returned so the network will act like a giant memory bank
            //OP_METHOD     = 0x50, //OP_METHOD return hashAddress
            //0x51 - 0x5f UNASSIGNED

            OP_METHOD     = 0xff; //OP_METHOD <vchAddress> return <vchData>
        };
        
    }
}
