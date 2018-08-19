/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <boost/foreach.hpp>
#include <boost/tuple/tuple.hpp>

using namespace std;
using namespace boost;

#include "script.h"
#include "key.h"
#include "keystore.h"

#include "../core/core.h"
#include "../util/util.h"
#include "../util/bignum.h"

namespace Wallet
{

    bool CheckSig(vector<uint8_t> vchSig, vector<uint8_t> vchPubKey, CScript scriptCode, const Core::CTransaction& txTo, uint32_t nIn, int nHashType);

    static const std::vector<uint8_t> vchFalse(0);
    static const std::vector<uint8_t> vchZero(0);
    static const std::vector<uint8_t> vchTrue(1, 1);
    static const CBigNum bnZero(0);
    static const CBigNum bnOne(1);
    static const CBigNum bnFalse(0);
    static const CBigNum bnTrue(1);
    static const size_t nMaxNumSize = 4;


    CBigNum CastToBigNum(const std::vector<uint8_t>& vch)
    {
        if (vch.size() > nMaxNumSize)
            throw runtime_error("CastToBigNum() : overflow");
        // Get rid of extra leading zeros
        return CBigNum(CBigNum(vch).getvch());
    }

    bool CastToBool(const std::vector<uint8_t>& vch)
    {
        for (uint32_t i = 0; i < vch.size(); i++)
        {

            if (vch[i] != 0)
            {
                // Can be negative zero
                if (i == vch.size() - 1 && vch[i] == 0x80)
                    return false;

                return true;
            }
        }

        return false;
    }

    void MakeSameSize(std::vector<uint8_t>& vch1, std::vector<uint8_t>& vch2)
    {
        // Lengthen the shorter one
        if (vch1.size() < vch2.size())
            vch1.resize(vch2.size(), 0);
        if (vch2.size() < vch1.size())
            vch2.resize(vch1.size(), 0);
    }



    //
    // Script is a stack machine (like Forth) that evaluates a predicate
    // returning a bool indicating valid or not.  There are no loops.
    //
    #define stacktop(i)  (stack.at(stack.size()+(i)))
    #define altstacktop(i)  (altstack.at(altstack.size()+(i)))
    static inline void popstack(vector<std::vector<uint8_t> >& stack)
    {
        if (stack.empty())
            throw runtime_error("popstack() : stack empty");
        stack.pop_back();
    }


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

