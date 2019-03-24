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

#include <math.h>


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








//claims
//OP_CREDIT <txid of your debit> <hash-proof=0xff> <hash-to-b> 100
//---> check state of locking transaction

//OP_CREDIT <txid of your debit> <hash-proof=0xff> <hash-to-a> 500


struct OP
{
    /** Primitive Operations. **/
    enum
    {
        //register operations
        WRITE      = 0x01,
        REGISTER   = 0x02,
        AUTHORIZE  = 0x03,
        TRANSFER   = 0x04,
        REQUIRE    = 0x05,
        APPEND     = 0x06,


        //financial operations
        DEBIT      = 0x10,
        CREDIT     = 0x11,
        COINBASE   = 0x12,
        TRUST      = 0x13, //for proof of stake


        //internal funding
        AMBASSADOR = 0x20,
        DEVELOPER  = 0x21,

        //consensus operations
        VOTE  = 0x30,


        //0x41 = 0x69 RESERVED
    };


    /** Core validation types. **/
    struct TYPES
    {
        enum
        {
            UINT8_T     = 0x70,
            UINT16_T    = 0x71,
            UINT32_T    = 0x72,
            UINT64_T    = 0x73,
            UINT256_T   = 0x74,
            UINT512_T   = 0x75,
            UINT1024_T  = 0x76,
            STRING      = 0x77
        };
    };

    /** Core validation operations. **/
    enum
    {
        //RESERVED 0x08 - 0x0f
        EQUALS      = 0x80,
        LESSTHAN    = 0x81,
        GREATERTHAN = 0x82,
        NOTEQUALS   = 0x83,
        CONTAINS    = 0x84,


        //RESERVED to 0x1f
        ADD         = 0x90,
        SUB         = 0x91,
        DIV         = 0x92,
        MUL         = 0x93,
        MOD         = 0x94,
        INC         = 0x95,
        DEC         = 0x96,
        EXP         = 0x97,


        //RESERVED to 0x2f
        AND         = 0xa0,
        OR          = 0xa1,
        IF          = 0xa2,
    };


    /** Register layer state values. **/
    struct REGISTER
    {
        enum
        {
            TIMESTAMP     = 0xb0,
            OWNER         = 0xb1,
            TYPE          = 0xb2,
            STATE         = 0xb3
        };
    };


    /** Caller Values (The validation script caller). **/
    struct CALLER
    {
        enum
        {
            GENESIS      = 0xc0,
            TIMESTAMP    = 0xc1,
            OPERATIONS   = 0xc2,
        };
    };


    /* Ledger Layer State Values. */
    struct LEDGER
    {
        enum
        {
            HEIGHT        = 0xd0,
            BLOCK         = 0xd1,
            SUPPLY        = 0xd2,
            TIME          = 0xd3
        };
    };



    /* Cryptographic operations. */
    struct CRYPTO
    {
        enum
        {
            SK256        = 0xe0,
            SK512        = 0xe1
        };
    };


    /* Global state values. */
    struct GLOBAL
    {
        enum
        {
            UNIFIED       = 0xf0
        };
    };
};


/**  Compares two byte arrays and determines their signed equivalence byte for
 *   byte.
 **/
inline int64_t compare(const std::vector<uint64_t>& a, const std::vector<uint64_t>& b)
{
    size_t nSize = std::min(a.size(), b.size()) - 1;
    for(int64_t i = nSize; i >= 0; --i)
    {
        //debug::log(0, "I ", i, " Byte ", std::hex, a[i], " vs Bytes ", std::hex, b[i]);
        if(a[i] != b[i])
            return a[i] - b[i];

    }
    return 0;
}


struct Value
{
    uint32_t nBegin;
    uint32_t nEnd;

    Value()
    : nBegin(0)
    , nEnd(0)
    {

    }

    uint32_t size() const
    {
        return nEnd - nBegin;
    }

    bool null() const
    {
        return nBegin == 0 && nEnd == 0;
    }
};



class Registers
{
    std::vector<uint64_t> vRegister;

    uint32_t nPointer;

public:
    Registers()
    : vRegister(64, 0) //MAX of 512 bytes in register buffer.
    , nPointer(0)
    {

    }

    const uint8_t* begin(const Value& test) const
    {
        return (uint8_t*)&vRegister[test.nBegin];
    }


