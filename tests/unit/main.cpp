#include <Util/include/debug.h>
#include <Util/include/hex.h>

#include <Util/include/runtime.h>

#include <LLC/falcon/falcon.h>

#include <TAO/Ledger/types/sigchain.h>

class FLKey
{
    /* The contained falcon public key. */
    std::vector<uint8_t> vchPubKey;

    /* the contained falcon private key. */
    std::vector<uint8_t> vchPrivKey;


    uint32_t nLog;


    uint32_t nSigSize;

public:
    FLKey()
    : vchPubKey()
    , vchPrivKey()
    , nLog(9)
    , nSigSize()
    {
        if(nLog == 9)
            nSigSize = 1025;
        else if(nLog == 10)
            nSigSize = 2049;
        else
            throw std::runtime_error("Invalid log input for FL Key");
    }

    void SetPubKey(const std::vector<uint8_t>& vchPubKeyIn)
    {
        vchPubKey = vchPubKeyIn;
    }


    std::vector<uint8_t> GetPubKey()
    {
        return vchPubKey;
    }


    void SetSecret(const std::vector<uint8_t>& vchSecret)
    {
        falcon_keygen* fk = falcon_keygen_new(nLog, 0);

        falcon_keygen_set_seed(fk, &vchSecret[0], vchSecret.size(), 1);

        size_t privkey_size = falcon_keygen_max_privkey_size(fk);
        size_t pubkey_size  = falcon_keygen_max_pubkey_size(fk);

        vchPubKey.resize(pubkey_size);
        vchPrivKey.resize(privkey_size);

        int ret = falcon_keygen_make(fk, FALCON_COMP_STATIC, &vchPrivKey[0], &privkey_size, &vchPubKey[0], &pubkey_size);

        vchPrivKey.resize(privkey_size);
        vchPubKey.resize(pubkey_size);

        falcon_keygen_free(fk);
    }


    bool Sign(const std::vector<uint8_t>& vchData, std::vector<uint8_t>& vchSignature)
    {
        vchSignature.clear();
        vchSignature.resize(nSigSize);

        falcon_sign* fs = falcon_sign_new();

        if(falcon_sign_set_private_key(fs, &vchPrivKey[0], vchPrivKey.size()) != 1)
        {
            falcon_sign_free(fs);
            return false;
        }

        std::vector<uint8_t> nonce(40);
        if(falcon_sign_start(fs, &nonce[0]) != 1)
        {
            falcon_sign_free(fs);
            return false;
        }

        falcon_sign_update(fs, &vchData[0], vchData.size());

        if(falcon_sign_generate(fs, &vchSignature[0], vchSignature.size(), 0) != vchSignature.size())
        {
            falcon_sign_free(fs);
            return false;
        }

        vchSignature.insert(vchSignature.end(), nonce.begin(), nonce.end());

        falcon_sign_free(fs);

        return true;
    }

    bool Verify(const std::vector<uint8_t>& vchData, const std::vector<uint8_t>& vchSignature)
    {
        falcon_vrfy* fv = falcon_vrfy_new();

        if(falcon_vrfy_set_public_key(fv, &vchPubKey[0], vchPubKey.size()) != 1)
        {
            falcon_vrfy_free(fv);
            return false;
        }

        falcon_vrfy_start(fv, &vchSignature[nSigSize], 40);

        falcon_vrfy_update(fv, &vchData[0], vchData.size());

        if(falcon_vrfy_verify(fv, &vchSignature[0], nSigSize) != 1)
        {
            falcon_vrfy_free(fv);
            return false;
        }

        return true;
    }
};


std::atomic<uint64_t> nVerified;

std::vector<uint8_t> vchPubKey;

std::vector<uint8_t> vchSignature;

std::vector<uint8_t> vchMessage;

void Verifier()
{
    while(true)
    {
        FLKey key2;
        key2.SetPubKey(vchPubKey);
        if(!key2.Verify(vchMessage, vchSignature))
            debug::error(FUNCTION, "failed to verify");

        ++nVerified;
    }
}

int main(int argc, char **argv)
{
    debug::log(0, FUNCTION, "Running live tests");

    TAO::Ledger::SignatureChain* user = new TAO::Ledger::SignatureChain("colin", "passing");
    uint512_t hashGenerate = user->Generate(0, "1234");

    runtime::timer timer;
    timer.Start();
    FLKey key;
    key.SetSecret(hashGenerate.GetBytes());
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

    std::thread t1 = std::thread(Verifier);
    std::thread t2 = std::thread(Verifier);
    std::thread t3 = std::thread(Verifier);
    std::thread t4 = std::thread(Verifier);
    std::thread t5 = std::thread(Verifier);
    std::thread t6 = std::thread(Verifier);

    while(true)
    {
        runtime::sleep(1000);

        debug::log(0, "LLC Verified ", nVerified.load(), " signatures per second");

        nVerified = 0;
    }

    debug::log(0, FUNCTION, "Passed (", vchPubKey.size() + vchSignature.size(), " bytes)");

    return 0;
}
