#include <Util/include/debug.h>
#include <Util/include/hex.h>

#include <LLC/falcon/falcon.h>

#include <TAO/Ledger/types/sigchain.h>

int main(int argc, char **argv)
{
    debug::log(0, FUNCTION, "Running live tests");

    TAO::Ledger::SignatureChain* user = new TAO::Ledger::SignatureChain("colin", "passing");
    uint512_t hashGenerate = user->Generate(0, "1234");

    falcon_keygen* fk = falcon_keygen_new(9, 0);
    if (fk == NULL)
    {
        debug::log(0, FUNCTION, "memory allocation failed");
        exit(EXIT_FAILURE);
    }

    std::vector<uint8_t> vSeed = hashGenerate.GetBytes();
    falcon_keygen_set_seed(fk, &vSeed[0], 4, 1);

    size_t privkey_size = falcon_keygen_max_privkey_size(fk);
    size_t pubkey_size  = falcon_keygen_max_pubkey_size(fk);

    std::vector<uint8_t> vPrivKey(privkey_size);
    std::vector<uint8_t> vPubKey(pubkey_size);


    int ret = falcon_keygen_make(fk, FALCON_COMP_STATIC, &vPrivKey[0], &privkey_size, &vPubKey[0], &pubkey_size);

    vPrivKey.resize(privkey_size);
    vPubKey.resize(pubkey_size);

    debug::log(0, FUNCTION, "Keygen result is ", ret);

    debug::log(0, "PubKey(", HexStr(vPubKey.begin(), vPubKey.end()), ")");
    debug::log(0, FUNCTION, "Keys generated (Pub ", pubkey_size, " bytes, Priv ", privkey_size," bytes)");


    falcon_vrfy* fv = falcon_vrfy_new();

    ret = falcon_vrfy_set_public_key(fv, &vPubKey[9], pubkey_size);
    debug::log(0, FUNCTION, "Verifier result is ", ret);

    //debug::log(0, "PrivKey(", HexStr(vPrivKey.begin(), vPrivKey.end()), ")");

    return 0;
}