    bool EvalScript(vector<vector<uint8_t> >& stack, const CScript& script, const Core::CTransaction& txTo, uint32_t nIn, int nHashType)
    {
        CAutoBN_CTX pctx;
        CScript::const_iterator pc = script.begin();
        CScript::const_iterator pend = script.end();
        CScript::const_iterator pbegincodehash = script.begin();
        opcodetype opcode;
        std::vector<uint8_t> vchPushValue;
        vector<bool> vfExec;
        vector<std::vector<uint8_t> > altstack;
        if (script.size() > 10000)
            return false;
        int nOpCount = 0;


        try
        {
            while (pc < pend)
            {
                bool fExec = !count(vfExec.begin(), vfExec.end(), false);

                //
                // Read instruction
                //
                if (!script.GetOp(pc, opcode, vchPushValue))
                    return false;
                if (vchPushValue.size() > 520)
                    return false;
                if (opcode > OP_16 && ++nOpCount > 201)
                    return false;

                if (opcode == OP_CAT ||
                    opcode == OP_SUBSTR ||
                    opcode == OP_LEFT ||
                    opcode == OP_RIGHT ||
                    opcode == OP_INVERT ||
                    opcode == OP_AND ||
                    opcode == OP_OR ||
                    opcode == OP_XOR ||
                    opcode == OP_2MUL ||
                    opcode == OP_2DIV ||
                    opcode == OP_MUL ||
                    opcode == OP_DIV ||
                    opcode == OP_MOD ||
                    opcode == OP_LSHIFT ||
                    opcode == OP_RSHIFT)
                    return false;

                if (fExec && 0 <= opcode && opcode <= OP_PUSHDATA4)
                    stack.push_back(vchPushValue);

                else if (fExec || (OP_IF <= opcode && opcode <= OP_ENDIF))
                switch (opcode)
                {
                    //
                    // Push value
                    //
                    case OP_1NEGATE:
                    case OP_1:
                    case OP_2:
                    case OP_3:
                    case OP_4:
                    case OP_5:
                    case OP_6:
                    case OP_7:
                    case OP_8:
                    case OP_9:
                    case OP_10:
                    case OP_11:
                    case OP_12:
                    case OP_13:
                    case OP_14:
                    case OP_15:
                    case OP_16:
                    {
                        // ( -- value)
                        CBigNum bn((int)opcode - (int)(OP_1 - 1));
                        stack.push_back(bn.getvch());
                    }
                    break;


                    //
                    // Control
                    //
                    case OP_NOP:
                    case OP_NOP1: case OP_NOP2: case OP_NOP3: case OP_NOP4: case OP_NOP5:
                    case OP_NOP6: case OP_NOP7: case OP_NOP8: case OP_NOP9: case OP_NOP10:
                    break;

                    case OP_IF:
                    case OP_NOTIF:
                    {
                        // <expression> if [statements] [else [statements]] endif
                        bool fValue = false;
                        if (fExec)
                        {
                            if (stack.size() < 1)
                                return false;
                            std::vector<uint8_t>& vch = stacktop(-1);
                            fValue = CastToBool(vch);
                            if (opcode == OP_NOTIF)
                                fValue = !fValue;
                            popstack(stack);
                        }
                        vfExec.push_back(fValue);
                    }
                    break;

                    case OP_ELSE:
                    {
                        if (vfExec.empty())
                            return false;
                        vfExec.back() = !vfExec.back();
                    }
                    break;

                    case OP_ENDIF:
                    {
                        if (vfExec.empty())
                            return false;
                        vfExec.pop_back();
                    }
                    break;

                    case OP_VERIFY:
                    {
                        // (true -- ) or
                        // (false -- false) and return
                        if (stack.size() < 1)
                            return false;
                        bool fValue = CastToBool(stacktop(-1));
                        if (fValue)
                            popstack(stack);
                        else
                            return false;
                    }
                    break;

                    case OP_RETURN:
                    {
                        return false;
                    }
                    break;


                    //
                    // Stack ops
                    //
                    case OP_TOALTSTACK:
                    {
                        if (stack.size() < 1)
                            return false;
                        altstack.push_back(stacktop(-1));
                        popstack(stack);
                    }
                    break;

                    case OP_FROMALTSTACK:
                    {
                        if (altstack.size() < 1)
                            return false;
                        stack.push_back(altstacktop(-1));
                        popstack(altstack);
                    }
                    break;

                    case OP_2DROP:
                    {
                        // (x1 x2 -- )
                        if (stack.size() < 2)
                            return false;
                        popstack(stack);
                        popstack(stack);
                    }
                    break;

                    case OP_2DUP:
                    {
                        // (x1 x2 -- x1 x2 x1 x2)
                        if (stack.size() < 2)
                            return false;
                        std::vector<uint8_t> vch1 = stacktop(-2);
                        std::vector<uint8_t> vch2 = stacktop(-1);
                        stack.push_back(vch1);
                        stack.push_back(vch2);
                    }
                    break;

                    case OP_3DUP:
                    {
                        // (x1 x2 x3 -- x1 x2 x3 x1 x2 x3)
                        if (stack.size() < 3)
                            return false;
                        std::vector<uint8_t> vch1 = stacktop(-3);
                        std::vector<uint8_t> vch2 = stacktop(-2);
                        std::vector<uint8_t> vch3 = stacktop(-1);
                        stack.push_back(vch1);
                        stack.push_back(vch2);
                        stack.push_back(vch3);
                    }
                    break;

                    case OP_2OVER:
                    {
                        // (x1 x2 x3 x4 -- x1 x2 x3 x4 x1 x2)
                        if (stack.size() < 4)
                            return false;
                        std::vector<uint8_t> vch1 = stacktop(-4);
                        std::vector<uint8_t> vch2 = stacktop(-3);
                        stack.push_back(vch1);
                        stack.push_back(vch2);
                    }
                    break;

                    case OP_2ROT:
                    {
                        // (x1 x2 x3 x4 x5 x6 -- x3 x4 x5 x6 x1 x2)
                        if (stack.size() < 6)
                            return false;
                        std::vector<uint8_t> vch1 = stacktop(-6);
                        std::vector<uint8_t> vch2 = stacktop(-5);
                        stack.erase(stack.end()-6, stack.end()-4);
                        stack.push_back(vch1);
                        stack.push_back(vch2);
                    }
                    break;

                    case OP_2SWAP:
                    {
                        // (x1 x2 x3 x4 -- x3 x4 x1 x2)
                        if (stack.size() < 4)
                            return false;
                        swap(stacktop(-4), stacktop(-2));
                        swap(stacktop(-3), stacktop(-1));
                    }
                    break;

                    case OP_IFDUP:
                    {
                        // (x - 0 | x x)
                        if (stack.size() < 1)
                            return false;
                        std::vector<uint8_t> vch = stacktop(-1);
                        if (CastToBool(vch))
                            stack.push_back(vch);
                    }
                    break;

                    case OP_DEPTH:
                    {
                        // -- stacksize
                        CBigNum bn(stack.size());
                        stack.push_back(bn.getvch());
                    }
                    break;

                    case OP_DROP:
                    {
                        // (x -- )
                        if (stack.size() < 1)
                            return false;
                        popstack(stack);
                    }
                    break;

                    case OP_DUP:
                    {
                        // (x -- x x)
                        if (stack.size() < 1)
                            return false;
                        std::vector<uint8_t> vch = stacktop(-1);
                        stack.push_back(vch);
                    }
                    break;

                    case OP_NIP:
                    {
                        // (x1 x2 -- x2)
                        if (stack.size() < 2)
                            return false;
                        stack.erase(stack.end() - 2);
                    }
                    break;

                    case OP_OVER:
                    {
                        // (x1 x2 -- x1 x2 x1)
                        if (stack.size() < 2)
                            return false;
                        std::vector<uint8_t> vch = stacktop(-2);
                        stack.push_back(vch);
                    }
                    break;

                    case OP_PICK:
                    case OP_ROLL:
                    {
                        // (xn ... x2 x1 x0 n - xn ... x2 x1 x0 xn)
                        // (xn ... x2 x1 x0 n - ... x2 x1 x0 xn)
                        if (stack.size() < 2)
                            return false;
                        int n = CastToBigNum(stacktop(-1)).getint();
                        popstack(stack);
                        if (n < 0 || n >= (int)stack.size())
                            return false;
                        std::vector<uint8_t> vch = stacktop(-n-1);
                        if (opcode == OP_ROLL)
                            stack.erase(stack.end()-n-1);
                        stack.push_back(vch);
                    }
                    break;

                    case OP_ROT:
                    {
                        // (x1 x2 x3 -- x2 x3 x1)
                        //  x2 x1 x3  after first swap
                        //  x2 x3 x1  after second swap
                        if (stack.size() < 3)
                            return false;
                        swap(stacktop(-3), stacktop(-2));
                        swap(stacktop(-2), stacktop(-1));
                    }
                    break;

                    case OP_SWAP:
                    {
                        // (x1 x2 -- x2 x1)
                        if (stack.size() < 2)
                            return false;
                        swap(stacktop(-2), stacktop(-1));
                    }
                    break;

                    case OP_TUCK:
                    {
                        // (x1 x2 -- x2 x1 x2)
                        if (stack.size() < 2)
                            return false;
                        std::vector<uint8_t> vch = stacktop(-1);
                        stack.insert(stack.end()-2, vch);
                    }
                    break;


                    //
                    // Splice ops
                    //
                    case OP_CAT:
                    {
                        // (x1 x2 -- out)
                        if (stack.size() < 2)
                            return false;
                        std::vector<uint8_t>& vch1 = stacktop(-2);
                        std::vector<uint8_t>& vch2 = stacktop(-1);
                        vch1.insert(vch1.end(), vch2.begin(), vch2.end());
                        popstack(stack);
                        if (stacktop(-1).size() > 520)
                            return false;
                    }
                    break;

                    case OP_SUBSTR:
                    {
                        // (in begin size -- out)
                        if (stack.size() < 3)
                            return false;
                        std::vector<uint8_t>& vch = stacktop(-3);
                        int nBegin = CastToBigNum(stacktop(-2)).getint();
                        int nEnd = nBegin + CastToBigNum(stacktop(-1)).getint();
                        if (nBegin < 0 || nEnd < nBegin)
                            return false;
                        if (nBegin > (int)vch.size())
                            nBegin = vch.size();
                        if (nEnd > (int)vch.size())
                            nEnd = vch.size();
                        vch.erase(vch.begin() + nEnd, vch.end());
                        vch.erase(vch.begin(), vch.begin() + nBegin);
                        popstack(stack);
                        popstack(stack);
                    }
                    break;

                    case OP_LEFT:
                    case OP_RIGHT:
                    {
                        // (in size -- out)
                        if (stack.size() < 2)
                            return false;
                        std::vector<uint8_t>& vch = stacktop(-2);
                        int nSize = CastToBigNum(stacktop(-1)).getint();
                        if (nSize < 0)
                            return false;
                        if (nSize > (int)vch.size())
                            nSize = vch.size();
                        if (opcode == OP_LEFT)
                            vch.erase(vch.begin() + nSize, vch.end());
                        else
                            vch.erase(vch.begin(), vch.end() - nSize);
                        popstack(stack);
                    }
                    break;

                    case OP_SIZE:
                    {
                        // (in -- in size)
                        if (stack.size() < 1)
                            return false;
                        CBigNum bn(stacktop(-1).size());
                        stack.push_back(bn.getvch());
                    }
                    break;


                    //
                    // Bitwise logic
                    //
                    case OP_INVERT:
                    {
                        // (in - out)
                        if (stack.size() < 1)
                            return false;
                        std::vector<uint8_t>& vch = stacktop(-1);
                        for (uint32_t i = 0; i < vch.size(); i++)
                            vch[i] = ~vch[i];
                    }
                    break;

                    case OP_AND:
                    case OP_OR:
                    case OP_XOR:
                    {
                        // (x1 x2 - out)
                        if (stack.size() < 2)
                            return false;
                        std::vector<uint8_t>& vch1 = stacktop(-2);
                        std::vector<uint8_t>& vch2 = stacktop(-1);
                        MakeSameSize(vch1, vch2);
                        if (opcode == OP_AND)
                        {
                            for (uint32_t i = 0; i < vch1.size(); i++)
                                vch1[i] &= vch2[i];
                        }
                        else if (opcode == OP_OR)
                        {
                            for (uint32_t i = 0; i < vch1.size(); i++)
                                vch1[i] |= vch2[i];
                        }
                        else if (opcode == OP_XOR)
                        {
                            for (uint32_t i = 0; i < vch1.size(); i++)
                                vch1[i] ^= vch2[i];
                        }
                        popstack(stack);
                    }
                    break;

                    case OP_EQUAL:
                    case OP_EQUALVERIFY:
                    //case OP_NOTEQUAL: // use OP_NUMNOTEQUAL
                    {
                        // (x1 x2 - bool)
                        if (stack.size() < 2)
                            return false;
                        std::vector<uint8_t>& vch1 = stacktop(-2);
                        std::vector<uint8_t>& vch2 = stacktop(-1);
                        bool fEqual = (vch1 == vch2);
                        // OP_NOTEQUAL is disabled because it would be too easy to say
                        // something like n != 1 and have some wiseguy pass in 1 with extra
                        // zero bytes after it (numerically, 0x01 == 0x0001 == 0x000001)
                        //if (opcode == OP_NOTEQUAL)
                        //    fEqual = !fEqual;
                        popstack(stack);
                        popstack(stack);
                        stack.push_back(fEqual ? vchTrue : vchFalse);
                        if (opcode == OP_EQUALVERIFY)
                        {
                            if (fEqual)
                                popstack(stack);
                            else
                                return false;
                        }
                    }
                    break;


                    //
                    // Numeric
                    //
                    case OP_1ADD:
                    case OP_1SUB:
                    case OP_2MUL:
                    case OP_2DIV:
                    case OP_NEGATE:
                    case OP_ABS:
                    case OP_NOT:
                    case OP_0NOTEQUAL:
                    {
                        // (in -- out)
                        if (stack.size() < 1)
                            return false;
                        CBigNum bn = CastToBigNum(stacktop(-1));
                        switch (opcode)
                        {
                        case OP_1ADD:       bn += bnOne; break;
                        case OP_1SUB:       bn -= bnOne; break;
                        case OP_2MUL:       bn <<= 1; break;
                        case OP_2DIV:       bn >>= 1; break;
                        case OP_NEGATE:     bn = -bn; break;
                        case OP_ABS:        if (bn < bnZero) bn = -bn; break;
                        case OP_NOT:        bn = (bn == bnZero); break;
                        case OP_0NOTEQUAL:  bn = (bn != bnZero); break;
                        default:            assert(!"invalid opcode"); break;
                        }
                        popstack(stack);
                        stack.push_back(bn.getvch());
                    }
                    break;

                    case OP_ADD:
                    case OP_SUB:
                    case OP_MUL:
                    case OP_DIV:
                    case OP_MOD:
                    case OP_LSHIFT:
                    case OP_RSHIFT:
                    case OP_BOOLAND:
                    case OP_BOOLOR:
                    case OP_NUMEQUAL:
                    case OP_NUMEQUALVERIFY:
                    case OP_NUMNOTEQUAL:
                    case OP_LESSTHAN:
                    case OP_GREATERTHAN:
                    case OP_LESSTHANOREQUAL:
                    case OP_GREATERTHANOREQUAL:
                    case OP_MIN:
                    case OP_MAX:
                    {
                        // (x1 x2 -- out)
                        if (stack.size() < 2)
                            return false;
                        CBigNum bn1 = CastToBigNum(stacktop(-2));
                        CBigNum bn2 = CastToBigNum(stacktop(-1));
                        CBigNum bn;
                        switch (opcode)
                        {
                        case OP_ADD:
                            bn = bn1 + bn2;
                            break;

                        case OP_SUB:
                            bn = bn1 - bn2;
                            break;

                        case OP_MUL:
                            if (!BN_mul(bn.getBN(), bn1.getBN(), bn2.getBN(), pctx))
                                return false;
                            break;

                        case OP_DIV:
                            if (!BN_div(bn.getBN(), NULL, bn1.getBN(), bn2.getBN(), pctx))
                                return false;
                            break;

                        case OP_MOD:
                            if (!BN_mod(bn.getBN(), bn1.getBN(), bn2.getBN(), pctx))
                                return false;
                            break;

                        case OP_LSHIFT:
                            if (bn2 < bnZero || bn2 > CBigNum(2048))
                                return false;
                            bn = bn1 << bn2.getulong();
                            break;

                        case OP_RSHIFT:
                            if (bn2 < bnZero || bn2 > CBigNum(2048))
                                return false;
                            bn = bn1 >> bn2.getulong();
                            break;

                        case OP_BOOLAND:             bn = (bn1 != bnZero && bn2 != bnZero); break;
                        case OP_BOOLOR:              bn = (bn1 != bnZero || bn2 != bnZero); break;
                        case OP_NUMEQUAL:            bn = (bn1 == bn2); break;
                        case OP_NUMEQUALVERIFY:      bn = (bn1 == bn2); break;
                        case OP_NUMNOTEQUAL:         bn = (bn1 != bn2); break;
                        case OP_LESSTHAN:            bn = (bn1 < bn2); break;
                        case OP_GREATERTHAN:         bn = (bn1 > bn2); break;
                        case OP_LESSTHANOREQUAL:     bn = (bn1 <= bn2); break;
                        case OP_GREATERTHANOREQUAL:  bn = (bn1 >= bn2); break;
                        case OP_MIN:                 bn = (bn1 < bn2 ? bn1 : bn2); break;
                        case OP_MAX:                 bn = (bn1 > bn2 ? bn1 : bn2); break;
                        default:                     assert(!"invalid opcode"); break;
                        }
                        popstack(stack);
                        popstack(stack);
                        stack.push_back(bn.getvch());

                        if (opcode == OP_NUMEQUALVERIFY)
                        {
                            if (CastToBool(stacktop(-1)))
                                popstack(stack);
                            else
                                return false;
                        }
                    }
                    break;

                    case OP_WITHIN:
                    {
                        // (x min max -- out)
                        if (stack.size() < 3)
                            return false;
                        CBigNum bn1 = CastToBigNum(stacktop(-3));
                        CBigNum bn2 = CastToBigNum(stacktop(-2));
                        CBigNum bn3 = CastToBigNum(stacktop(-1));
                        bool fValue = (bn2 <= bn1 && bn1 < bn3);
                        popstack(stack);
                        popstack(stack);
                        popstack(stack);
                        stack.push_back(fValue ? vchTrue : vchFalse);
                    }
                    break;


                    //
                    // Crypto
                    //
                    case OP_HASH256:
                    {
                        // (in -- hash)
                        if (stack.size() < 1)
                            return false;
                        std::vector<uint8_t>& vch = stacktop(-1);
                        std::vector<uint8_t> vchHash(32);

                        uint256_t hash256 = SK256(vch);
                        memcpy(&vchHash[0], &hash256, sizeof(hash256));

                        popstack(stack);
                        stack.push_back(vchHash);
                    }
                    break;

                    case OP_CODESEPARATOR:
                    {
                        // Hash starts after the code separator
                        pbegincodehash = pc;
                    }
                    break;

                    case OP_CHECKSIG:
                    case OP_CHECKSIGVERIFY:
                    {
                        // (sig pubkey -- bool)
                        if (stack.size() < 2)
                            return false;

                        std::vector<uint8_t>& vchSig    = stacktop(-2);
                        std::vector<uint8_t>& vchPubKey = stacktop(-1);

                        ////// debug print
                        //PrintHex(vchSig.begin(), vchSig.end(), "sig: %s\n");
                        //PrintHex(vchPubKey.begin(), vchPubKey.end(), "pubkey: %s\n");

                        // Subset of script starting at the most recent codeseparator
                        CScript scriptCode(pbegincodehash, pend);

                        // Drop the signature, since there's no way for a signature to sign itself
                        scriptCode.FindAndDelete(CScript(vchSig));

                        bool fSuccess = CheckSig(vchSig, vchPubKey, scriptCode, txTo, nIn, nHashType);
                        popstack(stack);
                        popstack(stack);
                        stack.push_back(fSuccess ? vchTrue : vchFalse);
                        if (opcode == OP_CHECKSIGVERIFY)
                        {
                            if (fSuccess)
                                popstack(stack);
                            else
                                return false;
                        }
                    }
                    break;

                    case OP_CHECKMULTISIG:
                    case OP_CHECKMULTISIGVERIFY:
                    {
                        // ([sig ...] num_of_signatures [pubkey ...] num_of_pubkeys -- bool)

                        int i = 1;
                        if (stack.size() < i)
                            return false;

                        int nKeysCount = CastToBigNum(stacktop(-i)).getint();
                        if (nKeysCount < 0 || nKeysCount > 20)
                            return false;
                        nOpCount += nKeysCount;
                        if (nOpCount > 201)
                            return false;
                        int ikey = ++i;
                        i += nKeysCount;
                        if (stack.size() < i)
                            return false;

                        int nSigsCount = CastToBigNum(stacktop(-i)).getint();
                        if (nSigsCount < 0 || nSigsCount > nKeysCount)
                            return false;
                        int isig = ++i;
                        i += nSigsCount;
                        if (stack.size() < i)
                            return false;

                        // Subset of script starting at the most recent codeseparator
                        CScript scriptCode(pbegincodehash, pend);

                        // Drop the signatures, since there's no way for a signature to sign itself
                        for (int k = 0; k < nSigsCount; k++)
                        {
                            std::vector<uint8_t>& vchSig = stacktop(-isig-k);
                            scriptCode.FindAndDelete(CScript(vchSig));
                        }

                        bool fSuccess = true;
                        while (fSuccess && nSigsCount > 0)
                        {
                            std::vector<uint8_t>& vchSig    = stacktop(-isig);
                            std::vector<uint8_t>& vchPubKey = stacktop(-ikey);

                            // Check signature
                            if (CheckSig(vchSig, vchPubKey, scriptCode, txTo, nIn, nHashType))
                            {
                                isig++;
                                nSigsCount--;
                            }
                            ikey++;
                            nKeysCount--;

                            // If there are more signatures left than keys left,
                            // then too many signatures have failed
                            if (nSigsCount > nKeysCount)
                                fSuccess = false;
                        }

                        while (i-- > 0)
                            popstack(stack);
                        stack.push_back(fSuccess ? vchTrue : vchFalse);

                        if (opcode == OP_CHECKMULTISIGVERIFY)
                        {
                            if (fSuccess)
                                popstack(stack);
                            else
                                return false;
                        }
                    }
                    break;

                    default:
                        return false;
                }

                // Size limits
                if (stack.size() + altstack.size() > 1000)
                    return false;
            }
        }
        catch (...)
        {
            return false;
        }


        if (!vfExec.empty())
            return false;

        return true;
    }









