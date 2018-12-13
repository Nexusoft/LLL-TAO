/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/types/bignum.h>
#include <LLC/hash/SK.h>
#include <Util/include/base58.h>

#include <TAO/Legacy/include/constants.h>
#include <TAO/Legacy/include/evaluate.h>
#include <TAO/Legacy/include/signature.h>

#include <TAO/Legacy/types/enum.h>
#include <TAO/Legacy/types/script.h>

#include <string>
#include <vector>

#include <stdexcept>

namespace Legacy
{

    //stack preprocessors
    #define stacktop(i)  (stack.at(stack.size()+(i)))
    #define altstacktop(i)  (altstack.at(altstack.size()+(i)))


    /**
     *
     * Script is a stack machine (like Forth) that evaluates a predicate
     * returning a bool indicating valid or not.  There are no loops.
     *
     **/
    static inline void popstack(std::vector< std::vector<uint8_t> >& stack)
    {
        if (stack.empty())
            throw std::runtime_error("popstack() : stack empty");

        stack.pop_back();
    }


    /** Conversion function to bignum value. **/
    LLC::CBigNum CastToBigNum(const std::vector<uint8_t>& vch)
    {
        if (vch.size() > nMaxNumSize)
            throw std::runtime_error("CastToBigNum() : overflow");

        // Get rid of extra leading zeros
        return LLC::CBigNum(LLC::CBigNum(vch).getvch());
    }

    /** Set two vectors to be of the same size. **/
    void MakeSameSize(std::vector<unsigned char>& vch1, std::vector<unsigned char>& vch2)
    {
        // Lengthen the shorter one
        if (vch1.size() < vch2.size())
            vch1.resize(vch2.size(), 0);
        if (vch2.size() < vch1.size())
            vch2.resize(vch1.size(), 0);
    }


