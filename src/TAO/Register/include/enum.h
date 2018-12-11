/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_TAO_REGISTER_INCLUDE_ENUM_H
#define NEXUS_TAO_REGISTER_INCLUDE_ENUM_H

namespace TAO::Register
{
    /** Operation Layer Byte Code. **/
    enum OBJECT
    {
        READONLY = 0x00, //this type of register cannot have the data changed
        RAW      = 0x01, //this type of register is just raw data that can be changed
        ACCOUNT  = 0x02, //this type of register handles general accounts and DEBITS / CREDITS
        TOKEN    = 0x03, //this type of register to hold token parameters
    };
}

#endif