    uint256_t SignatureHash(CScript scriptCode, const Core::CTransaction& txTo, uint32_t nIn, int nHashType)
    {
        if (nIn >= txTo.vin.size())
        {
            printf("ERROR: SignatureHash() : nIn=%d out of range\n", nIn);
            return 1;
        }

        Core::CTransaction txTmp(txTo);

        // In case concatenating two scripts ends up with two codeseparators,
        // or an extra one at the end, this prevents all those possible incompatibilities.
        scriptCode.FindAndDelete(CScript(OP_CODESEPARATOR));

        // Blank out other inputs' signatures
        for (uint32_t i = 0; i < txTmp.vin.size(); i++)
            txTmp.vin[i].scriptSig = CScript();
        txTmp.vin[nIn].scriptSig = scriptCode;

        // Blank out some of the outputs
        if ((nHashType & 0x1f) == SIGHASH_NONE)
        {
            // Wildcard payee
            txTmp.vout.clear();

            // Let the others update at will
            for (uint32_t i = 0; i < txTmp.vin.size(); i++)
                if (i != nIn)
                    txTmp.vin[i].nSequence = 0;
        }
        else if ((nHashType & 0x1f) == SIGHASH_SINGLE)
        {
            // Only lockin the txout payee at same index as txin
            uint32_t nOut = nIn;
            if (nOut >= txTmp.vout.size())
            {
                printf("ERROR: SignatureHash() : nOut=%d out of range\n", nOut);
                return 1;
            }
            txTmp.vout.resize(nOut+1);
            for (uint32_t i = 0; i < nOut; i++)
                txTmp.vout[i].SetNull();

            // Let the others update at will
            for (uint32_t i = 0; i < txTmp.vin.size(); i++)
                if (i != nIn)
                    txTmp.vin[i].nSequence = 0;
        }

        // Blank out other inputs completely, not recommended for open transactions
        if (nHashType & SIGHASH_ANYONECANPAY)
        {
            txTmp.vin[0] = txTmp.vin[nIn];
            txTmp.vin.resize(1);
        }

        // Serialize and hash
        CDataStream ss(SER_GETHASH, 0);
        ss.reserve(10000);
        ss << txTmp << nHashType;
        return SK256(ss.begin(), ss.end());
    }


