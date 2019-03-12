#include <Util/include/debug.h>
#include <Util/include/hex.h>

#include <LLC/falcon/falcon.h>

#include <TAO/Ledger/types/sigchain.h>

int main(int argc, char **argv)
{
    debug::log(0, FUNCTION, "Running live tests");

    TAO::Ledger::SignatureChain* user = new TAO::Ledger::SignatureChain("colin", "passing");
    uint512_t hashGenerate = user->Generate(0, "1234");

    falcon_keygen* fk = falcon_keygen_new(10, 0);
    if (fk == NULL)
    {
        debug::log(0, FUNCTION, "memory allocation failed");
        exit(EXIT_FAILURE);
    }

    std::vector<uint8_t> vSeed = hashGenerate.GetBytes();
    falcon_keygen_set_seed(fk, &vSeed[0], vSeed.size(), 1);

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


    falcon_sign* fs = falcon_sign_new();

    ret = falcon_sign_set_private_key(fs, &vPrivKey[0], privkey_size);
    debug::log(0, FUNCTION, "Signer result is ", ret);

    std::vector<uint8_t> nonce(40);
    ret = falcon_sign_start(fs, &nonce[0]);
    debug::log(0, FUNCTION, "Nonce result is ", ret);
    debug::log(0, "Nonce(", HexStr(nonce.begin(), nonce.end()), ")");


    std::vector<uint8_t> vMsg = { 0xff, 0xfe, 0xfd, 0xfc };
    falcon_sign_update(fs, &vMsg[0], vMsg.size());

    std::vector<uint8_t> vSig(2049);
    int nSize = falcon_sign_generate(fs, &vSig[0], vSig.size(), 0);
    debug::log(0, FUNCTION, "Signature Generate is ", nSize);
    debug::log(0, "Signature(", HexStr(vSig.begin(), vSig.end()), ")");

    //void falcon_sign_start_external_nonce(falcon_sign *fs, const void *r, size_t rlen);

    falcon_vrfy* fv = falcon_vrfy_new();

    ret = falcon_vrfy_set_public_key(fv, &vPubKey[0], pubkey_size);
    debug::log(0, FUNCTION, "Verifier result is ", ret);

    falcon_vrfy_start(fv, &nonce[0], nonce.size());

    falcon_vrfy_update(fv, &vMsg[0], vMsg.size());

    ret = falcon_vrfy_verify(fv, &vSig[0], vSig.size());
    debug::log(0, FUNCTION, "Signature Verified is ", ret);



    //debug::log(0, "PrivKey(", HexStr(vPrivKey.begin(), vPrivKey.end()), ")");

    return 0;
}
