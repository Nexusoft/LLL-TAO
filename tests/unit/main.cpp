#include <Util/include/debug.h>
#include <Util/include/hex.h>

#include <Util/include/runtime.h>

#include <LLC/include/flkey.h>

#include <TAO/Ledger/types/sigchain.h>

#include <Util/include/memory.h>

#include <LLC/include/random.h>

#include <openssl/rand.h>

#include <LLC/aes/aes.h>

#include <TAO/Operation/include/stream.h>

#include <LLD/include/global.h>


std::atomic<uint64_t> nVerified;

std::vector<uint8_t> vchPubKey;

std::vector<uint8_t> vchSignature;

std::vector<uint8_t> vchMessage;

void Verifier()
{
    while(true)
    {
        LLC::FLKey key2;
        key2.SetPubKey(vchPubKey);
        if(!key2.Verify(vchMessage, vchSignature))
            debug::error(FUNCTION, "failed to verify");

        ++nVerified;
    }
}


struct OP
{
    struct TYPES
    {
        enum
        {
            RESERVED    = 0x00,
            UINT8_T     = 0x01,
            UINT16_T    = 0x02,
            UINT32_T    = 0x03,
            UINT64_T    = 0x04,
            UINT256_T   = 0x05,
            UINT512_T   = 0x06,
            UINT1024_T  = 0x07,
        };
    };

    enum
    {
        //RESERVED 0x08 - 0x0f
        EQUALS      = 0x10,
        LESSTHAN    = 0x11,
        GREATERTHAN = 0x12,
        NOTEQUALS   = 0x13,


        //RESERVED to 0x1f
        ADD         = 0x20,
        SUB         = 0x21,
        DIV         = 0x22,
        MUL         = 0x23,
        MOD         = 0x24,
        INC         = 0x25,
        DEC         = 0x26,
        EXP         = 0x27,


        //RESERVED to 0x2f
        AND         = 0x30,
        OR          = 0x31,
        IF          = 0x32,
    };


    //BYTES 0xa0 and up
    struct REGISTER
    {
        enum
        {
            TIMESTAMP     = 0xa0,
            OWNER         = 0xa1,
            TYPE          = 0xa2,
            STATE         = 0xa3
        };
    };


    //BYTES 0xb0 and up
    struct CALLER
    {
        enum
        {
            GENESIS      = 0xb0,
            TIMESTAMP    = 0xb1,
            TXID         = 0xb2
        };
    };


    //BYTES 0xc0 and up
    struct CHAIN
    {
        enum
        {
            HEIGHT        = 0xc0,
            TIMESTAMP     = 0xc1,
            UNIFIED       = 0xc2
        };
    };
};


inline void x86_add(const std::vector<uint32_t>& x, const std::vector<uint32_t>& y, std::vector<uint32_t>& ret)
{
    uint64_t carry = 0;

    size_t nSize = std::min(x.size(), y.size());
    for (int i = 0; i < nSize; ++i)
    {
        uint64_t n = carry + x[i] + y[i];
        ret[i] = (n & 0xffffffff);
        carry  =  (n >> 32);
    }
}


inline void x86_sub(const std::vector<uint32_t>& x, const std::vector<uint32_t>& y, std::vector<uint32_t>& ret)
{
    std::vector<uint32_t> sub(y.size(), 0);
    for(int i = 0; i < sub.size(); i++)
        sub[i] = ~y[i];

    int i = 0;
    while (++sub[i] == 0 && i < sub.size())
        ++i;

    uint64_t carry = 0;
    for (int i = 0; i < std::min(x.size(), y.size()); ++i)
    {
        uint64_t n = carry + x[i] + sub[i];
        ret[i] = (n & 0xffffffff);
        carry =  (n >> 32);
    }
}


/**  Compares two byte arrays and determines their signed equivalence byte for
 *   byte.
 **/
inline int32_t x86_compare(const std::vector<uint32_t>& a, const std::vector<uint32_t>& b)
{
    size_t nSize = std::min(a.size(), b.size()) - 1;
    for(int64_t i = nSize; i >= 0; --i)
    {
        //debug::log(0, "I ", i, " Byte ", std::hex, (uint32_t)a[i], " vs Bytes ", std::hex, (uint32_t)b[i]);
        if(a[i] != b[i])
            return a[i] - b[i];

    }
    return 0;
}