    // Valid signature cache, to avoid doing expensive ECDSA signature checking
    // twice for every transaction (once when accepted into memory pool, and
    // again when accepted into the block chain)

    class CSignatureCache
    {
    private:
        // sigdata_type is (signature hash, signature, public key):
        typedef boost::tuple<uint256_t, std::vector<uint8_t>, std::vector<uint8_t> > sigdata_type;
        std::set< sigdata_type> setValid;
        CCriticalSection cs_sigcache;

    public:
        bool
        Get(uint256_t hash, const std::vector<uint8_t>& vchSig, const std::vector<uint8_t>& pubKey)
        {
            LOCK(cs_sigcache);

            sigdata_type k(hash, vchSig, pubKey);
            std::set<sigdata_type>::iterator mi = setValid.find(k);
            if (mi != setValid.end())
                return true;
            return false;
        }

        void
        Set(uint256_t hash, const std::vector<uint8_t>& vchSig, const std::vector<uint8_t>& pubKey)
        {
            // DoS prevention: limit cache size to less than 10MB
            // (~200 bytes per cache entry times 50,000 entries)
            // Since there are a maximum of 20,000 signature operations per block
            // 50,000 is a reasonable default.
            int64_t nMaxCacheSize = GetArg("-maxsigcachesize", 50000);
            if (nMaxCacheSize <= 0) return;

            LOCK(cs_sigcache);

            while (static_cast<int64_t>(setValid.size()) > nMaxCacheSize)
            {
                // Evict a random entry. Random because that helps
                // foil would-be DoS attackers who might try to pre-generate
                // and re-use a set of valid signatures just-slightly-greater
                // than our cache size.
                uint256_t randomHash = GetRand256();
                std::vector<uint8_t> unused;
                std::set<sigdata_type>::iterator it =
                    setValid.lower_bound(sigdata_type(randomHash, unused, unused));
                if (it == setValid.end())
                    it = setValid.begin();
                setValid.erase(*it);
            }

            sigdata_type k(hash, vchSig, pubKey);
            setValid.insert(k);
        }
    };