    const uint8_t* end(const Value& test) const
    {
        return (uint8_t*)&vRegister[test.nEnd];
    }


    template<typename Data>
    void allocate(const Data& data, Value& test)
    {
        test.nBegin = nPointer;
        test.nEnd   = nPointer + sizeof(data) / 8;

        if(test.nEnd >= vRegister.size())
            throw std::runtime_error(debug::safe_printstr(FUNCTION, " out of register memory"));

        std::copy((uint8_t*)&data, (uint8_t*)&data + sizeof(data), (uint8_t*)&vRegister[test.nBegin]);

        nPointer += test.size();
    }


    void allocate(const uint64_t& data, Value& test)
    {
        test.nBegin = nPointer;
        test.nEnd   = ++nPointer;

        if(test.nEnd >= vRegister.size())
            throw std::runtime_error(debug::safe_printstr(FUNCTION, " out of register memory"));

        vRegister[test.nBegin] = data;
    }


    void allocate(const std::vector<uint8_t>& data, Value& test)
    {
        test.nBegin = nPointer;
        test.nEnd   = nPointer + (data.size() / 8);

        if(test.nEnd >= vRegister.size())
            throw std::runtime_error(debug::safe_printstr(FUNCTION, " out of register memory"));

        std::copy((uint8_t*)&data[0], (uint8_t*)&data[0] + data.size(), (uint8_t*)&vRegister[test.nBegin]);

        nPointer += test.size();
    }


    void allocate(const std::string& data, Value& test)
    {
        test.nBegin = nPointer;
        test.nEnd   = nPointer + (data.size() / 8);

        if(test.nEnd >= vRegister.size())
            throw std::runtime_error(debug::safe_printstr(FUNCTION, " out of register memory"));

        std::copy((uint8_t*)&data[0], (uint8_t*)&data[0] + data.size(), (uint8_t*)&vRegister[test.nBegin]);

        nPointer += test.size();
    }


    template<typename Data>
    void deallocate(Data& data, const Value& test)
    {
        if(test.nEnd != nPointer)
            throw std::runtime_error(debug::safe_printstr(FUNCTION, " cannot deallocate when not last"));

        std::copy((uint8_t*)&vRegister[test.nBegin], (uint8_t*)&vRegister[test.nEnd], (uint8_t*)&data);

        if((int32_t)(nPointer - test.size()) < 0)
            throw std::runtime_error(debug::safe_printstr(FUNCTION, " invalid memory address ", nPointer - test.size()));

        nPointer -= test.size();
    }


    void deallocate(const Value& test)
    {
        if(test.nEnd != nPointer)
            throw std::runtime_error(debug::safe_printstr(FUNCTION, " cannot deallocate when not last"));

        if((int32_t)(nPointer - test.size()) < 0)
            throw std::runtime_error(debug::safe_printstr(FUNCTION, " invalid memory address ", nPointer - test.size()));

        nPointer -= test.size();
    }

    uint64_t& at(const Value& test)
    {
        return vRegister[test.nBegin];
    }

    /**  Compares two byte arrays and determines their signed equivalence byte for
     *   byte.
     **/
    int64_t compare(const Value& a, const Value& b)
    {
        size_t nSize = std::min(a.size(), b.size()) - 1;
        for(int64_t i = nSize; i >= 0; --i)
        {
            //debug::log(0, "I ", i, " Byte ", std::hex, a[i], " vs Bytes ", std::hex, b[i]);
            if(vRegister[a.nBegin + i] != vRegister[b.nBegin + i])
                return vRegister[a.nBegin + i] - vRegister[b.nBegin + i];

        }
        return 0;
    }

    uint32_t available()
    {
        return (vRegister.size() - nPointer) * 8;
    }
};





Registers registers;




/** Get Value
 *
 *  Extracts a value from an operation ssOperation for use in the boolean validation scripts.
 *
 **/