inline bool x86_GetValue(const TAO::Operation::Stream& stream, std::vector<uint32_t>& ret, int32_t &nLimit)
{
    while(!stream.end())
    {
        uint8_t OPERATION;
        stream >> OPERATION;

        switch(OPERATION)
        {
            //RVALUES
            case OP::TYPES::UINT8_T:
            {
                nLimit -= 1;

                uint8_t n;
                stream >> n;

                ret.push_back(n);

                break;
            }

            case OP::TYPES::UINT16_T:
            {
                nLimit -= 2;

                uint16_t n;
                stream >> n;

                ret.push_back(n);

                break;
            }

            case OP::TYPES::UINT32_T:
            {
                nLimit -= 4;

                uint32_t n;
                stream >> n;

                ret.push_back(n);

                break;
            }

            case OP::TYPES::UINT64_T:
            {
                nLimit -= 8;

                uint64_t n;
                stream >> n;

                ret.push_back(n);
                ret.push_back(n >> 32);

                break;
            }

            case OP::TYPES::UINT256_T:
            {
                nLimit -= 32;

                uint256_t n;
                stream >> n;

                for(int i = 0; i < 8; i++)
                    ret.push_back(n.get(i));

                break;
            }

            case OP::TYPES::UINT512_T:
            {
                nLimit -= 64;

                uint512_t n;
                stream >> n;

                for(int i = 0; i < 16; i++)
                    ret.push_back(n.get(i));

                break;
            }

            case OP::TYPES::UINT1024_T:
            {
                nLimit -= 128;

                uint1024_t n;
                stream >> n;

                for(int i = 0; i < 32; i++)
                    ret.push_back(n.get(i));

                break;
            }


            //LVALUES
            case OP::CHAIN::UNIFIED:
            {
                nLimit -= 8;

                uint64_t n = runtime::timestamp();
                ret.push_back(n);
                ret.push_back(n >> 32);

                break;
            }


            case OP::REGISTER::TIMESTAMP:
            {
                nLimit -= 40;

                uint256_t hashRegister;
                stream >> hashRegister;

                TAO::Register::State state;
                if(!LLD::regDB->Read(hashRegister, state))
                    return false;

                ret.push_back(state.nTimestamp);
                ret.push_back(state.nTimestamp >> 32);

                break;
            }


            case OP::REGISTER::OWNER:
            {
                nLimit -= 64;

                uint256_t hashRegister;
                stream >> hashRegister;

                TAO::Register::State state;
                if(!LLD::regDB->Read(hashRegister, state))
                    return false;

                for(int i = 0; i < 8; i++)
                    ret.push_back(state.hashOwner.get(i));

                break;
            }


            case OP::REGISTER::TYPE:
            {
                nLimit -= 33;

                uint256_t hashRegister;
                stream >> hashRegister;

                TAO::Register::State state;
                if(!LLD::regDB->Read(hashRegister, state))
                    return false;

                ret.push_back(state.nType);

                break;
            }


            case OP::REGISTER::STATE:
            {
                nLimit -= 128;

                uint256_t hashRegister;
                stream >> hashRegister;

                TAO::Register::State state;
                if(!LLD::regDB->Read(hashRegister, state))
                    return false;

                std::vector<uint8_t> vchState = state.GetState();

                //TODO: add binary data into 32-bit VM form

                break;
            }




            //COMPUTATION
            case OP::ADD:
            {
                std::vector<uint32_t> rvalue;
                if(!x86_GetValue(stream, rvalue, nLimit))
                    return false;

                x86_add(ret, rvalue, ret);

                nLimit -= ((ret.size() + rvalue.size()) * 4);

                break;
            }


            case OP::SUB:
            {
                std::vector<uint32_t> rvalue;
                if(!x86_GetValue(stream, rvalue, nLimit))
                    return false;

                x86_sub(ret, rvalue, ret);

                nLimit -= ((ret.size() + rvalue.size()) * 4);

                break;
            }


            case OP::INC:
            {
                std::vector<uint32_t> rvalue(1);
                x86_add(ret, rvalue, ret);

                nLimit -= ((ret.size() + 1) * 4);

                break;
            }


            case OP::DEC:
            {
                std::vector<uint32_t> rvalue(1);
                x86_sub(ret, rvalue, ret);

                nLimit -= ((ret.size() + 1) * 4);

                break;
            }


            case OP::DIV:
            {
                std::vector<uint32_t> rvalue;
                if(!x86_GetValue(stream, rvalue, nLimit))
                    return false;

                uint32_t nTotal = 0;
                while(x86_compare(ret, rvalue) >= 0)
                {
                    x86_sub(ret, rvalue, ret);

                    ++nTotal;
                }

                ret.clear();
                ret.push_back(nTotal);

                nLimit -= ((ret.size() + rvalue.size()) * 16);

                break;
            }


            case OP::MUL:
            {
                const std::vector<uint32_t> mul = ret;

                std::vector<uint32_t> rvalue;
                if(!x86_GetValue(stream, rvalue, nLimit))
                    return false;

                std::vector<uint32_t> rdec(1, 1);

                std::vector<uint32_t> cmp(rvalue.size(), 0);

                while(x86_compare(rvalue, cmp) != 1)
                {
                    x86_add(ret, mul, ret);
                    x86_sub(rvalue, rdec, rvalue);
                }

                nLimit -= ((ret.size() + rvalue.size()) * 16);

                break;
            }


            case OP::EXP:
            {
                const std::vector<uint32_t> base = ret;

                std::vector<uint32_t> mul = base;

                std::vector<uint32_t> rexp;
                if(!x86_GetValue(stream, rexp, nLimit))
                    return false;

                std::vector<uint32_t> rdec(1, 1);

                std::vector<uint32_t> rvalue = ret;

                std::vector<uint32_t> cmp(std::min(rexp.size(), rvalue.size()), 0);
                while(x86_compare(rexp , cmp) != 1)
                {
                    while(x86_compare(rvalue, cmp) != 1)
                    {
                        x86_add(ret, mul, ret);

                        x86_sub(rvalue, rdec, rvalue);
                    }

                    x86_sub(rexp, rdec, rexp);

                    mul    = ret;
                    rvalue = base;
                }


                nLimit -= ((ret.size() + base.size()) * 64);

                break;
            }


            case OP::MOD:
            {
                std::vector<uint32_t> rvalue;
                if(!x86_GetValue(stream, rvalue, nLimit))
                    return false;

                while(x86_compare(ret, rvalue) >= 0)
                    x86_sub(ret, rvalue, ret);

                nLimit -= ((ret.size() + rvalue.size()) * 16);

                break;
            }


            default:
            {
                stream.seek(-1);
                if(nLimit < 0)
                    debug::error(FUNCTION, "out of computational limits ", nLimit);

                return (nLimit >= 0);
            }
        }
    }

    if(nLimit < 0)
        debug::error(FUNCTION, "out of computational limits ", nLimit);

    return (nLimit >= 0);
}