    bool CheckSig(vector<uint8_t> vchSig, vector<uint8_t> vchPubKey, CScript scriptCode,
                const Core::CTransaction& txTo, uint32_t nIn, int nHashType)
    {
        static CSignatureCache signatureCache;

        // Hash type is one byte tacked on to the end of the signature
        if (vchSig.empty())
            return false;
        if (nHashType == 0)
            nHashType = vchSig.back();
        else if (nHashType != vchSig.back())
            return false;
        vchSig.pop_back();

        uint256_t sighash = SignatureHash(scriptCode, txTo, nIn, nHashType);

        if (signatureCache.Get(sighash, vchSig, vchPubKey))
            return true;

        CKey key;
        if (!key.SetPubKey(vchPubKey))
            return false;

        if (!key.Verify(sighash, vchSig, 256))
            return false;

        signatureCache.Set(sighash, vchSig, vchPubKey);
        return true;
    }









    //
    // Return public keys or hashes from scriptPubKey, for 'standard' transaction types.
    //
    bool Solver(const CScript& scriptPubKey, TransactionType& typeRet, vector<vector<uint8_t> >& vSolutionsRet)
    {
        // Templates
        static map<TransactionType, CScript> mTemplates;
        if (mTemplates.empty())
        {
            // Standard tx, sender provides pubkey, receiver adds signature
            mTemplates.insert(make_pair(TX_PUBKEY, CScript() << OP_PUBKEY << OP_CHECKSIG));

            // Nexus address tx, sender provides hash of pubkey, receiver provides signature and pubkey
            mTemplates.insert(make_pair(TX_PUBKEYHASH, CScript() << OP_DUP << OP_HASH256 << OP_PUBKEYHASH << OP_EQUALVERIFY << OP_CHECKSIG));

            // Sender provides N pubkeys, receivers provides M signatures
            mTemplates.insert(make_pair(TX_MULTISIG, CScript() << OP_SMALLINTEGER << OP_PUBKEYS << OP_SMALLINTEGER << OP_CHECKMULTISIG));
        }

        // Shortcut for pay-to-script-hash, which are more constrained than the other types:
        // it is always OP_HASH256 20 [20 byte hash] OP_EQUAL
        if (scriptPubKey.IsPayToScriptHash())
        {
            typeRet = TX_SCRIPTHASH;
            vector<uint8_t> hashBytes(scriptPubKey.begin()+2, scriptPubKey.begin()+22);
            vSolutionsRet.push_back(hashBytes);
            return true;
        }

        // Scan templates
        const CScript& script1 = scriptPubKey;
        BOOST_FOREACH(const PAIRTYPE(TransactionType, CScript)& tplate, mTemplates)
        {
            const CScript& script2 = tplate.second;
            vSolutionsRet.clear();

            opcodetype opcode1, opcode2;
            vector<uint8_t> vch1, vch2;

            // Compare
            CScript::const_iterator pc1 = script1.begin();
            CScript::const_iterator pc2 = script2.begin();
            loop() {
                if (pc1 == script1.end() && pc2 == script2.end())
                {
                    // Found a match
                    typeRet = tplate.first;
                    if (typeRet == TX_MULTISIG)
                    {
                        // Additional checks for TX_MULTISIG:
                        uint8_t m = vSolutionsRet.front()[0];
                        uint8_t n = vSolutionsRet.back()[0];
                        if (m < 1 || n < 1 || m > n || vSolutionsRet.size()-2 != n)
                            return false;
                    }
                    return true;
                }
                if (!script1.GetOp(pc1, opcode1, vch1))
                    break;
                if (!script2.GetOp(pc2, opcode2, vch2))
                    break;

                // Template matching opcodes:
                if (opcode2 == OP_PUBKEYS)
                {
                    while (vch1.size() >= 33 && vch1.size() <= 120)
                    {
                        vSolutionsRet.push_back(vch1);
                        if (!script1.GetOp(pc1, opcode1, vch1))
                            break;
                    }
                    if (!script2.GetOp(pc2, opcode2, vch2))
                        break;
                    // Normal situation is to fall through
                    // to other if/else statments
                }

                if (opcode2 == OP_PUBKEY)
                {
                    if (vch1.size() < 33 || vch1.size() > 120)
                        break;
                    vSolutionsRet.push_back(vch1);
                }
                else if (opcode2 == OP_PUBKEYHASH)
                {
                    if (vch1.size() != sizeof(uint256_t))
                        break;
                    vSolutionsRet.push_back(vch1);
                }
                else if (opcode2 == OP_SMALLINTEGER)
                {   // Single-byte small integer pushed onto vSolutions
                    if (opcode1 == OP_0 ||
                        (opcode1 >= OP_1 && opcode1 <= OP_16))
                    {
                        char n = (char)CScript::DecodeOP_N(opcode1);
                        vSolutionsRet.push_back(std::vector<uint8_t>(1, n));
                    }
                    else
                        break;
                }
                else if (opcode1 != opcode2 || vch1 != vch2)
                {
                    // Others must match exactly
                    break;
                }
            }
        }

        vSolutionsRet.clear();
        typeRet = TX_NONSTANDARD;
        return false;
    }


