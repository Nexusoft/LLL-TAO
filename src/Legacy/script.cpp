/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/include/key.h>
#include <LLC/hash/SK.h>


#include <Util/include/debug.h>
#include <Util/include/hex.h>

#include <Legacy/include/enum.h>
#include <Legacy/types/address.h>
#include <Legacy/types/script.h>

#include <cstring>
#include <vector>
#include <algorithm>

namespace Legacy
{

    /* Returns a string in integer value. */
    std::string ValueString(const std::vector<uint8_t>& vch)
    {
        if (vch.size() <= 4)
            return debug::strprintf("%d", LLC::CBigNum(vch).getint32());
        else
            return HexStr(vch);
    }


    /* Builds a string of values in a stack seperated by spaces */
    std::string StackString(const std::vector<std::vector<uint8_t> >& vStack)
    {
        std::string str;
        for(auto vch : vStack)
        {
            if (!str.empty())
                str += " ";
            str += ValueString(vch);
        }
        return str;
    }


    /* Push a 64 bit signed int onto the stack. */
    CScript& CScript::push_int64(int64_t n)
    {
        if (n == -1 || (n >= 1 && n <= 16))
        {
            push_back(n + (OP_1 - 1));
        }
        else
        {
            LLC::CBigNum bn(n);
            *this << bn.getvch();
        }
        return *this;
    }

    /* Push a 64 bit unsigned int onto the stack. */
    CScript& CScript::push_uint64(uint64_t n)
    {
        if (n >= 1 && n <= 16)
        {
            push_back(n + (OP_1 - 1));
        }
        else
        {
            LLC::CBigNum bn(n);
            *this << bn.getvch();
        }
        return *this;
    }


    /* Get the op codes from stack */
    bool CScript::GetOp(iterator& pc, opcodetype& opcodeRet, std::vector<uint8_t>& vchRet)
    {
        // Wrapper so it can be called with either iterator or const_iterator
        const_iterator pc2 = pc;
        bool fRet = GetOp2(pc2, opcodeRet, &vchRet);
        pc = begin() + (pc2 - begin());
        return fRet;
    }


    /* Get the op codes from stack */
    bool CScript::GetOp(iterator& pc, opcodetype& opcodeRet)
    {
        const_iterator pc2 = pc;
        bool fRet = GetOp2(pc2, opcodeRet, nullptr);
        pc = begin() + (pc2 - begin());
        return fRet;
    }


    /* Get the op codes from stack */
    bool CScript::GetOp(const_iterator& pc, opcodetype& opcodeRet, std::vector<uint8_t>& vchRet) const
    {
        return GetOp2(pc, opcodeRet, &vchRet);
    }


    /* Get the op codes from stack */
    bool CScript::GetOp(const_iterator& pc, opcodetype& opcodeRet) const
    {
        return GetOp2(pc, opcodeRet, nullptr);
    }


    /* Get the op codes from stack */
    bool CScript::GetOp2(const_iterator& pc, opcodetype& opcodeRet, std::vector<uint8_t>* pvchRet) const
    {
        opcodeRet = OP_INVALIDOPCODE;
        if (pvchRet)
            pvchRet->clear();
        if (pc >= end())
            return false;

        // Read instruction
        if (end() - pc < 1)
            return false;
        uint32_t opcode = *pc++;

        // Immediate operand
        if (opcode <= OP_PUSHDATA4)
        {
            uint32_t nSize = 0;
            if (opcode < OP_PUSHDATA1)
            {
                nSize = opcode;
            }
            else if (opcode == OP_PUSHDATA1)
            {
                if (end() - pc < 1)
                    return false;
                nSize = *pc++;
            }
            else if (opcode == OP_PUSHDATA2)
            {
                if (end() - pc < 2)
                    return false;
                nSize = 0;
                //memcpy(&nSize, &pc[0], 2);
                std::copy((uint8_t *)&pc[0], (uint8_t *)&pc[0] + 2, &nSize);
                pc += 2;
            }
            else if (opcode == OP_PUSHDATA4)
            {
                if (end() - pc < 4)
                    return false;
                //memcpy(&nSize, &pc[0], 4);
                std::copy((uint8_t *)&pc[0], (uint8_t *)&pc[0] + 4, &nSize);
                pc += 4;
            }
            if (end() - pc < nSize)
                return false;
            if (pvchRet)
                pvchRet->assign(pc, pc + nSize);
            pc += nSize;
        }

        opcodeRet = (opcodetype)opcode;
        return true;
    }


