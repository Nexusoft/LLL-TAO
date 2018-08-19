/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_TAO_LEGACY_TYPES_INCLUDE_SCRIPT_H
#define NEXUS_TAO_LEGACY_TYPES_INCLUDE_SCRIPT_H

#include "base58.h"

#include <string>
#include <vector>

#include <boost/foreach.hpp>

namespace Core { class CTransaction; }
namespace Legacy
{
    class CKeyStore;


    const char* GetTxnOutputType(TransactionType t);


    const char* GetOpName(opcodetype opcode);



    inline std::string ValueString(const std::vector<uint8_t>& vch)
    {
        if (vch.size() <= 4)
            return strprintf("%d", CBigNum(vch).getint());
        else
            return HexStr(vch);
    }

    inline std::string StackString(const std::vector<std::vector<uint8_t> >& vStack)
    {
        std::string str;
        for(const auto & vch : vStack)
        {
            if (!str.empty())
                str += " ";
            str += ValueString(vch);
        }
        return str;
    }








    /** Serialized script, used inside transaction inputs and outputs */
    class CScript : public std::vector<uint8_t>
    {
    protected:
        CScript& push_int64(int64_t n)
        {
            if (n == -1 || (n >= 1 && n <= 16))
            {
                push_back(n + (OP_1 - 1));
            }
            else
            {
                CBigNum bn(n);
                *this << bn.getvch();
            }
            return *this;
        }

        CScript& push_uint64(uint64_t n)
        {
            if (n >= 1 && n <= 16)
            {
                push_back(n + (OP_1 - 1));
            }
            else
            {
                CBigNum bn(n);
                *this << bn.getvch();
            }
            return *this;
        }

    public:
        CScript() { }
        CScript(const CScript& b) : std::vector<uint8_t>(b.begin(), b.end()) { }
        CScript(const_iterator pbegin, const_iterator pend) : std::vector<uint8_t>(pbegin, pend) { }
    #ifndef _MSC_VER
        CScript(const uint8_t* pbegin, const uint8_t* pend) : std::vector<uint8_t>(pbegin, pend) { }
    #endif

        CScript& operator+=(const CScript& b)
        {
            insert(end(), b.begin(), b.end());
            return *this;
        }

        friend CScript operator+(const CScript& a, const CScript& b)
        {
            CScript ret = a;
            ret += b;
            return ret;
        }


        //explicit CScript(char b) is not portable.  Use 'signed char' or 'uint8_t'.
        explicit CScript(signed char b)    { operator<<(b); }
        explicit CScript(short b)          { operator<<(b); }
        explicit CScript(int b)            { operator<<(b); }
        explicit CScript(long b)           { operator<<(b); }
        explicit CScript(int64_t b)          { operator<<(b); }
        explicit CScript(uint8_t b)  { operator<<(b); }
        explicit CScript(uint32_t b)   { operator<<(b); }
        explicit CScript(uint16_t b) { operator<<(b); }
        explicit CScript(unsigned long b)  { operator<<(b); }
        explicit CScript(uint64_t b)         { operator<<(b); }

        explicit CScript(opcodetype b)     { operator<<(b); }
        explicit CScript(const uint256_t& b) { operator<<(b); }
        explicit CScript(const CBigNum& b) { operator<<(b); }
        explicit CScript(const std::vector<uint8_t>& b) { operator<<(b); }


        //CScript& operator<<(char b) is not portable.  Use 'signed char' or 'uint8_t'.
        CScript& operator<<(signed char b)    { return push_int64(b); }
        CScript& operator<<(short b)          { return push_int64(b); }
        CScript& operator<<(int b)            { return push_int64(b); }
        CScript& operator<<(long b)           { return push_int64(b); }
        CScript& operator<<(int64_t b)          { return push_int64(b); }
        CScript& operator<<(uint8_t b)  { return push_uint64(b); }
        CScript& operator<<(uint32_t b)   { return push_uint64(b); }
        CScript& operator<<(uint16_t b) { return push_uint64(b); }
        CScript& operator<<(unsigned long b)  { return push_uint64(b); }
        CScript& operator<<(uint64_t b)         { return push_uint64(b); }

        CScript& operator<<(opcodetype opcode)
        {
            if (opcode < 0 || opcode > 0xff)
                throw std::runtime_error("CScript::operator<<() : invalid opcode");
            insert(end(), (uint8_t)opcode);
            return *this;
        }

        CScript& operator<<(const uint256_t& b)
        {
            insert(end(), sizeof(b));
            insert(end(), (uint8_t*)&b, (uint8_t*)&b + sizeof(b));
            return *this;
        }

        CScript& operator<<(const CBigNum& b)
        {
            *this << b.getvch();
            return *this;
        }

        CScript& operator<<(const std::vector<uint8_t>& b)
        {
            if (b.size() < OP_PUSHDATA1)
            {
                insert(end(), (uint8_t)b.size());
            }
            else if (b.size() <= 0xff)
            {
                insert(end(), OP_PUSHDATA1);
                insert(end(), (uint8_t)b.size());
            }
            else if (b.size() <= 0xffff)
            {
                insert(end(), OP_PUSHDATA2);
                uint16_t nSize = b.size();
                insert(end(), (uint8_t*)&nSize, (uint8_t*)&nSize + sizeof(nSize));
            }
            else
            {
                insert(end(), OP_PUSHDATA4);
                uint32_t nSize = b.size();
                insert(end(), (uint8_t*)&nSize, (uint8_t*)&nSize + sizeof(nSize));
            }
            insert(end(), b.begin(), b.end());
            return *this;
        }