    bool Sign1(const NexusAddress& address, const CKeyStore& keystore, uint256_t hash, int nHashType, CScript& scriptSigRet)
    {
        CKey key;
        if (!keystore.GetKey(address, key))
            return false;

        vector<uint8_t> vchSig;
        if (!key.Sign(hash, vchSig, 256))
            return false;
        vchSig.push_back((uint8_t)nHashType);
        scriptSigRet << vchSig;

        return true;
    }

    bool SignN(const vector<std::vector<uint8_t> >& multisigdata, const CKeyStore& keystore, uint256_t hash, int nHashType, CScript& scriptSigRet)
    {
        int nSigned = 0;
        int nRequired = multisigdata.front()[0];
        for (vector<std::vector<uint8_t> >::const_iterator it = multisigdata.begin()+1; it != multisigdata.begin()+multisigdata.size()-1; it++)
        {
            const std::vector<uint8_t>& pubkey = *it;
            NexusAddress address;
            address.SetPubKey(pubkey);
            if (Sign1(address, keystore, hash, nHashType, scriptSigRet))
            {
                ++nSigned;
                if (nSigned == nRequired) break;
            }
        }
        return nSigned==nRequired;
    }

    //
    // Sign scriptPubKey with private keys stored in keystore, given transaction hash and hash type.
    // Signatures are returned in scriptSigRet (or returns false if scriptPubKey can't be signed),
    // unless whichTypeRet is TX_SCRIPTHASH, in which case scriptSigRet is the redemption script.
    // Returns false if scriptPubKey could not be completely satisified.
    //
    bool Solver(const CKeyStore& keystore, const CScript& scriptPubKey, uint256_t hash, int nHashType,
                    CScript& scriptSigRet, TransactionType& whichTypeRet)
    {
        scriptSigRet.clear();

        vector<std::vector<uint8_t> > vSolutions;
        if (!Solver(scriptPubKey, whichTypeRet, vSolutions))
            return false;

        NexusAddress address;
        switch (whichTypeRet)
        {
        case TX_NONSTANDARD:
            return false;
        case TX_PUBKEY:
            address.SetPubKey(vSolutions[0]);
            return Sign1(address, keystore, hash, nHashType, scriptSigRet);
        case TX_PUBKEYHASH:
            address.SetHash256(uint256_t(vSolutions[0]));
            if (!Sign1(address, keystore, hash, nHashType, scriptSigRet))
                return false;
            else
            {
                std::vector<uint8_t> vch;
                keystore.GetPubKey(address, vch);
                scriptSigRet << vch;
            }
            return true;
        case TX_SCRIPTHASH:
            return keystore.GetCScript(uint256_t(vSolutions[0]), scriptSigRet);

        case TX_MULTISIG:
            scriptSigRet << OP_0; // workaround CHECKMULTISIG bug
            return (SignN(vSolutions, keystore, hash, nHashType, scriptSigRet));
        }
        return false;
    }

