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


        //RESERVERD to 0x3f (lvalues)
        TIMESTAMP   = 0x40,
        HEIGHT      = 0x41,
        REGISTER    = 0x42,
        PENDING     = 0x43

    };
};


static std::vector<uint8_t> binaryadd(const std::vector<uint8_t>& x, const std::vector<uint8_t>& y)
{
    std::vector<uint8_t> ret(x.size(), 0);

    uint16_t carry = 0;
    for (int i = 0; i < std::min(x.size(), y.size()); ++i)
    {
        uint16_t n = carry + x[i] + y[i];
        ret[i] = (n & 0xff);
        carry =  (n >> 8);
    }

    return ret;
}

static std::vector<uint8_t> binarysub(const std::vector<uint8_t>& x, const std::vector<uint8_t>& y)
{
    std::vector<uint8_t> ret(x.size(), 0);

    std::vector<uint8_t> sub(y.size(), 0);



    uint16_t carry = 0;
    for (int i = 0; i < std::min(x.size(), y.size()); ++i)
    {
        sub[i] = ~y[i];
        if(i == 0)
            sub[0] += 1;

        uint16_t n = carry + x[i] + sub[i];
        ret[i] = (n & 0xff);
        carry =  (n >> 8);
    }

    return ret;
}


/**  Compares two byte arrays and determines their signed equivalence byte for
 *   byte.
 **/
static int32_t compare(const std::vector<uint8_t>& a, const std::vector<uint8_t>& b)
{
    for(int64_t i = std::min(a.size(), b.size()) - 1; i >= 0; --i)
    {
        //debug::log(0, "I ", i, " Byte ", std::hex, (uint32_t)a[i], " vs Bytes ", std::hex, (uint32_t)b[i]);
        if(a[i] != b[i])
            return a[i] - b[i];

    }
    return 0;
}