inline bool GetValue(const TAO::Operation::Stream& ssOperation, Value &vRet, int32_t &nLimit)
{

    /* Iterate until end of stream. */
    while(!ssOperation.end())
    {

        /* Extract the operation byte. */
        uint8_t OPERATION;
        ssOperation >> OPERATION;


        /* Switch based on the operation. */
        switch(OPERATION)
        {

            /* Add two 64-bit numbers. */
            case OP::ADD:
            {
                /* Get the add from r-value. */
                Value vAdd;
                if(!GetValue(ssOperation, vAdd, nLimit))
                    return false;

                /* Check computational bounds. */
                if(vAdd.size() > 1 || vRet.size() > 1)
                    throw std::runtime_error(debug::safe_printstr("OP::ADD computation greater than 64-bits"));

                /* Compute the return value. */
                registers.at(vRet) += registers.at(vAdd);

                /* Deallocate r-value from memory. */
                registers.deallocate(vAdd);

                /* Reduce the limits to prevent operation exhuastive attacks. */
                nLimit -= 64;

                break;
            }


            /* Subtract one number from another. */
            case OP::SUB:
            {
                /* Get the sub from r-value. */
                Value vSub;
                if(!GetValue(ssOperation, vSub, nLimit))
                    return false;

                /* Check computational bounds. */
                if(vSub.size() > 1 || vRet.size() > 1)
                    throw std::runtime_error(debug::safe_printstr("OP::SUB computation greater than 64-bits"));

                /* Compute the return value. */
                registers.at(vRet) -= registers.at(vSub);

                /* Deallocate r-value from memory. */
                registers.deallocate(vSub);

                /* Reduce the limits to prevent operation exhuastive attacks. */
                nLimit -= 64;

                break;
            }


            /* Increment a number by an order of 1. */
            case OP::INC:
            {
                /* Check computational bounds. */
                if(vRet.size() > 1)
                    throw std::runtime_error(debug::safe_printstr("OP::INC computation greater than 64-bits"));

                /* Compute the return value. */
                ++registers.at(vRet);

                /* Reduce the limits to prevent operation exhuastive attacks. */
                nLimit -= 64;

                break;
            }


            /* De-increment a number by an order of 1. */
            case OP::DEC:
            {
                /* Check computational bounds. */
                if(vRet.size() > 1)
                    throw std::runtime_error(debug::safe_printstr("OP::DEC computation greater than 64-bits"));

                /* Compute the return value. */
                --registers.at(vRet);

                /* Reduce the limits to prevent operation exhuastive attacks. */
                nLimit -= 64;

                break;
            }


            /* Divide a number by another. */
            case OP::DIV:
            {
                /* Get the divisor from r-value. */
                Value vDiv;
                if(!GetValue(ssOperation, vDiv, nLimit))
                    return false;

                /* Check computational bounds. */
                if(vDiv.size() > 1 || vRet.size() > 1)
                    throw std::runtime_error(debug::safe_printstr("OP::DIV computation greater than 64-bits"));

                /* Compute the return value. */
                registers.at(vRet) /= registers.at(vDiv);

                /* Deallocate r-value from memory. */
                registers.deallocate(vDiv);

                /* Reduce the limits to prevent operation exhuastive attacks. */
                nLimit -= 128;

                break;
            }


            /* Multiply a number by another. */
            case OP::MUL:
            {
                /* Get the multiplier from r-value. */
                Value vMul;
                if(!GetValue(ssOperation, vMul, nLimit))
                    return false;

                /* Check computational bounds. */
                if(vMul.size() > 1 || vRet.size() > 1)
                    throw std::runtime_error(debug::safe_printstr("OP::MUL computation greater than 64-bits"));

                /* Compute the return value. */
                registers.at(vRet) *= registers.at(vMul);

                /* Deallocate r-value from memory. */
                registers.deallocate(vMul);

                /* Reduce the limits to prevent operation exhuastive attacks. */
                nLimit -= 128;

                break;
            }


            /* Raise a number by the power of another. */
            case OP::EXP:
            {
                /* Get the exponent from r-value. */
                Value vExp;
                if(!GetValue(ssOperation, vExp, nLimit))
                    return false;

                /* Check computational bounds. */
                if(vExp.size() > 1 || vRet.size() > 1)
                    throw std::runtime_error(debug::safe_printstr("OP::EXP computation greater than 64-bits"));

                /* Compute the return value. */
                registers.at(vRet) = pow(registers.at(vRet), registers.at(vExp));

                /* Deallocate r-value from memory. */
                registers.deallocate(vExp);

                /* Reduce the limits to prevent operation exhuastive attacks. */
                nLimit -= 256;

                break;
            }


            /* Get the remainder after a division. */
            case OP::MOD:
            {
                /* Get the modulus from r-value. */
                Value vMod;
                if(!GetValue(ssOperation, vMod, nLimit))
                    return false;

                /* Check computational bounds. */
                if(vMod.size() > 1 || vRet.size() > 1)
                    throw std::runtime_error(debug::safe_printstr("OP::MOD computation greater than 64-bits"));

                /* Compute the return value. */
                registers.at(vRet) %= registers.at(vMod);

                /* Deallocate r-value from memory. */
                registers.deallocate(vMod);

                /* Reduce the limits to prevent operation exhuastive attacks. */
                nLimit -= 128;

                break;
            }


            /* Extract an uint8_t from the stream. */
            case OP::TYPES::UINT8_T:
            {
                /* Extract the byte. */
                uint8_t n;
                ssOperation >> n;

                /* Set the register value. */
                registers.allocate((uint64_t)n, vRet);

                /* Reduce the limits to prevent operation exhuastive attacks. */
                nLimit -= 1;

                break;
            }


            /* Extract an uint16_t from the stream. */
            case OP::TYPES::UINT16_T:
            {
                /* Extract the short. */
                uint16_t n;
                ssOperation >> n;

                /* Set the register value. */
                registers.allocate((uint64_t)n, vRet);

                /* Reduce the limits to prevent operation exhuastive attacks. */
                nLimit -= 2;

                break;
            }


            /* Extract an uint32_t from the stream. */
            case OP::TYPES::UINT32_T:
            {
                /* Extract the integer. */
                uint32_t n;
                ssOperation >> n;

                /* Set the register value. */
                registers.allocate((uint64_t)n, vRet);

                /* Reduce the limits to prevent operation exhuastive attacks. */
                nLimit -= 4;

                break;
            }


            /* Extract an uint64_t from the stream. */
            case OP::TYPES::UINT64_T:
            {
                /* Extract the integer. */
                uint64_t n;
                ssOperation >> n;

                /* Set the register value. */
                registers.allocate(n, vRet);

                /* Reduce the limits to prevent operation exhuastive attacks. */
                nLimit -= 8;

                break;
            }


            /* Extract an uint256_t from the stream. */
            case OP::TYPES::UINT256_T:
            {
                /* Extract the integer. */
                uint256_t n;
                ssOperation >> n;

                /* Set the register value. */
                registers.allocate(n, vRet);

                /* Reduce the limits to prevent operation exhuastive attacks. */
                nLimit -= 32;

                break;

            }


            /* Extract an uint512_t from the stream. */
            case OP::TYPES::UINT512_T:
            {
                /* Extract the integer. */
                uint512_t n;
                ssOperation >> n;

                /* Set the register value. */
                registers.allocate(n, vRet);

                /* Reduce the limits to prevent operation exhuastive attacks. */
                nLimit -= 64;

                break;
            }


            /* Extract an uint1024_t from the stream. */
            case OP::TYPES::UINT1024_T:
            {
                /* Extract the integer. */
                uint1024_t n;
                ssOperation >> n;

                /* Set the register value. */
                registers.allocate(n, vRet);

                /* Reduce the limits to prevent operation exhuastive attacks. */
                nLimit -= 128;

                break;
            }


            /* Extract a string from the stream. */
            case OP::TYPES::STRING:
            {
                /* Extract the string. */
                std::string str;
                ssOperation >> str;

                /* Set the register value. */
                registers.allocate(str, vRet);

                /* Reduce the limits to prevent operation exhuastive attacks. */
                nLimit -= str.size();

                break;
            }


            /* Get a unified timestamp and push to return value. */
            case OP::GLOBAL::UNIFIED:
            {
                /* Get the current unified timestamp. */
                uint64_t n = runtime::unifiedtimestamp();

                /* Set the register value. */
                registers.allocate(n, vRet);

                /* Reduce the limits to prevent operation exhuastive attacks. */
                nLimit -= 8;

                break;
            }


            /* Get a register's timestamp and push to the return value. */
            case OP::REGISTER::TIMESTAMP:
            {
                /* Read the register vAddress. */
                uint256_t hashRegister;
                ssOperation >> hashRegister;

                /* Read the register states. */
                TAO::Register::State state;
                if(!LLD::regDB->Read(hashRegister, state))
                    return false;

                /* Set the register value. */
                registers.allocate(state.nTimestamp, vRet);

                /* Reduce the limits to prevent operation exhuastive attacks. */
                nLimit -= 40;

                break;
            }


            /* Get a register's owner and push to the return value. */
            case OP::REGISTER::OWNER:
            {
                /* Read the register vAddress. */
                uint256_t hashRegister;
                ssOperation >> hashRegister;

                /* Read the register states. */
                TAO::Register::State state;
                if(!LLD::regDB->Read(hashRegister, state))
                    return false;

                /* Set the register value. */
                registers.allocate(state.hashOwner, vRet);

                /* Reduce the limits to prevent operation exhuastive attacks. */
                nLimit -= 64;

                break;
            }


            /* Get a register's type and push to the return value. */
            case OP::REGISTER::TYPE:
            {
                /* Read the register vAddress. */
                uint256_t hashRegister;
                ssOperation >> hashRegister;

                /* Read the register states. */
                TAO::Register::State state;
                if(!LLD::regDB->Read(hashRegister, state))
                    return false;

                /* Push the type onto the return value. */
                registers.allocate((uint64_t)state.nType, vRet);

                /* Reduce the limits to prevent operation exhuastive attacks. */
                nLimit -= 33;

                break;
            }


            /* Get a register's state and push to the return value. */
            case OP::REGISTER::STATE:
            {
                /* Read the register vAddress. */
                uint256_t hashRegister;
                ssOperation >> hashRegister;

                /* Read the register states. */
                TAO::Register::State state;
                if(!LLD::regDB->Read(hashRegister, state))
                    return false;

                /* Allocate to the registers. */
                registers.allocate(state.GetState(), vRet);

                /* Reduce the limits to prevent operation exhuastive attacks. */
                nLimit -= 128;

                break;
            }


            /* Compute an SK256 hash of current return value. */
            case OP::CRYPTO::SK256:
            {
                if(vRet.null())
                    throw std::runtime_error("OP::CRYPTO::SK256: can't hash with no input");

                /* Compute the return hash. */
                uint256_t hash = LLC::SK256((uint8_t*)registers.begin(vRet), (uint8_t*)registers.end(vRet));

                /* Copy new hash into return value. */
                registers.deallocate(vRet);
                registers.allocate(hash, vRet);

                /* Reduce the limits to prevent operation exhuastive attacks. */
                nLimit -= 512;

                break;
            }


            /* Compute an SK512 hash of current return value. */
            case OP::CRYPTO::SK512:
            {
                if(vRet.null())
                    throw std::runtime_error("OP::CRYPTO::SK512: can't hash with no input");

                /* Compute the return hash. */
                uint512_t hash = LLC::SK512((uint8_t*)registers.begin(vRet), (uint8_t*)registers.end(vRet));

                /* Copy new hash into return value. */
                registers.deallocate(vRet);
                registers.allocate(hash, vRet);

                /* Reduce the limits to prevent operation exhuastive attacks. */
                nLimit -= 1024;

                break;
            }


            default:
            {
                /* If no applicable instruction found, rewind and return. */
                ssOperation.seek(-1);
                if(nLimit < 0)
                    debug::error(FUNCTION, "out of computational limits ", nLimit);

                return (nLimit >= 0);
            }
        }
    }

    /* Log if computational limits expired. */
    if(nLimit < 0)
        debug::error(FUNCTION, "out of computational limits ", nLimit);

    return (nLimit >= 0);
}