    int ScriptSigArgsExpected(TransactionType t, const std::vector<std::vector<uint8_t> >& vSolutions)
    {
        switch (t)
        {
        case TX_NONSTANDARD:
            return -1;
        case TX_PUBKEY:
            return 1;
        case TX_PUBKEYHASH:
            return 2;
        case TX_MULTISIG:
            if (vSolutions.size() < 1 || vSolutions[0].size() < 1)
                return -1;
            return vSolutions[0][0] + 1;
        case TX_SCRIPTHASH:
            return 1; // doesn't include args needed by the script
        }
        return -1;
    }

    bool IsStandard(const CScript& scriptPubKey)
    {
        vector<std::vector<uint8_t> > vSolutions;
        TransactionType whichType;
        if (!Solver(scriptPubKey, whichType, vSolutions))
            return false;

        if (whichType == TX_MULTISIG)
        {
            uint8_t m = vSolutions.front()[0];
            uint8_t n = vSolutions.back()[0];
            // Support up to x-of-3 multisig txns as standard
            if (n < 1 || n > 3)
                return false;
            if (m < 1 || m > n)
                return false;
        }

        return whichType != TX_NONSTANDARD;
    }


    uint32_t HaveKeys(const vector<std::vector<uint8_t> >& pubkeys, const CKeyStore& keystore)
    {
        uint32_t nResult = 0;
        BOOST_FOREACH(const std::vector<uint8_t>& pubkey, pubkeys)
        {
            NexusAddress address;
            address.SetPubKey(pubkey);
            if (keystore.HaveKey(address))
                ++nResult;
        }
        return nResult;
    }

    bool IsMine(const CKeyStore &keystore, const CScript& scriptPubKey)
    {
        vector<std::vector<uint8_t> > vSolutions;
        TransactionType whichType;
        if (!Solver(scriptPubKey, whichType, vSolutions))
            return false;

        NexusAddress address;
        switch (whichType)
        {
        case TX_NONSTANDARD:
            return false;
        case TX_PUBKEY:
            address.SetPubKey(vSolutions[0]);
            return keystore.HaveKey(address);
        case TX_PUBKEYHASH:
            address.SetHash256(uint256_t(vSolutions[0]));
            return keystore.HaveKey(address);
        case TX_SCRIPTHASH:
        {
            CScript subscript;
            if (!keystore.GetCScript(uint256_t(vSolutions[0]), subscript))
                return false;

            return IsMine(keystore, subscript);
        }
        case TX_MULTISIG:
        {
            // Only consider transactions "mine" if we own ALL the
            // keys involved. multi-signature transactions that are
            // partially owned (somebody else has a key that can spend
            // them) enable spend-out-from-under-you attacks, especially
            // in shared-wallet situations.
            vector<std::vector<uint8_t> > keys(vSolutions.begin()+1, vSolutions.begin()+vSolutions.size()-1);
            return HaveKeys(keys, keystore) == keys.size();
        }
        }
        return false;
    }

    bool ExtractAddress(const CScript& scriptPubKey, NexusAddress& addressRet)
    {
        vector<std::vector<uint8_t> > vSolutions;
        TransactionType whichType;
        if (!Solver(scriptPubKey, whichType, vSolutions))
            return false;

        if (whichType == TX_PUBKEY)
        {
            addressRet.SetPubKey(vSolutions[0]);
            return true;
        }
        else if (whichType == TX_PUBKEYHASH)
        {
            addressRet.SetHash256(uint256_t(vSolutions[0]));
            return true;
        }
        else if (whichType == TX_SCRIPTHASH)
        {
            addressRet.SetScriptHash256(uint256_t(vSolutions[0]));
            return true;
        }
        // Multisig txns have more than one address...
        return false;
    }

    bool ExtractAddresses(const CScript& scriptPubKey, TransactionType& typeRet, vector<NexusAddress>& addressRet, int& nRequiredRet)
    {
        addressRet.clear();
        typeRet = TX_NONSTANDARD;
        vector<std::vector<uint8_t> > vSolutions;
        if (!Solver(scriptPubKey, typeRet, vSolutions))
            return false;

        if (typeRet == TX_MULTISIG)
        {
            nRequiredRet = vSolutions.front()[0];
            for (uint32_t i = 1; i < vSolutions.size()-1; i++)
            {
                NexusAddress address;
                address.SetPubKey(vSolutions[i]);
                addressRet.push_back(address);
            }
        }
        else
        {
            nRequiredRet = 1;
            NexusAddress address;
            if (typeRet == TX_PUBKEYHASH)
                address.SetHash256(uint256_t(vSolutions.front()));
            else if (typeRet == TX_SCRIPTHASH)
                address.SetScriptHash256(uint256_t(vSolutions.front()));
            else if (typeRet == TX_PUBKEY)
                address.SetPubKey(vSolutions.front());
            addressRet.push_back(address);
        }

        return true;
    }