std::vector<uint8_t> GetValue(const TAO::Operation::Stream& stream)
{
    TAO::Operation::Stream ret;

    while(!stream.end())
    {
        uint8_t OPERATION;
        stream >> OPERATION;

        switch(OPERATION)
        {
            //RVALUES
            case OP::UINT8_T:
            {
                uint8_t n;
                stream >> n;

                debug::log(0, (uint32_t)n);

                ret << n;

                break;
            }

            case OP::UINT16_T:
            {
                uint16_t n;
                stream >> n;

                debug::log(0, (uint32_t)n);

                ret << n;

                break;
            }

            case OP::UINT32_T:
            {
                uint32_t n;
                stream >> n;

                debug::log(0, n);

                ret << n;

                break;
            }

            case OP::UINT64_T:
            {
                uint64_t n;
                stream >> n;

                debug::log(0, n);

                ret << n;

                break;
            }

            case OP::UINT256_T:
            {
                uint256_t n;
                stream >> n;

                debug::log(0, n.ToString());

                ret << n;

                break;
            }

            case OP::UINT512_T:
            {
                uint512_t n;
                stream >> n;

                debug::log(0, n.ToString());

                ret << n;

                break;
            }

            case OP::UINT1024_T:
            {
                uint1024_t n;
                stream >> n;

                debug::log(0, n.ToString());

                ret << n;

                break;
            }



            //LVALUES

            case OP::TIMESTAMP:
            {
                ret << (uint64_t)runtime::timestamp();

                break;
            }



            //CALC
            case OP::ADD:
            {
                std::vector<uint8_t> lvalue = ret.Bytes();
                std::vector<uint8_t> rvalue = GetValue(stream);

                PrintHex(lvalue.begin(), lvalue.end());
                PrintHex(rvalue.begin(), rvalue.end());

                ret.SetNull();

                std::vector<uint8_t> sum = binaryadd(lvalue, rvalue);

                PrintHex(sum.begin(), sum.end());

                for(int i = 0; i < sum.size(); ++i)
                    ret << sum[i];

                break;
            }


            case OP::SUB:
            {
                std::vector<uint8_t> lvalue = ret.Bytes();
                std::vector<uint8_t> rvalue = GetValue(stream);

                PrintHex(lvalue.begin(), lvalue.end());
                PrintHex(rvalue.begin(), rvalue.end());

                ret.SetNull();

                std::vector<uint8_t> sub = binarysub(lvalue, rvalue);

                PrintHex(sub.begin(), sub.end());

                for(int i = 0; i < sub.size(); ++i)
                    ret << sub[i];


                break;
            }


            case OP::INC:
            {
                std::vector<uint8_t> lvalue = ret.Bytes();
                std::vector<uint8_t> rvalue(1);

                PrintHex(lvalue.begin(), lvalue.end());
                PrintHex(rvalue.begin(), rvalue.end());

                ret.SetNull();

                std::vector<uint8_t> sum = binaryadd(lvalue, rvalue);

                PrintHex(sum.begin(), sum.end());

                for(int i = 0; i < sum.size(); ++i)
                    ret << sum[i];

                break;
            }


            case OP::DEC:
            {
                std::vector<uint8_t> lvalue = ret.Bytes();
                std::vector<uint8_t> rvalue(1);

                PrintHex(lvalue.begin(), lvalue.end());
                PrintHex(rvalue.begin(), rvalue.end());

                ret.SetNull();

                std::vector<uint8_t> sub = binarysub(lvalue, rvalue);

                PrintHex(sub.begin(), sub.end());

                for(int i = 0; i < sub.size(); ++i)
                    ret << sub[i];

                break;
            }


            case OP::DIV:
            {
                std::vector<uint8_t> lvalue = ret.Bytes();
                const std::vector<uint8_t> rvalue = GetValue(stream);

                uint32_t nTotal = 0;
                while(compare(lvalue, rvalue) >= 0)
                {
                    lvalue = binarysub(lvalue, rvalue);

                    ++nTotal;
                }

                ret.SetNull();

                ret << nTotal;

                break;
            }


            case OP::MUL:
            {
                std::vector<uint8_t> lvalue = ret.Bytes();
                const std::vector<uint8_t> mul    = lvalue;

                std::vector<uint8_t> rvalue = GetValue(stream);
                std::vector<uint8_t> rdec(1, 1);

                std::vector<uint8_t> cmp(rvalue.size(), 0);

                while(compare(rvalue, cmp) != 1)
                {
                    lvalue = binaryadd(lvalue, mul);

                    rvalue = binarysub(rvalue, rdec);
                }

                ret.Set(lvalue);

                break;
            }


            case OP::EXP:
            {
                std::vector<uint8_t> lvalue             = ret.Bytes();
                const std::vector<uint8_t> base         = lvalue;

                std::vector<uint8_t> mul = lvalue;

                std::vector<uint8_t> rexp = GetValue(stream);
                std::vector<uint8_t> rdec(1, 1);

                std::vector<uint8_t> rvalue = lvalue;

                std::vector<uint8_t> cmp(std::min(rexp.size(), rvalue.size()), 0);


                std::vector<uint8_t> send(lvalue.size(), 0);
                while(compare(rexp , cmp) != 1)
                {
                    while(compare(rvalue, cmp) != 1)
                    {
                        lvalue = binaryadd(lvalue, mul);

                        rvalue = binarysub(rvalue, rdec);
                    }

                    rexp = binarysub(rexp, rdec);

                    mul    = lvalue;

                    rvalue = base;
                }

                ret.Set(lvalue);

                break;
            }


            case OP::MOD:
            {
                std::vector<uint8_t> lvalue = ret.Bytes();
                const std::vector<uint8_t> rvalue = GetValue(stream);

                while(compare(lvalue, rvalue) >= 0)
                    lvalue = binarysub(lvalue, rvalue);

                ret.Set(lvalue);

                break;
            }


            default:
            {
                stream.seek(-1);
                return ret.Bytes();
            }
        }

    }

    return ret.Bytes();
}