bool x86_Validate(const TAO::Operation::Stream& stream)
{
    int32_t nLimit = 256;

    bool fRet = false;

    std::vector<uint32_t> lvalue;
    while(!stream.end())
    {
        if(!x86_GetValue(stream, lvalue, nLimit))
            return false;

        uint8_t OPERATION;
        stream >> OPERATION;

        switch(OPERATION)
        {
            case OP::EQUALS:
            {
                std::vector<uint32_t> rvalue;
                if(!x86_GetValue(stream, rvalue, nLimit))
                    return false;

                fRet = (x86_compare(lvalue, rvalue) == 0);

                break;
            }

            case OP::LESSTHAN:
            {
                std::vector<uint32_t> rvalue;
                if(!x86_GetValue(stream, rvalue, nLimit))
                    return false;

                fRet = (x86_compare(lvalue, rvalue) < 0);

                break;
            }

            case OP::GREATERTHAN:
            {
                std::vector<uint32_t> rvalue;
                if(!x86_GetValue(stream, rvalue, nLimit))
                    return false;

                fRet = (x86_compare(lvalue, rvalue) > 0);

                break;
            }

            case OP::NOTEQUALS:
            {
                std::vector<uint32_t> rvalue;
                if(!x86_GetValue(stream, rvalue, nLimit))
                    return false;

                fRet = (x86_compare(lvalue, rvalue) != 0);

                break;
            }

            case OP::AND:
            {
                return fRet && x86_Validate(stream);
            }

            case OP::OR:
            {
                if(fRet)
                    return true;

                return x86_Validate(stream);
            }

            default:
            {
                return false;
            }
        }
    }

    return fRet;
}


bool Validate(uint32_t a, uint32_t b)
{
    return a + b == 10;
}



