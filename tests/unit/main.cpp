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
    std::vector<uint8_t> ret(0, std::min(x.size(), y.size()));

    uint16_t carry = 0;
    for (int i = 0; i < ret.size(); ++i)
    {
        uint16_t n = carry + x[i] + y[i];
        ret[i] = (n & 0xff);
        carry =  (n >> 8);
    }

    return ret;
}

uint8_t binarysub(uint8_t x, uint8_t y)
{
    // Iterate till there
    // is no carry
    while (y != 0)
    {
        // borrow contains common
        // set bits of y and unset
        // bits of x
        uint8_t borrow = (~x) & y;

        // Subtraction of bits of x
        // and y where at least one
        // of the bits is not set
        x = x ^ y;

        // Borrow is shifted by one
        // so that subtracting it from
        // x gives the required sum
        y = borrow << 1;
    }
    return x;
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

                for(int i = 0; i < std::min(lvalue.size(), rvalue.size()); ++i)
                    ret << binarysub(lvalue[i], rvalue[i]);

                break;
            }

            case OP::DIV:
            {
                break;
            }

            case OP::MUL:
            {
                break;
            }

            case OP::MOD:
            {
                std::vector<uint8_t> lvalue = ret.Bytes();
                std::vector<uint8_t> rvalue = GetValue(stream);

                uint8_t result = 0; // Just in case, to prevent overflow
                for(int i = 0; i < lvalue.size(); i++)
                {
                    result *= (256 % rvalue[0]);
                    result %= rvalue[0];
                    result += (lvalue[i] % rvalue[0]);
                    result %= rvalue[0];
                }

                ret.SetNull();

                ret << result;

                debug::log(0, "MOD ", (uint32_t)result);

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

                return memory::compare(&lvalue[0], &rvalue[0], std::min(lvalue.size(), rvalue.size())) == 0;
            }

            case OP::LESSTHAN:
            {
                std::vector<uint8_t> rvalue = GetValue(stream);
                return memory::compare(&lvalue[0], &rvalue[0], std::min(lvalue.size(), rvalue.size())) < 0;
            }

            case OP::GREATERTHAN:
            {
                std::vector<uint8_t> rvalue = GetValue(stream);
                return memory::compare(&lvalue[0], &rvalue[0], std::min(lvalue.size(), rvalue.size())) > 0;
            }

            case OP::NOTEQUALS:
            {
                std::vector<uint8_t> rvalue = GetValue(stream);
                return memory::compare(&lvalue[0], &rvalue[0], std::min(lvalue.size(), rvalue.size())) != 0;
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

    uint64_t n = 726;
    stream << (uint8_t)OP::UINT32_T << 526u << (uint8_t) OP::ADD << (uint8_t)OP::UINT32_T << 200u << (uint8_t)OP::EQUALS << (uint8_t)OP::UINT32_T << n;
    debug::log(0, "Validate: ", Validate(stream) ? "True" : "False");

    memory::encrypted_ptr<TAO::Ledger::SignatureChain> user = new TAO::Ledger::SignatureChain("colin", "passing");

    uint512_t hashGenerate = user->Generate(0, "1234");

    debug::log(0, hashGenerate.ToString());

    uint512_t hashGenerate2 = user->Generate(0, "1234");

    debug::log(0, hashGenerate2.ToString());

    user.free();


    runtime::timer timer;
    timer.Start();
    LLC::FLKey key;

    /* Get the secret from new key. */
    std::vector<uint8_t> vBytes = hashGenerate.GetBytes();
    LLC::CSecret vchSecret(vBytes.begin(), vBytes.end());

    key.SetSecret(vchSecret, true);
    uint64_t nElapsed = timer.ElapsedMicroseconds();
    debug::log(0, FUNCTION, "Generated in ", nElapsed, " microseconds");
    timer.Reset();

    uint256_t hashRandom = 55;
    vchMessage = hashRandom.GetBytes();

    if(!key.Sign(vchMessage, vchSignature))
        return debug::error(FUNCTION, "failed to sign");

    nElapsed = timer.ElapsedMicroseconds();
    debug::log(0, FUNCTION, "Signed in ", nElapsed, " microseconds");
    timer.Reset();

    vchPubKey = key.GetPubKey();



    LLC::FLKey key2;
    key2.SetPubKey(vchPubKey);
    if(!key2.Verify(vchMessage, vchSignature))
        debug::error(FUNCTION, "failed to verify");

    nElapsed = timer.ElapsedMicroseconds();
    debug::log(0, FUNCTION, "Verified in ", nElapsed, " microseconds");

    debug::log(0, FUNCTION, "Passed (", vchPubKey.size() + vchSignature.size(), " bytes)");



    return 0;
}