    /* Decodes the operation code. */
    int32_t CScript::DecodeOP_N(opcodetype opcode) const
    {
        if (opcode == OP_0)
            return 0;
        assert(opcode >= OP_1 && opcode <= OP_16);
        return (int)opcode - (int)(OP_1 - 1);
    }


    /* Encodes the operation code. */
    opcodetype CScript::EncodeOP_N(int n)
    {
        assert(n >= 0 && n <= 16);
        if (n == 0)
            return OP_0;
        return (opcodetype)(OP_1+n-1);
    }


    /* Removes a script from a script */
    int CScript::FindAndDelete(const CScript& b)
    {
        int nFound = 0;
        if (b.empty())
            return nFound;
        iterator pc = begin();
        opcodetype opcode;
        do
        {
            while (end() - pc >= (long)b.size() && memcmp(&pc[0], &b[0], b.size()) == 0)
            {
                erase(pc, pc + b.size());
                ++nFound;
            }
        }
        while (GetOp(pc, opcode));
        return nFound;
    }


    /* Find the location of an opcode in the script. */
    int CScript::Find(opcodetype op) const
    {
        int nFound = 0;
        opcodetype opcode;
        for (const_iterator pc = begin(); pc != end() && GetOp(pc, opcode);)
            if (opcode == op)
                ++nFound;
        return nFound;
    }


    /* Get the total number of signature operations */
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


    /* Get the total number of signature operations */
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


    /* Determine if script fits P2SH template */
    bool CScript::IsPayToScriptHash() const
    {
        // Extra-fast test for pay-to-script-hash CScripts:
        return (this->size() == 23 &&
                this->at(0) == OP_HASH256 &&
                this->at(1) == 0x14 &&
                this->at(22) == OP_EQUAL);
    }


    /* Determine if script is a pushdata only script */
    bool CScript::IsPushOnly() const
    {
        const_iterator pc = begin();
        while (pc < end())
        {
            opcodetype opcode;
            if (!GetOp(pc, opcode))
                return false;
            if (opcode > OP_16)
                return false;
        }
        return true;
    }


    /* Set the nexus address into script */
    void CScript::SetNexusAddress(const NexusAddress& address)
    {
        this->clear();
        if (address.IsScript())
            *this << OP_HASH256 << address.GetHash256() << OP_EQUAL;
        else
            *this << OP_DUP << OP_HASH256 << address.GetHash256() << OP_EQUALVERIFY << OP_CHECKSIG;
    }


    /* Set the nexus address from public key */
    void CScript::SetNexusAddress(const std::vector<uint8_t>& vchPubKey)
    {
        SetNexusAddress(NexusAddress(vchPubKey));
    }


    /* Set script based on multi-sig data */
    void CScript::SetMultisig(int nRequired, const std::vector<LLC::ECKey>& keys)
    {
        this->clear();

        *this << EncodeOP_N(nRequired);
        for(auto key : keys)
            *this << key.GetPubKey();
        *this << EncodeOP_N(keys.size()) << OP_CHECKMULTISIG;
    }


    /* Set the script based on a P2SH script input */
    void CScript::SetPayToScriptHash(const CScript& subscript)
    {
        assert(!subscript.empty());
        uint256_t subscriptHash = LLC::SK256(subscript);
        this->clear();
        *this << OP_HASH256 << subscriptHash << OP_EQUAL;
    }


    /* Print the Hex output of the script */
    void CScript::PrintHex() const
    {
        debug::log(0, "CScript(", HexStr(begin(), end(), true), ")");
    }


    /* Print the Hex output of the script into a std::string */
    std::string CScript::ToString(bool fShort) const
    {
        std::string str;
        opcodetype opcode;
        std::vector<uint8_t> vch;
        const_iterator pc = begin();
        while (pc < end())
        {
            if (!str.empty())
                str += " ";
            if (!GetOp(pc, opcode, vch))
            {
                str += "[error]";
                return str;
            }
            if (0 <= opcode && opcode <= OP_PUSHDATA4)
                str += fShort? ValueString(vch).substr(0, 10) : ValueString(vch);
            else
                str += GetOpName(opcode);
        }
        return str;
    }


    /* Dump the Hex data into std::out or console */
    void CScript::print() const
    {
        printf("%s", ToString().c_str());
    }
}
