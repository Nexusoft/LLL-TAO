#include <Util/include/debug.h>
#include <Util/include/hex.h>

#include <Util/include/runtime.h>

#include <LLC/include/flkey.h>

#include <TAO/Ledger/types/sigchain.h>

#include <Util/include/memory.h>

#include <LLC/include/random.h>

#include <openssl/rand.h>


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



static std::vector<uint8_t> vKey(32);

template<class TypeName>
void crypt(TypeName* data)
{
    size_t nSize = sizeof(*data);
    std::vector<uint8_t> vData(nSize);

    std::copy((uint8_t*)data, (uint8_t*)data + nSize, (uint8_t*)&vData[0]);
    for(int i = 0; i < sizeof(*data); i++)
    {
        int index = i % vKey.size();
        vData[i] ^= vKey[index];
    }

    std::copy((uint8_t*)&vData[0], (uint8_t*)&vData[0] + nSize, (uint8_t*)data);
}


/** lock_proxy
 *
 *  Temporary class that unlocks a mutex when outside of scope.
 *  Useful for protecting member access to a raw pointer.
 *
 **/
template <class TypeName>
class decrypted_proxy
{
    /** Reference of the mutex. **/
    std::recursive_mutex& MUTEX;


    /** The pointer being locked. **/
    TypeName* data;

    std::atomic<int>& nRefs;


public:

    /** Basic constructor
     *
     *  Assign the pointer and reference to the mutex.
     *
     *  @param[in] pData The pointer to shallow copy
     *  @param[in] MUTEX_IN The mutex reference
     *
     **/
    decrypted_proxy(TypeName* pData, std::recursive_mutex& MUTEX_IN, std::atomic<int>& nRefsIn)
    : MUTEX(MUTEX_IN)
    , data(pData)
    , nRefs(nRefsIn)
    {
        MUTEX.lock();

        ++nRefs;

        if(nRefs == 1)
            crypt(data);

        debug::log(0, "Refs ", nRefs.load());
    }


    /** Destructor
    *
    *  Unlock the mutex.
    *
    **/
    ~decrypted_proxy()
    {
        --nRefs;

        if(nRefs == 0)
            crypt(data);

        MUTEX.unlock();
    }


    /** Member Access Operator.
    *
    *  Access the memory of the raw pointer.
    *
    **/
    TypeName* operator->() const
    {
        if(data == nullptr)
            throw std::runtime_error(debug::safe_printstr(FUNCTION, "member access to nullptr"));

        return data;
    }
};


/** encrypted_ptr
 *
 *  Protects a pointer with a mutex.
 *  Encrypts / Decrypts data dereferenced from the pointer.
 *
 **/

template<class TypeName>
class encrypted_ptr
{
    /** The internal locking mutex. **/
    mutable std::recursive_mutex MUTEX;


    /** The internal raw poitner. **/
    TypeName* data;


    std::atomic<int> nRefs;


public:

    /** Default Constructor. **/
    encrypted_ptr()
    : MUTEX()
    , data(nullptr)
    , nRefs(0)
    {
    }


    /** Copy Constructor. **/
    encrypted_ptr(const encrypted_ptr<TypeName>& pointer) = delete;


    /** Move Constructor. **/
    encrypted_ptr(const encrypted_ptr<TypeName>&& pointer)
    : data(pointer.data)
    {
    }


    /** Destructor. **/
    ~encrypted_ptr()
    {
    }


    /** Assignment operator. **/
    encrypted_ptr& operator=(const encrypted_ptr<TypeName>& dataIn) = delete;


    /** Assignment operator. **/
    encrypted_ptr& operator=(TypeName* dataIn) = delete;


    /** Equivilent operator.
     *
     *  @param[in] a The data type to compare to.
     *
     **/
    bool operator==(const TypeName& dataIn) const
    {
        RLOCK(MUTEX);

        /* Throw an exception on nullptr. */
        if(data == nullptr)
            return false;

        crypt(data);
        bool fEquals = (*data == dataIn);
        crypt(data);

        return fEquals;
    }


    /** Not operator
     *
     *  Check if the pointer is nullptr.
     *
     **/
    bool operator!(void)
    {
        RLOCK(MUTEX);

        return data == nullptr;
    }


    /** Member access overload
     *
     *  Allow atomic_ptr access like a normal pointer.
     *
     **/
    decrypted_proxy<TypeName> operator->()
    {
        return decrypted_proxy<TypeName>(data, MUTEX, nRefs);
    }


    /** dereference operator overload
     *
     *  Load the object from memory.
     *
     **/
    TypeName operator*() const
    {
        RLOCK(MUTEX);

        /* Throw an exception on nullptr. */
        if(data == nullptr)
            throw std::runtime_error(debug::safe_printstr(FUNCTION, "dereferencing a nullptr"));

        crypt(data);
        size_t nSize = sizeof(*data);
        std::vector<uint8_t> vchData(nSize);

        std::copy((uint8_t*)data, (uint8_t*)data + nSize, (uint8_t*)&vchData[0]);

        crypt(data);

        TypeName type;
        std::copy((uint8_t*)&vchData[0], (uint8_t*)&vchData[0] + nSize, (uint8_t*)&type);

        return type;
    }


    /** store
     *
     *  Stores an object into memory.
     *
     *  @param[in] dataIn The data into protected memory.
     *
     **/
    void store(TypeName* dataIn)
    {
        RLOCK(MUTEX);

        if(data)
            delete data;

        data = dataIn;

        crypt(data);
    }


    /** reset
     *
     *  Reset the internal memory of the atomic pointer.
     *
     **/
    void free()
    {
        RLOCK(MUTEX);

        if(data)
            delete data;

        data = nullptr;
    }
};

struct Test
{
    uint32_t a;
    uint32_t b;
    uint32_t c;

    uint256_t hash;
};


int main(int argc, char **argv)
{
    Test test;

    RAND_bytes((uint8_t*)&vKey[0], vKey.size());

    debug::log(0, FUNCTION, "Symmetric Key is ", HexStr(vKey.begin(), vKey.end()));
    debug::log(0, FUNCTION, "Running live tests");

    test.a    = 55;
    test.b    = 77;
    test.c    = 66;

    test.hash = LLC::GetRand256();

    debug::log(0, "A ", test.a, " B ", test.b, " C ", test.c, " Hash ", test.hash.ToString());

    encrypted_ptr<Test> ptr;
    ptr.store(&test);

    debug::log(0, "A ", test.a, " B ", test.b, " C ", test.c, " Hash ", test.hash.ToString());

    debug::log(0, "A ", ptr->a, " B ", ptr->b, " C ", ptr->c, " Hash ", ptr->hash.ToString());


    return 0;

    TAO::Ledger::SignatureChain* user = new TAO::Ledger::SignatureChain("colin", "passing");
    uint512_t hashGenerate = user->Generate(0, "1234");

    debug::log(0, hashGenerate.ToString());

    uint512_t hashGenerate2 = user->Generate(0, "1234");

    debug::log(0, hashGenerate2.ToString());

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