int main(int argc, char **argv)
{
    TAO::Operation::Stream stream;

    stream << (uint8_t)OP::TYPES::UINT64_T << (uint64_t)5u << (uint8_t) OP::ADD << (uint8_t) OP::TYPES::UINT64_T << (uint64_t)5u << (uint8_t)OP::EQUALS << (uint8_t)OP::TYPES::UINT64_T << (uint64_t)10u;

    runtime::timer bench;
    bench.Start();
    for(int i = 0; i < 1000000; ++i)
    {
        assert(x86_Validate(stream));
        stream.reset();
    }

    uint64_t nTime = bench.ElapsedMicroseconds();

    debug::log(0, "Operations ", nTime);

    uint32_t nA = 5;
    uint32_t nB = 5;

    bench.Reset();
    for(uint64_t i = 0; i <= 1000000; ++i)
    {
        assert(Validate(nA, nB));
        stream.reset();
    }

    nTime = bench.ElapsedMicroseconds();
    debug::log(0, "Binary ", nTime);


    stream.SetNull();
    stream << (uint8_t)OP::TYPES::UINT32_T << 555u << (uint8_t) OP::SUB << (uint8_t) OP::TYPES::UINT32_T << 333u << (uint8_t)OP::EQUALS << (uint8_t)OP::TYPES::UINT32_T << 222u;
    assert(x86_Validate(stream));


    stream.SetNull();
    stream << (uint8_t)OP::TYPES::UINT32_T << 9234837u << (uint8_t) OP::SUB << (uint8_t) OP::TYPES::UINT32_T << 384728u << (uint8_t)OP::EQUALS << (uint8_t)OP::TYPES::UINT32_T << 8850109u;
    assert(x86_Validate(stream));

    stream.SetNull();
    stream << (uint8_t)OP::TYPES::UINT32_T << 905u << (uint8_t) OP::MOD << (uint8_t) OP::TYPES::UINT32_T << 30u << (uint8_t)OP::EQUALS << (uint8_t)OP::TYPES::UINT32_T << 5u;
    assert(x86_Validate(stream));


    stream.SetNull();
    stream << (uint8_t)OP::TYPES::UINT32_T << 1200u << (uint8_t) OP::DIV << (uint8_t) OP::TYPES::UINT32_T << 30u << (uint8_t)OP::EQUALS << (uint8_t)OP::TYPES::UINT32_T << 40u;
    assert(x86_Validate(stream));


    stream.SetNull();
    stream << (uint8_t)OP::TYPES::UINT32_T << 5u << (uint8_t) OP::MUL << (uint8_t) OP::TYPES::UINT32_T << 5u << (uint8_t)OP::EQUALS << (uint8_t)OP::TYPES::UINT32_T << 25u;
    assert(x86_Validate(stream));


    stream.SetNull();
    stream << (uint8_t)OP::TYPES::UINT32_T << 2u << (uint8_t) OP::EXP << (uint8_t) OP::TYPES::UINT32_T << 8u << (uint8_t)OP::EQUALS << (uint8_t)OP::TYPES::UINT32_T << 256u;
    assert(x86_Validate(stream));


    stream.SetNull();
    stream << (uint8_t)OP::TYPES::UINT64_T << (uint64_t)9837 << (uint8_t) OP::ADD << (uint8_t) OP::TYPES::UINT64_T << (uint64_t)7878 << (uint8_t)OP::EQUALS << (uint8_t)OP::TYPES::UINT32_T << 17715u;
    assert(x86_Validate(stream));


    stream.SetNull();
    stream << (uint8_t)OP::TYPES::UINT256_T << uint256_t(9837) << (uint8_t) OP::ADD << (uint8_t) OP::TYPES::UINT32_T << 7878u << (uint8_t)OP::LESSTHAN << (uint8_t)OP::TYPES::UINT256_T << uint256_t(17716);
    assert(x86_Validate(stream));


    stream.SetNull();
    stream << (uint8_t)OP::TYPES::UINT32_T << 9837u << (uint8_t) OP::ADD << (uint8_t) OP::TYPES::UINT32_T << 7878u << (uint8_t)OP::EQUALS << (uint8_t)OP::TYPES::UINT32_T << 17715u;
    stream << (uint8_t)OP::AND;
    stream << (uint8_t)OP::TYPES::UINT32_T << 9837u << (uint8_t) OP::ADD << (uint8_t) OP::TYPES::UINT32_T << 7878u << (uint8_t)OP::EQUALS << (uint8_t)OP::TYPES::UINT32_T << 17715u;
    stream << (uint8_t)OP::AND;
    stream << (uint8_t)OP::TYPES::UINT32_T << 9837u << (uint8_t) OP::ADD << (uint8_t) OP::TYPES::UINT32_T << 7878u << (uint8_t)OP::EQUALS << (uint8_t)OP::TYPES::UINT32_T << 17715u;
    assert(x86_Validate(stream));



    return 0;
}