    /** Conversion function to boolean value. **/
    bool CastToBool(const std::vector<uint8_t>& vch)
    {
        for (unsigned int i = 0; i < vch.size(); i++)
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


    /* Evaluate a script to true or false based on operation codes. */
    bool EvalScript(std::vector<std::vector<uint8_t> >& stack, const CScript& script, const Transaction& txTo, uint32_t nIn, int32_t nHashType)
    {
        LLC::CAutoBN_CTX pctx;
        CScript::const_iterator pc = script.begin();
        CScript::const_iterator pend = script.end();
        CScript::const_iterator pbegincodehash = script.begin();
        opcodetype opcode;
        std::vector<uint8_t> vchPushValue;
        std::vector<bool> vfExec;
        std::vector<std::vector<uint8_t> > altstack;
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
                        LLC::CBigNum bn((int)opcode - (int)(OP_1 - 1));
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
                        LLC::CBigNum bn((uint32_t)stack.size());
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
                        LLC::CBigNum bn((uint32_t)stacktop(-1).size());
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
                        for (unsigned int i = 0; i < vch.size(); i++)
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
                            for (unsigned int i = 0; i < vch1.size(); i++)
                                vch1[i] &= vch2[i];
                        }
                        else if (opcode == OP_OR)
                        {
                            for (unsigned int i = 0; i < vch1.size(); i++)
                                vch1[i] |= vch2[i];
                        }
                        else if (opcode == OP_XOR)
                        {
                            for (unsigned int i = 0; i < vch1.size(); i++)
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
                        LLC::CBigNum bn = CastToBigNum(stacktop(-1));
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
                        LLC::CBigNum bn1 = CastToBigNum(stacktop(-2));
                        LLC::CBigNum bn2 = CastToBigNum(stacktop(-1));
                        LLC::CBigNum bn;
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
                            if (!BN_div(bn.getBN(), nullptr, bn1.getBN(), bn2.getBN(), pctx))
                                return false;
                            break;

                        case OP_MOD:
                            if (!BN_mod(bn.getBN(), bn1.getBN(), bn2.getBN(), pctx))
                                return false;
                            break;

                        case OP_LSHIFT:
                            if (bn2 < bnZero || bn2 > LLC::CBigNum(2048))
                                return false;
                            bn = bn1 << bn2.getulong();
                            break;

                        case OP_RSHIFT:
                            if (bn2 < bnZero || bn2 > LLC::CBigNum(2048))
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
                        LLC::CBigNum bn1 = CastToBigNum(stacktop(-3));
                        LLC::CBigNum bn2 = CastToBigNum(stacktop(-2));
                        LLC::CBigNum bn3 = CastToBigNum(stacktop(-1));
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

                        uint256_t hash256 = LLC::SK256(vch);
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


    /* Extract data from a script object. */
    bool Solver(const CScript& scriptPubKey, TransactionType& typeRet, std::vector< std::vector<uint8_t> >& vSolutionsRet)
    {
        // Templates
        static std::map<TransactionType, CScript> mTemplates;
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
            std::vector<uint8_t> hashBytes(scriptPubKey.begin() + 2, scriptPubKey.begin() + 22);
            vSolutionsRet.push_back(hashBytes);
            return true;
        }

        // Scan templates
        const CScript& script1 = scriptPubKey;
        for(auto tplate : mTemplates)
        {
            const CScript& script2 = tplate.second;
            vSolutionsRet.clear();

            opcodetype opcode1, opcode2;
            std::vector<uint8_t> vch1, vch2;

            // Compare
            CScript::const_iterator pc1 = script1.begin();
            CScript::const_iterator pc2 = script2.begin();
            while(true)
            {
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
                        char n = (int8_t)scriptPubKey.DecodeOP_N(opcode1);
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


    /* Sign scriptPubKey with private keys, given transaction hash and hash type. */
    bool Solver(const CKeyStore& keystore, const CScript& scriptPubKey, uint256_t hash, int32_t nHashType, CScript& scriptSigRet, TransactionType& whichTypeRet)
    {
        scriptSigRet.clear();

        std::vector< std::vector<uint8_t> > vSolutions;
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


    /* Used in standard inputs function check in transaction. potential to remove */
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


    /* Detects if a script object is of a standard type. */
    bool IsStandard(const CScript& scriptPubKey)
    {
        std::vector< std::vector<uint8_t> > vSolutions;
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


    /* Determines if given list of public keys exist in the keystore object. */
    uint32_t HaveKeys(const std::vector< std::vector<uint8_t> >& pubkeys, const CKeyStore& keystore)
    {
        uint32_t nResult = 0;
        for(auto pubkey : pubkeys)
        {
            NexusAddress address;
            address.SetPubKey(pubkey);
            if (keystore.HaveKey(address))
                nResult ++;
        }

        return nResult;
    }


    /* Checks an output to your keystore to detect if you have a key that is involed in the output or transaction. */
    bool IsMine(const CKeyStore& keystore, const CScript& scriptPubKey)
    {
        std::vector< std::vector<uint8_t> > vSolutions;
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
                std::vector<std::vector<uint8_t> > keys(vSolutions.begin()+1, vSolutions.begin() + vSolutions.size()-1);
                return HaveKeys(keys, keystore) == keys.size();
            }
        }
        return false;
    }



    /* Extract a Nexus Address from a public key script. */
    bool ExtractAddress(const CScript& scriptPubKey, NexusAddress& addressRet)
    {
        std::vector< std::vector<uint8_t> > vSolutions;
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


    /* Extract a list of Nexus Addresses from a public key script. */
    bool ExtractAddresses(const CScript& scriptPubKey, TransactionType& typeRet, std::vector<NexusAddress>& addressRet, int32_t& nRequiredRet)
    {
        addressRet.clear();
        typeRet = TX_NONSTANDARD;
        std::vector< std::vector<uint8_t> > vSolutions;
        if (!Solver(scriptPubKey, typeRet, vSolutions))
            return false;

        if (typeRet == TX_MULTISIG)
        {
            nRequiredRet = vSolutions.front()[0];
            for (uint32_t i = 1; i < vSolutions.size() - 1; i++)
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


    /* Verify a script is a valid */
    bool VerifyScript(const CScript& scriptSig, const CScript& scriptPubKey, const Transaction& txTo, uint32_t nIn, int32_t nHashType)
    {
        std::vector< std::vector<uint8_t> > stack, stackCopy;
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

}
