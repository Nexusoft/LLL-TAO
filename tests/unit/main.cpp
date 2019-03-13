#include <Util/include/debug.h>
#include <Util/include/hex.h>

#include <LLC/falcon/falcon.h>

#include <TAO/Ledger/types/sigchain.h>

class FLKey
{
    /* The contained falcon public key. */
    std::vector<uint8_t> vchPubKey;

    /* the contained falcon private key. */
    std::vector<uint8_t> vchPrivKey;

public:
    FLKey()
    : vchPubKey()
    , vchPrivKey()
    {

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
        falcon_keygen* fk = falcon_keygen_new(10, 0);

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
        vchSignature.resize(2049);

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

        falcon_vrfy_start(fv, &vchSignature[2049], 40);

        falcon_vrfy_update(fv, &vchData[0], vchData.size());

        if(falcon_vrfy_verify(fv, &vchSignature[0], 2049) != 1)
        {
            falcon_vrfy_free(fv);
            return false;
        }

        return true;
    }
};

int main(int argc, char **argv)
{
    debug::log(0, FUNCTION, "Running live tests");

    TAO::Ledger::SignatureChain* user = new TAO::Ledger::SignatureChain("colin", "passing");
    uint512_t hashGenerate = user->Generate(0, "1234");

    FLKey key;
    key.SetSecret(hashGenerate.GetBytes());

    std::vector<uint8_t> vchSignature;

    uint256_t hashRandom = 55;
    if(!key.Sign(hashRandom.GetBytes(), vchSignature))
        return debug::error(FUNCTION, "failed to sign");

    std::vector<uint8_t> vchPubKey = key.GetPubKey();

    FLKey key2;
    key2.SetPubKey(vchPubKey);
    if(!key2.Verify(hashRandom.GetBytes(), vchSignature))
        return debug::error(FUNCTION, "failed to verify");

    debug::log(0, FUNCTION, "Passed");

    return 0;
}
