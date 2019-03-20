#include <Util/include/debug.h>
#include <Util/include/hex.h>

#include <Util/include/runtime.h>

#include <LLC/include/flkey.h>

#include <TAO/Ledger/types/sigchain.h>

#include <Util/include/memory.h>

#include <LLC/include/random.h>

#include <openssl/rand.h>

#include <LLC/aes/aes.h>


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


struct Test
{
    uint32_t a;
    uint32_t b;
    uint32_t c;

    uint256_t hash;
};


int main(int argc, char **argv)
{
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