//Exchange would be:
//OP::DEBIT <hash-from-a> <0xffffffffffffff> 100
//OP::REQUIRE OP_DEBIT <identifier> 500

//OP::AND TIMESTAMP OP::LESSTHAN OP::CHAIN::UNIFIED
//OP::OR OP_CREDIT <%txid%> <hash-proof=0xff> <MATCH::hash-to-a> 100

//first debit:
// OP::DEBIT <hash-from-account> <0xffffffff> OP::REQUIRE OP::CALLER::OPERATIONS OP::CONTAINS OP::DEBIT <0xffffffffff> <0xffffffffff> 100 AND OP::REGISTER::TYPE OP::CALLER::DEBIT::FROM OP::EQUALS TAO::REGISTER::ACCOUNT AND OP::REGISTER::ACCOUNT::IDENTIFIER OP::CALLER::DEBIT::FROM OP::EQUALS <token-type>


//second debit:
// OP::DEBIT <hash-from-account> <0xffffffff>> OP_VALIDATE <txid>
//---> OP_VALIDATE executes OP::VALIDATE from txid, if OP_REQUIRE satisfied


//return to self
//OP_VALIDATE <txid> OP_CREDIT <txid> <hash-proof=0xff> <hash-to-a> 100


bool Validate(const TAO::Operation::Stream& ssOperation)
{
    int32_t nLimit = 2048;

    bool fRet = false;

    ssOperation.reset();

    Value lvalue;
    while(!ssOperation.end())
    {
        //uint32_t nBeginL = nPosition;
        if(!GetValue(ssOperation, lvalue, nLimit))
            return false;

        uint8_t OPERATION;
        ssOperation >> OPERATION;

        switch(OPERATION)
        {

            case OP::DEBIT:
            {
                break;
            }


            case OP::CREDIT:
            {
                break;
            }


            case OP::EQUALS:
            {
                Value rvalue;
                if(!GetValue(ssOperation, rvalue, nLimit))
                    return false;

                fRet = (registers.compare(lvalue, rvalue) == 0);


                registers.deallocate(rvalue);
                registers.deallocate(lvalue);

                break;
            }

            case OP::LESSTHAN:
            {
                Value rvalue;
                if(!GetValue(ssOperation, rvalue, nLimit))
                    return false;

                fRet = (registers.compare(lvalue, rvalue) < 0);


                registers.deallocate(rvalue);
                registers.deallocate(lvalue);

                break;
            }

            case OP::GREATERTHAN:
            {
                Value rvalue;
                if(!GetValue(ssOperation, rvalue, nLimit))
                    return false;

                fRet = (registers.compare(lvalue, rvalue) > 0);


                registers.deallocate(rvalue);
                registers.deallocate(lvalue);

                break;
            }

            case OP::NOTEQUALS:
            {
                Value rvalue;
                if(!GetValue(ssOperation, rvalue, nLimit))
                    return false;

                fRet = (registers.compare(lvalue, rvalue) != 0);


                registers.deallocate(rvalue);
                registers.deallocate(lvalue);

                break;
            }

            case OP::AND:
            {
                return fRet && Validate(ssOperation);
            }

            //OP::GROUP
            //OP::UNGROUP

            case OP::OR:
            {
                if(fRet)
                    return true;

                return Validate(ssOperation);
            }

            default:
            {
                return false;
            }
        }
    }

    return fRet;
}