    bool VerifyScript(const CScript& scriptSig, const CScript& scriptPubKey, const Core::CTransaction& txTo, uint32_t nIn, int nHashType)
    {
        vector<vector<uint8_t> > stack, stackCopy;
        if (!EvalScript(stack, scriptSig, txTo, nIn, nHashType))
            return false;

        stackCopy = stack;
        if (!EvalScript(stack, scriptPubKey, txTo, nIn, nHashType))
            return false;

        if (stack.empty())
            return false;

        if (CastToBool(stack.back()) == false)
            return false;

        // Additional validation for spend-to-script-hash transactions:
        if (scriptPubKey.IsPayToScriptHash())
        {
            if (!scriptSig.IsPushOnly()) // scriptSig must be literals-only
                return false;

            const std::vector<uint8_t>& pubKeySerialized = stackCopy.back();
            CScript pubKey2(pubKeySerialized.begin(), pubKeySerialized.end());
            popstack(stackCopy);

            if (!EvalScript(stackCopy, pubKey2, txTo, nIn, nHashType))
                return false;

            if (stackCopy.empty())
                return false;

            return CastToBool(stackCopy.back());
        }

        return true;
    }


    bool SignSignature(const CKeyStore &keystore, const Core::CTransaction& txFrom, Core::CTransaction& txTo, uint32_t nIn, int nHashType)
    {
        assert(nIn < txTo.vin.size());
        Core::CTxIn& txin = txTo.vin[nIn];
        assert(txin.prevout.n < txFrom.vout.size());
        assert(txin.prevout.hash == txFrom.GetHash());
        const Core::CTxOut& txout = txFrom.vout[txin.prevout.n];

        // Leave out the signature from the hash, since a signature can't sign itself.
        // The checksig op will also drop the signatures from its hash.
        uint256_t hash = SignatureHash(txout.scriptPubKey, txTo, nIn, nHashType);

        TransactionType whichType;
        if (!Solver(keystore, txout.scriptPubKey, hash, nHashType, txin.scriptSig, whichType))
            return false;

        if (whichType == TX_SCRIPTHASH)
        {
            // Solver returns the subscript that need to be evaluated;
            // the final scriptSig is the signatures from that
            // and then the serialized subscript:
            CScript subscript = txin.scriptSig;

            // Recompute txn hash using subscript in place of scriptPubKey:
            uint256_t hash2 = SignatureHash(subscript, txTo, nIn, nHashType);
            TransactionType subType;
            if (!Solver(keystore, subscript, hash2, nHashType, txin.scriptSig, subType))
                return false;
            if (subType == TX_SCRIPTHASH)
                return false;
            txin.scriptSig << static_cast<std::vector<uint8_t> >(subscript); // Append serialized subscript
        }

        // Test solution
        if (!VerifyScript(txin.scriptSig, txout.scriptPubKey, txTo, nIn, 0))
            return false;

        return true;
    }


    bool VerifySignature(const Core::CTransaction& txFrom, const Core::CTransaction& txTo, uint32_t nIn, int nHashType)
    {
        assert(nIn < txTo.vin.size());
        const Core::CTxIn& txin = txTo.vin[nIn];
        if (txin.prevout.n >= txFrom.vout.size())
            return false;
        const Core::CTxOut& txout = txFrom.vout[txin.prevout.n];

        if (txin.prevout.hash != txFrom.GetHash())
            return false;

        if (!VerifyScript(txin.scriptSig, txout.scriptPubKey, txTo, nIn, nHashType))
            return false;

        return true;
    }

    uint32_t CScript::GetSigOpCount(bool fAccurate) const
    {
        uint32_t n = 0;
        const_iterator pc = begin();
        opcodetype lastOpcode = OP_INVALIDOPCODE;
        while (pc < end())
        {
            opcodetype opcode;
            if (!GetOp(pc, opcode))
                break;
            if (opcode == OP_CHECKSIG || opcode == OP_CHECKSIGVERIFY)
                n++;
            else if (opcode == OP_CHECKMULTISIG || opcode == OP_CHECKMULTISIGVERIFY)
            {
                if (fAccurate && lastOpcode >= OP_1 && lastOpcode <= OP_16)
                    n += DecodeOP_N(lastOpcode);
                else
                    n += 20;
            }

            lastOpcode = opcode;
        }
        return n;
    }

    uint32_t CScript::GetSigOpCount(const CScript& scriptSig) const
    {
        if (!IsPayToScriptHash())
            return GetSigOpCount(true);

        // This is a pay-to-script-hash scriptPubKey;
        // get the last item that the scriptSig
        // pushes onto the stack:
        const_iterator pc = scriptSig.begin();
        vector<uint8_t> data;
        while (pc < scriptSig.end())
        {
            opcodetype opcode;
            if (!scriptSig.GetOp(pc, opcode, data))
                return 0;
            if (opcode > OP_16)
                return 0;
        }

        /// ... and return it's opcount:
        CScript subscript(data.begin(), data.end());
        return subscript.GetSigOpCount(true);
    }

    bool CScript::IsPayToScriptHash() const
    {
        // Extra-fast test for pay-to-script-hash CScripts:
        return (this->size() == 23 &&
                this->at(0) == OP_HASH256 &&
                this->at(1) == 0x14 &&
                this->at(22) == OP_EQUAL);
    }

    void CScript::SetNexusAddress(const NexusAddress& address)
    {
        this->clear();
        if (address.IsScript())
            *this << OP_HASH256 << address.GetHash256() << OP_EQUAL;
        else
            *this << OP_DUP << OP_HASH256 << address.GetHash256() << OP_EQUALVERIFY << OP_CHECKSIG;
    }

    void CScript::SetMultisig(int nRequired, const std::vector<CKey>& keys)
    {
        this->clear();

        *this << EncodeOP_N(nRequired);
        BOOST_FOREACH(const CKey& key, keys)
            *this << key.GetPubKey();
        *this << EncodeOP_N(keys.size()) << OP_CHECKMULTISIG;
    }

    void CScript::SetPayToScriptHash(const CScript& subscript)
    {
        assert(!subscript.empty());
        uint256_t subscriptHash = SK256(subscript);
        this->clear();
        *this << OP_HASH256 << subscriptHash << OP_EQUAL;
    }

}
