
#include "TAO/Ledger/include/sigchain.h"
#include "TAO/Register/include/state.h"

#include "Util/include/debug.h"
#include "Util/include/runtime.h"
#include "Util/include/hex.h"

#include "LLC/hash/SK.h"
#include "LLC/include/random.h"
#include "LLC/include/key.h"

#include "LLP/templates/server.h"

#include "LLD/templates/sector.h"
#include "LLD/templates/hashmap.h"
#include "LLD/templates/filemap.h"

#include <sys/time.h>

#include "LLP/templates/server.h"
#include "LLP/include/legacy.h"



class CAccount
{
    std::vector<unsigned char> vchIdentifier;

    uint256 hashAddress;

    uint64 nBalance;

    IMPLEMENT_SERIALIZE
    (
        READWRITE(hashAddress);
        READWRITE(nBalance);
        READWRITE(FLATDATA(vchIdentifier));
    )

    CAccount() : hashAddress(0), nBalance(0)
    {

    }

};



class RegisterDB : public LLD::SectorDatabase<LLD::BinaryFileMap>
{
    RegisterDB(const char* pszMode="r+") : SectorDatabase("regdb", pszMode) {}

    bool ReadRegister(uint256 address, TAO::Register::CStateRegister& regState)
    {
        return Read(address, regState);
    }

    bool WriteRegister(uint256 address, TAO::Register::CStateRegister regState)
    {
        return Write(address, regState);
    }
};


int main(int argc, char** argv)
{
    ParseParameters(argc, argv);

    LLP::Server<LLP::LegacyNode>* SERVER = new LLP::Server<LLP::LegacyNode>(9323, 10, 30);

    SERVER->AddConnection("104.192.170.130", "9323");
    SERVER->AddConnection("104.192.169.10",  "9323");

    while(true)
    {
        Sleep(10);
    }

    return 0;

    for(int i = 0; ; i++)
    {
        uint512 hashSeed = GetRand512();
        uint256 hash256  = GetRand256();

        TAO::Ledger::SignatureChain sigChain("username", hashSeed.ToString());
        uint512 hashGenesis = sigChain.Generate(i, hash256.ToString());

        printf("Genesis Priv %s\n", hashGenesis.ToString().c_str());

        LLC::CKey key(NID_brainpoolP512t1, 64);

        std::vector<unsigned char> vchData = hashGenesis.GetBytes();

        LLC::CSecret vchSecret(vchData.begin(), vchData.end());
        if(!key.SetSecret(vchSecret, true))
            return 0;

        LLC::CKey key512(key);

        std::vector<unsigned char> vPubKey = key.GetPubKey();

        printf("Genesis Pub %s\n", HexStr(vPubKey).c_str());

        uint512 genesisID = LLC::HASH::SK512(vPubKey);

        printf("Genesis ID %s\n", genesisID.ToString().c_str());

        printf("Keys Created == %s\n", (key == key512) ? "TRUE" : "FALSE");

        std::vector<unsigned char> vchSig;
        bool fSigned = key512.Sign(hashSeed, vchSig, 256);
        //vchSig[0] = 0x11;

        LLC::CKey keyVerify(NID_brainpoolP512t1, 64);
        keyVerify.SetPubKey(vPubKey);

        bool fVerify = keyVerify.Verify(hashSeed, vchSig, 256);
        printf("[%i] Signature %s [%u Bytes] Signed %s and Verified %s\n", i, HexStr(vchSig).c_str(), vchSig.size(), fSigned ? "True" : "False", fVerify ? "True" : "False");

        if(!fVerify)
            return 0;
    }

    return 0;
}