bool Validate(const TAO::Operation::Stream& stream)
{
    while(!stream.end())
    {
        std::vector<uint8_t> lvalue = GetValue(stream);

        uint8_t OPERATION;
        stream >> OPERATION;

        switch(OPERATION)
        {
            case OP::EQUALS:
            {
                std::vector<uint8_t> rvalue = GetValue(stream);

                PrintHex(lvalue.begin(), lvalue.end());
                PrintHex(rvalue.begin(), rvalue.end());

                return compare(lvalue, rvalue) == 0;
            }

            case OP::LESSTHAN:
            {
                std::vector<uint8_t> rvalue = GetValue(stream);
                return compare(lvalue, rvalue) < 0;
            }

            case OP::GREATERTHAN:
            {
                std::vector<uint8_t> rvalue = GetValue(stream);
                return compare(lvalue, rvalue) > 0;
            }

            case OP::NOTEQUALS:
            {
                std::vector<uint8_t> rvalue = GetValue(stream);
                return compare(lvalue, rvalue) != 0;
            }



            case OP::AND:
            {
                break;
            }

            case OP::OR:
            {
                break;
            }
        }
    }

    return true;
}


int main(int argc, char **argv)
{
    TAO::Operation::Stream stream;

    stream << (uint8_t)OP::UINT32_T << 900u << (uint8_t) OP::SUB << (uint8_t) OP::UINT32_T << 30u << (uint8_t)OP::EQUALS << (uint8_t)OP::UINT32_T << 870u;
    assert(Validate(stream));


    stream.SetNull();
    stream << (uint8_t)OP::UINT32_T << 555u << (uint8_t) OP::SUB << (uint8_t) OP::UINT32_T << 333u << (uint8_t)OP::EQUALS << (uint8_t)OP::UINT32_T << 222u;
    assert(Validate(stream));


    stream.SetNull();
    stream << (uint8_t)OP::UINT32_T << 9234837u << (uint8_t) OP::SUB << (uint8_t) OP::UINT32_T << 384728u << (uint8_t)OP::EQUALS << (uint8_t)OP::UINT32_T << 8850109u;
    assert(Validate(stream));

    stream.SetNull();
    stream << (uint8_t)OP::UINT32_T << 905u << (uint8_t) OP::MOD << (uint8_t) OP::UINT32_T << 30u << (uint8_t)OP::EQUALS << (uint8_t)OP::UINT32_T << 5u;
    assert(Validate(stream));


    stream.SetNull();
    stream << (uint8_t)OP::UINT32_T << 900u << (uint8_t) OP::DIV << (uint8_t) OP::UINT32_T << 30u << (uint8_t)OP::EQUALS << (uint8_t)OP::UINT32_T << 30u;
    assert(Validate(stream));


    stream.SetNull();
    stream << (uint8_t)OP::UINT32_T << 5u << (uint8_t) OP::MUL << (uint8_t) OP::UINT32_T << 5u << (uint8_t)OP::EQUALS << (uint8_t)OP::UINT32_T << 25u;
    assert(Validate(stream));


    stream.SetNull();
    stream << (uint8_t)OP::UINT32_T << 5u << (uint8_t) OP::EXP << (uint8_t) OP::UINT32_T << 3u << (uint8_t)OP::EQUALS << (uint8_t)OP::UINT32_T << 125u;
    assert(Validate(stream));


    stream.SetNull();
    stream << (uint8_t)OP::UINT32_T << 9837u << (uint8_t) OP::ADD << (uint8_t) OP::UINT32_T << 7878u << (uint8_t)OP::EQUALS << (uint8_t)OP::UINT32_T << 17715u;
    assert(Validate(stream));


    stream.SetNull();
    stream << (uint8_t)OP::UINT32_T << 9837u << (uint8_t) OP::ADD << (uint8_t) OP::UINT32_T << 7878u << (uint8_t)OP::LESSTHAN << (uint8_t)OP::UINT32_T << 17716u;
    assert(Validate(stream));



    return 0;
}