        CScript& operator<<(const CScript& b)
        {
            // I'm not sure if this should push the script or concatenate scripts.
            // If there's ever a use for pushing a script onto a script, delete this member fn
            assert(!"warning: pushing a CScript onto a CScript with << is probably not intended, use + to concatenate");
            return *this;
        }


        bool GetOp(iterator& pc, opcodetype& opcodeRet, std::vector<uint8_t>& vchRet)
        {
            // Wrapper so it can be called with either iterator or const_iterator
            const_iterator pc2 = pc;
            bool fRet = GetOp2(pc2, opcodeRet, &vchRet);
            pc = begin() + (pc2 - begin());
            return fRet;
        }

        bool GetOp(iterator& pc, opcodetype& opcodeRet)
        {
            const_iterator pc2 = pc;
            bool fRet = GetOp2(pc2, opcodeRet, NULL);
            pc = begin() + (pc2 - begin());
            return fRet;
        }

        bool GetOp(const_iterator& pc, opcodetype& opcodeRet, std::vector<uint8_t>& vchRet) const
        {
            return GetOp2(pc, opcodeRet, &vchRet);
        }

        bool GetOp(const_iterator& pc, opcodetype& opcodeRet) const
        {
            return GetOp2(pc, opcodeRet, NULL);
        }

        bool GetOp2(const_iterator& pc, opcodetype& opcodeRet, std::vector<uint8_t>* pvchRet) const
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
                uint32_t nSize;
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
                    memcpy(&nSize, &pc[0], 2);
                    pc += 2;
                }
                else if (opcode == OP_PUSHDATA4)
                {
                    if (end() - pc < 4)
                        return false;
                    memcpy(&nSize, &pc[0], 4);
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

        // Encode/decode small integers:
        static int DecodeOP_N(opcodetype opcode)
        {
            if (opcode == OP_0)
                return 0;
            assert(opcode >= OP_1 && opcode <= OP_16);
            return (int)opcode - (int)(OP_1 - 1);
        }
        static opcodetype EncodeOP_N(int n)
        {
            assert(n >= 0 && n <= 16);
            if (n == 0)
                return OP_0;
            return (opcodetype)(OP_1+n-1);
        }

        int FindAndDelete(const CScript& b)
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
        int Find(opcodetype op) const
        {
            int nFound = 0;
            opcodetype opcode;
            for (const_iterator pc = begin(); pc != end() && GetOp(pc, opcode);)
                if (opcode == op)
                    ++nFound;
            return nFound;
        }

        // Pre-version-0.6, Nexus always counted CHECKMULTISIGs
        // as 20 sigops. With pay-to-script-hash, that changed:
        // CHECKMULTISIGs serialized in scriptSigs are
        // counted more accurately, assuming they are of the form
        //  ... OP_N CHECKMULTISIG ...
        uint32_t GetSigOpCount(bool fAccurate) const;

        // Accurately count sigOps, including sigOps in
        // pay-to-script-hash transactions:
        uint32_t GetSigOpCount(const CScript& scriptSig) const;

        bool IsPayToScriptHash() const;

        // Called by Core::CTransaction::IsStandard
        bool IsPushOnly() const
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


        void SetNexusAddress(const NexusAddress& address);
        void SetNexusAddress(const std::vector<uint8_t>& vchPubKey)
        {
            SetNexusAddress(NexusAddress(vchPubKey));
        }
        void SetMultisig(int nRequired, const std::vector<CKey>& keys);
        void SetPayToScriptHash(const CScript& subscript);


        void PrintHex() const
        {
            printf("CScript(%s)\n", HexStr(begin(), end(), true).c_str());
        }

        std::string ToString(bool fShort=false) const
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

        void print() const
        {
            printf("%s\n", ToString().c_str());
        }
    };





    bool EvalScript(std::vector<std::vector<uint8_t> >& stack, const CScript& script, const Core::CTransaction& txTo, uint32_t nIn, int nHashType);
    bool Solver(const CScript& scriptPubKey, TransactionType& typeRet, std::vector<std::vector<uint8_t> >& vSolutionsRet);
    int ScriptSigArgsExpected(TransactionType t, const std::vector<std::vector<uint8_t> >& vSolutions);
    bool IsStandard(const CScript& scriptPubKey);
    bool IsMine(const CKeyStore& keystore, const CScript& scriptPubKey);
    bool ExtractAddress(const CScript& scriptPubKey, NexusAddress& addressRet);
    bool ExtractAddresses(const CScript& scriptPubKey, TransactionType& typeRet, std::vector<NexusAddress>& addressRet, int& nRequiredRet);
    bool SignSignature(const CKeyStore& keystore, const Core::CTransaction& txFrom, Core::CTransaction& txTo, uint32_t nIn, int nHashType=SIGHASH_ALL);
    bool VerifySignature(const Core::CTransaction& txFrom, const Core::CTransaction& txTo, uint32_t nIn, int nHashType);
    bool VerifyScript(const CScript& scriptSig, const CScript& scriptPubKey, const Core::CTransaction& txTo, uint32_t nIn, int nHashType);

    CScript CombineSignatures(CScript scriptPubKey, const Core::CTransaction& txTo, uint32_t nIn, const CScript& scriptSig1, const CScript& scriptSig2);
}

#endif
