/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <cstddef>

#include <TAO/Legacy/types/enum.h>

namespace Legacy
{

    /*  Formats the Transaction Type enumeration into a string.
     *  Used in RPC server only. To be Deprecated.
     */
    const char* GetTxnOutputType(TransactionType t)
    {
        switch (t)
        {
            case TX_NONSTANDARD: return "nonstandard";
            case TX_PUBKEY: return "pubkey";
            case TX_PUBKEYHASH: return "pubkeyhash";
            case TX_SCRIPTHASH: return "scripthash";
            case TX_MULTISIG: return "multisig";
        }

        return NULL;
    }


    /*  Formats the Operation Code enumeration into a string.
     *  Used in script string only. To be Deprecated.
     */
    const char* GetOpName(opcodetype opcode)
    {
        switch (opcode)
        {
            // push value
            case OP_0                      : return "0";
            case OP_PUSHDATA1              : return "OP_PUSHDATA1";
            case OP_PUSHDATA2              : return "OP_PUSHDATA2";
            case OP_PUSHDATA4              : return "OP_PUSHDATA4";
            case OP_1NEGATE                : return "-1";
            case OP_RESERVED               : return "OP_RESERVED";
            case OP_1                      : return "1";
            case OP_2                      : return "2";
            case OP_3                      : return "3";
            case OP_4                      : return "4";
            case OP_5                      : return "5";
            case OP_6                      : return "6";
            case OP_7                      : return "7";
            case OP_8                      : return "8";
            case OP_9                      : return "9";
            case OP_10                     : return "10";
            case OP_11                     : return "11";
            case OP_12                     : return "12";
            case OP_13                     : return "13";
            case OP_14                     : return "14";
            case OP_15                     : return "15";
            case OP_16                     : return "16";

            // control
            case OP_NOP                    : return "OP_NOP";
            case OP_VER                    : return "OP_VER";
            case OP_IF                     : return "OP_IF";
            case OP_NOTIF                  : return "OP_NOTIF";
            case OP_VERIF                  : return "OP_VERIF";
            case OP_VERNOTIF               : return "OP_VERNOTIF";
            case OP_ELSE                   : return "OP_ELSE";
            case OP_ENDIF                  : return "OP_ENDIF";
            case OP_VERIFY                 : return "OP_VERIFY";
            case OP_RETURN                 : return "OP_RETURN";

            // stack ops
            case OP_TOALTSTACK             : return "OP_TOALTSTACK";
            case OP_FROMALTSTACK           : return "OP_FROMALTSTACK";
            case OP_2DROP                  : return "OP_2DROP";
            case OP_2DUP                   : return "OP_2DUP";
            case OP_3DUP                   : return "OP_3DUP";
            case OP_2OVER                  : return "OP_2OVER";
            case OP_2ROT                   : return "OP_2ROT";
            case OP_2SWAP                  : return "OP_2SWAP";
            case OP_IFDUP                  : return "OP_IFDUP";
            case OP_DEPTH                  : return "OP_DEPTH";
            case OP_DROP                   : return "OP_DROP";
            case OP_DUP                    : return "OP_DUP";
            case OP_NIP                    : return "OP_NIP";
            case OP_OVER                   : return "OP_OVER";
            case OP_PICK                   : return "OP_PICK";
            case OP_ROLL                   : return "OP_ROLL";
            case OP_ROT                    : return "OP_ROT";
            case OP_SWAP                   : return "OP_SWAP";
            case OP_TUCK                   : return "OP_TUCK";

            // splice ops
            case OP_CAT                    : return "OP_CAT";
            case OP_SUBSTR                 : return "OP_SUBSTR";
            case OP_LEFT                   : return "OP_LEFT";
            case OP_RIGHT                  : return "OP_RIGHT";
            case OP_SIZE                   : return "OP_SIZE";

            // bit logic
            case OP_INVERT                 : return "OP_INVERT";
            case OP_AND                    : return "OP_AND";
            case OP_OR                     : return "OP_OR";
            case OP_XOR                    : return "OP_XOR";
            case OP_EQUAL                  : return "OP_EQUAL";
            case OP_EQUALVERIFY            : return "OP_EQUALVERIFY";
            case OP_RESERVED1              : return "OP_RESERVED1";
            case OP_RESERVED2              : return "OP_RESERVED2";

            // numeric
            case OP_1ADD                   : return "OP_1ADD";
            case OP_1SUB                   : return "OP_1SUB";
            case OP_2MUL                   : return "OP_2MUL";
            case OP_2DIV                   : return "OP_2DIV";
            case OP_NEGATE                 : return "OP_NEGATE";
            case OP_ABS                    : return "OP_ABS";
            case OP_NOT                    : return "OP_NOT";
            case OP_0NOTEQUAL              : return "OP_0NOTEQUAL";
            case OP_ADD                    : return "OP_ADD";
            case OP_SUB                    : return "OP_SUB";
            case OP_MUL                    : return "OP_MUL";
            case OP_DIV                    : return "OP_DIV";
            case OP_MOD                    : return "OP_MOD";
            case OP_LSHIFT                 : return "OP_LSHIFT";
            case OP_RSHIFT                 : return "OP_RSHIFT";
            case OP_BOOLAND                : return "OP_BOOLAND";
            case OP_BOOLOR                 : return "OP_BOOLOR";
            case OP_NUMEQUAL               : return "OP_NUMEQUAL";
            case OP_NUMEQUALVERIFY         : return "OP_NUMEQUALVERIFY";
            case OP_NUMNOTEQUAL            : return "OP_NUMNOTEQUAL";
            case OP_LESSTHAN               : return "OP_LESSTHAN";
            case OP_GREATERTHAN            : return "OP_GREATERTHAN";
            case OP_LESSTHANOREQUAL        : return "OP_LESSTHANOREQUAL";
            case OP_GREATERTHANOREQUAL     : return "OP_GREATERTHANOREQUAL";
            case OP_MIN                    : return "OP_MIN";
            case OP_MAX                    : return "OP_MAX";
            case OP_WITHIN                 : return "OP_WITHIN";

            // crypto
            case OP_HASH256                : return "OP_HASH256";
            case OP_CODESEPARATOR          : return "OP_CODESEPARATOR";
            case OP_CHECKSIG               : return "OP_CHECKSIG";
            case OP_CHECKSIGVERIFY         : return "OP_CHECKSIGVERIFY";
            case OP_CHECKMULTISIG          : return "OP_CHECKMULTISIG";
            case OP_CHECKMULTISIGVERIFY    : return "OP_CHECKMULTISIGVERIFY";

            // expanson
            case OP_NOP1                   : return "OP_NOP1";
            case OP_NOP2                   : return "OP_NOP2";
            case OP_NOP3                   : return "OP_NOP3";
            case OP_NOP4                   : return "OP_NOP4";
            case OP_NOP5                   : return "OP_NOP5";
            case OP_NOP6                   : return "OP_NOP6";
            case OP_NOP7                   : return "OP_NOP7";
            case OP_NOP8                   : return "OP_NOP8";
            case OP_NOP9                   : return "OP_NOP9";
            case OP_NOP10                  : return "OP_NOP10";



            // template matching params
            case OP_PUBKEYHASH             : return "OP_PUBKEYHASH";
            case OP_PUBKEY                 : return "OP_PUBKEY";

            case OP_INVALIDOPCODE          : return "OP_INVALIDOPCODE";
            default:
                return "OP_UNKNOWN";
        }
    }
}