bool Validate2(uint64_t a, uint64_t b)
{
    uint64_t vRet;
    vRet = pow(a, b);

    return (vRet == 25);
}






int main(int argc, char **argv)
{


    Value test;
    uint256_t hashing = LLC::GetRand256();

    registers.allocate(hashing, test);

    debug::log(0, "Hash ", hashing.ToString());

    //uint256_t random = LLC::GetRand256();

    Value test2;

    registers.allocate(hashing, test2);

    debug::log(0, "Random ", hashing.ToString());

    debug::log(0, "Compare: ", registers.compare(test, test2));


    Value test3;
    uint64_t nTest = 48384839483948;

    registers.allocate(nTest, test3);

    debug::log(0, "Test ", nTest);


    uint64_t nTest33;

    registers.deallocate(nTest33, test3);

    debug::log(0, "Test33 ", nTest33);


    uint256_t hash333;
    registers.deallocate(hash333, test2);


    debug::log(0, "Hash333 ", hash333.ToString());


    uint256_t random2;
    registers.deallocate(random2, test);

    debug::log(0, "Random ", random2.ToString());

    debug::log(0, "Available bytes ", registers.available());


    TAO::Operation::Stream ssOperation;

    ssOperation << (uint8_t)OP::TYPES::UINT32_T << (uint32_t)7u << (uint8_t) OP::EXP << (uint8_t) OP::TYPES::UINT32_T << (uint32_t)2u << (uint8_t) OP::EQUALS << (uint8_t)OP::TYPES::UINT32_T << (uint32_t)49u;

    runtime::timer bench;
    bench.Reset();

    runtime::timer func;
    func.Reset();

    for(int t = 0; t < 10000000; ++t)
    {
        assert(Validate(ssOperation));
    }

    //uint64_t nTime = bench.ElapsedMicroseconds();

    debug::log(0, "Operations ", bench.ElapsedMicroseconds());

    uint64_t nA = 5;
    uint64_t nB = 2;

    bench.Reset();
    for(uint64_t i = 0; i <= 1000000; ++i)
    {
        assert(Validate2(nA, nB));
        ssOperation.reset();
    }

    //nTime = bench.ElapsedMicroseconds();
    debug::log(0, "Binary ", bench.ElapsedMicroseconds());


    uint256_t hash = LLC::GetRand256();

    uint256_t hash2 = LLC::GetRand256();
    TAO::Register::State state;
    state.hashOwner = LLC::GetRand256();
    state.nType     = 2;
    state << hash2;


    LLD::regDB = new LLD::RegisterDB();
    assert(LLD::regDB->Write(hash, state));

    std::string strName = "colasdfasdfasdfasdfasdfasdfasdfasdfasdfasdfasdfasdfasdfasdfin!!!";
    uint256_t hashRegister = LLC::SK256(std::vector<uint8_t>(strName.begin(), strName.end()));


    ssOperation.SetNull();
    ssOperation << (uint8_t)OP::TYPES::STRING << strName << (uint8_t)OP::EQUALS << (uint8_t)OP::TYPES::STRING << strName;
    assert(Validate(ssOperation));


    ssOperation.SetNull();
    ssOperation << (uint8_t)OP::TYPES::STRING << strName << (uint8_t)OP::CRYPTO::SK256 << (uint8_t)OP::EQUALS << (uint8_t)OP::TYPES::UINT256_T << hashRegister;
    assert(Validate(ssOperation));


    uint512_t hashRegister2 = LLC::SK512(strName.begin(), strName.end());

    ssOperation.SetNull();
    ssOperation << (uint8_t)OP::TYPES::STRING << strName << (uint8_t)OP::CRYPTO::SK512 << (uint8_t)OP::EQUALS << (uint8_t)OP::TYPES::UINT512_T << hashRegister2;
    assert(Validate(ssOperation));





    //////////////COMPARISONS
    ssOperation.SetNull();
    ssOperation << (uint8_t)OP::TYPES::UINT256_T << hash << (uint8_t)OP::EQUALS << (uint8_t)OP::TYPES::UINT256_T << hash;
    assert(Validate(ssOperation));


    ssOperation.SetNull();
    ssOperation << (uint8_t)OP::TYPES::UINT256_T << hash << (uint8_t)OP::LESSTHAN << (uint8_t)OP::TYPES::UINT256_T << hash;
    assert(!Validate(ssOperation));


    ssOperation.SetNull();
    ssOperation << (uint8_t)OP::TYPES::UINT256_T << hash << (uint8_t)OP::GREATERTHAN << (uint8_t)OP::TYPES::UINT256_T << hash;
    assert(!Validate(ssOperation));


    ssOperation.SetNull();
    ssOperation << (uint8_t)OP::TYPES::UINT256_T << hash << (uint8_t)OP::NOTEQUALS << (uint8_t)OP::TYPES::UINT256_T << hash;
    assert(!Validate(ssOperation));


    ssOperation.SetNull();
    ssOperation << (uint8_t)OP::TYPES::UINT256_T << hash << (uint8_t)OP::NOTEQUALS << (uint8_t)OP::TYPES::UINT256_T << hash + 1;
    assert(Validate(ssOperation));




    /////REGISTERS
    ssOperation.SetNull();
    ssOperation << (uint8_t)OP::REGISTER::STATE << hash << (uint8_t)OP::EQUALS << (uint8_t) OP::TYPES::UINT256_T << hash2;
    assert(Validate(ssOperation));

    ssOperation.SetNull();
    ssOperation << (uint8_t)OP::REGISTER::OWNER << hash << (uint8_t)OP::EQUALS << (uint8_t) OP::TYPES::UINT256_T << state.hashOwner;
    assert(Validate(ssOperation));

    ssOperation.SetNull();
    ssOperation << (uint8_t)OP::REGISTER::TIMESTAMP << hash << (uint8_t) OP::ADD << (uint8_t) OP::TYPES::UINT32_T << 3u << (uint8_t)OP::GREATERTHAN << (uint8_t) OP::GLOBAL::UNIFIED;
    assert(Validate(ssOperation));

    ssOperation.SetNull();
    ssOperation << (uint8_t)OP::REGISTER::TYPE << hash << (uint8_t)OP::EQUALS << (uint8_t) OP::TYPES::UINT64_T << (uint64_t) 2;
    assert(Validate(ssOperation));


    return 0;

    ////////////COMPUTATION
    ssOperation.SetNull();
    ssOperation << (uint8_t)OP::TYPES::UINT32_T << 555u << (uint8_t) OP::SUB << (uint8_t) OP::TYPES::UINT32_T << 333u << (uint8_t)OP::EQUALS << (uint8_t)OP::TYPES::UINT32_T << 222u;
    assert(Validate(ssOperation));

    //return 0;

    ssOperation.SetNull();
    ssOperation << (uint8_t)OP::TYPES::UINT32_T << 9234837u << (uint8_t) OP::SUB << (uint8_t) OP::TYPES::UINT32_T << 384728u << (uint8_t)OP::EQUALS << (uint8_t)OP::TYPES::UINT32_T << 8850109u;
    assert(Validate(ssOperation));

    ssOperation.SetNull();
    ssOperation << (uint8_t)OP::TYPES::UINT32_T << 905u << (uint8_t) OP::MOD << (uint8_t) OP::TYPES::UINT32_T << 30u << (uint8_t)OP::EQUALS << (uint8_t)OP::TYPES::UINT32_T << 5u;
    assert(Validate(ssOperation));

    ssOperation.SetNull();
    ssOperation << (uint8_t)OP::TYPES::UINT64_T << (uint64_t)1000000000000 << (uint8_t) OP::DIV << (uint8_t) OP::TYPES::UINT64_T << (uint64_t)30000 << (uint8_t)OP::EQUALS << (uint8_t)OP::TYPES::UINT32_T << 33333333u;
    assert(Validate(ssOperation));

    ssOperation.SetNull();
    ssOperation << (uint8_t)OP::TYPES::UINT64_T << (uint64_t)5 << (uint8_t) OP::EXP << (uint8_t) OP::TYPES::UINT64_T << (uint64_t)15 << (uint8_t)OP::EQUALS << (uint8_t)OP::TYPES::UINT64_T << (uint64_t)30517578125;
    assert(Validate(ssOperation));

    ssOperation.SetNull();
    ssOperation << (uint8_t)OP::TYPES::UINT32_T << 2u << (uint8_t) OP::EXP << (uint8_t) OP::TYPES::UINT32_T << 8u << (uint8_t)OP::EQUALS << (uint8_t)OP::TYPES::UINT32_T << 256u;
    assert(Validate(ssOperation));

    ssOperation.SetNull();
    ssOperation << (uint8_t)OP::TYPES::UINT64_T << (uint64_t)9837 << (uint8_t) OP::ADD << (uint8_t) OP::TYPES::UINT64_T << (uint64_t)7878 << (uint8_t)OP::EQUALS << (uint8_t)OP::TYPES::UINT32_T << 17715u;
    assert(Validate(ssOperation));

    ssOperation.SetNull();
    ssOperation << (uint8_t)OP::TYPES::UINT64_T << (uint64_t)9837 << (uint8_t) OP::ADD << (uint8_t) OP::TYPES::UINT32_T << 7878u << (uint8_t)OP::LESSTHAN << (uint8_t)OP::TYPES::UINT256_T << uint256_t(17716);
    assert(Validate(ssOperation));


    //////////////// AND / OR
    ssOperation.SetNull();
    ssOperation << (uint8_t)OP::TYPES::UINT32_T << 9837u << (uint8_t) OP::ADD << (uint8_t) OP::TYPES::UINT32_T << 7878u << (uint8_t)OP::EQUALS << (uint8_t)OP::TYPES::UINT32_T << 17715u;
    ssOperation << (uint8_t)OP::AND;
    ssOperation << (uint8_t)OP::TYPES::UINT32_T << 9837u << (uint8_t) OP::ADD << (uint8_t) OP::TYPES::UINT32_T << 7878u << (uint8_t)OP::EQUALS << (uint8_t)OP::TYPES::UINT32_T << 17715u;
    ssOperation << (uint8_t)OP::AND;
    ssOperation << (uint8_t)OP::TYPES::UINT32_T << 9837u << (uint8_t) OP::ADD << (uint8_t) OP::TYPES::UINT32_T << 7878u << (uint8_t)OP::EQUALS << (uint8_t)OP::TYPES::UINT32_T << 17715u;
    assert(Validate(ssOperation));



    return 0;
}
