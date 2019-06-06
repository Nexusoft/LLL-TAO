/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/types/uint1024.h>
#include <LLC/hash/SK.h>
#include <LLC/include/random.h>

#include <LLD/cache/binary_lfu.h>

#include <Util/include/debug.h>
#include <Util/include/base64.h>

#include <openssl/rand.h>

#include <LLC/hash/argon2.h>
#include <LLC/include/flkey.h>

#include <iostream>

struct Node
{
    Node* pprev;
    Node* pnext;

    uint64_t nValue;

    Node(const uint64_t nValueIn)
    : pprev(nullptr)
    , pnext(nullptr)
    , nValue(nValueIn)
    {}
};

Node* pfirst = nullptr;
Node* plast  = nullptr;

void MoveForward(Node* pthis)
{
    /* Don't move to front if already in the front. */
    if(pthis == pfirst)
        return;

    /* Check for previous. */
    if(!pthis->pprev)
        return;

    /* Move last pointer if moving from back. */
    Node* pprev = pthis->pprev;

    /* Set the right link. */
    if(pprev->pprev)
        pprev->pprev->pnext = pthis;
    else
        pfirst = pthis;

    /* Re-link previous. */
    pprev->pnext = pthis->pnext;
    pprev->pnext->pprev = pprev;

    /* Set node's new prev and next. */
    pthis->pprev = pprev->pprev;
    pthis->pnext = pprev;

}


/* Proxy to handle locked LLD records. */
class LockProxy
{
    const std::mutex& MUTEX;

    LockProxy(const std::mutex& MUTEX_IN)
    : MUTEX(MUTEX_IN)
    {
        MUTEX.lock();
    }
};

/* This is for prototyping new code. This main is accessed by building with LIVE_TESTS=1. */
int main(int argc, char** argv)
{
    config::ParseParameters(argc, argv);

    std::string strRecovery = "";
    if(config::GetBoolArg("-generate"))
    {
        //https://github.com/A35G/gmCaptcha/blob/master/dictionary/1.1million%20word%20list.txt
        std::ifstream stream(config::GetDataDir() + "/wordlist.txt", std::ios::in | std::ios::out);

        std::vector<std::string> lines;

        std::string strLine = "";
        while(std::getline(stream, strLine))
            lines.push_back(strLine.substr(0, strLine.size() - 1));

        uint64_t nSize = lines.size();

        debug::log(0, nSize, " Total Words");


        for(int i = 0; i < 20; ++i)
        {
            uint64_t nRand = LLC::GetRand();

            strRecovery += lines[nRand % nSize];
            if(i < 19)
                strRecovery += " ";
        }
    }
    else
    {
        for(int i = 1; i < argc; ++i)
        {
            strRecovery += argv[i];

            if(i < argc - 1)
                strRecovery += " ";
        }
    }

    debug::log(0, strRecovery);

    std::string strUsername = "jack";

    /* Generate the Secret Phrase */
    std::vector<uint8_t> vUsername(strUsername.begin(), strUsername.end());
    if(vUsername.size() < 8)
        vUsername.resize(8);

    std::vector<uint8_t> vRecovery(strRecovery.begin(), strRecovery.end());


    // low-level API
    std::vector<uint8_t> vHash(64);
    std::vector<uint8_t> vSalt(16); //TODO: possibly make this your birthday (required in API)

    /* Create the hash context. */
    argon2_context context =
    {
        /* Hash Return Value. */
        &vHash[0],
        64,

        /* Password input data. */
        &vRecovery[0],
        static_cast<uint32_t>(vRecovery.size()),

        /* Password input data. */
        &vUsername[0],
        static_cast<uint32_t>(vUsername.size()),

        /* The salt for usernames */
        &vSalt[0],
        static_cast<uint32_t>(vSalt.size()),

        /* Optional associated data */
        NULL, 0,

        /* Computational Cost. */
        108,

        /* Memory Cost (64 MB). */
        (1 << 18),

        /* The number of threads and lanes */
        1, 1,

        /* Algorithm Version */
        ARGON2_VERSION_13,

        /* Custom memory allocation / deallocation functions. */
        NULL, NULL,

        /* By default only internal memory is cleared (pwd is not wiped) */
        ARGON2_DEFAULT_FLAGS
    };

    /* Run the argon2 computation. */
    int32_t nRet = argon2id_ctx(&context);
    if(nRet != ARGON2_OK)
        throw std::runtime_error(debug::safe_printstr(FUNCTION, "Argon2 failed with code ", nRet));

    /* Set the bytes for the key. */
    uint512_t hashKey;
    hashKey.SetBytes(vHash);

    /* Get the secret from new key. */
    std::vector<uint8_t> vBytes = hashKey.GetBytes();
    LLC::CSecret vchSecret(vBytes.begin(), vBytes.end());

    /* Create the FL Key object. */
    LLC::FLKey key;

    /* Set the secret key. */
    if(!key.SetSecret(vchSecret, true))
        return false;

    /* Calculate the next hash. */
    uint256_t hashNext = LLC::SK256(key.GetPubKey());

    /* Output recovery private key. */
    debug::log(0, "Recovery Hash: ", hashKey.ToString());


    return 0;


    LLD::BinaryLFU* cache = new LLD::BinaryLFU(512);

    for(int i = 0; i < 1000000; ++i)
    {

        uint256_t hashKey = LLC::GetRand256();

        uint512_t hashData = LLC::GetRand512();

        cache->Put(hashKey.GetBytes(), hashData.GetBytes());

        std::vector<uint8_t> vBytes;
        if(!cache->Get(hashKey.GetBytes(), vBytes))
            continue;

        uint512_t hashData2;
        hashData2.SetBytes(vBytes);

        debug::log(0, "data=", hashData.ToString());
        debug::log(0, "data=", hashData2.ToString());
    }


    std::vector<Node*> vItems;
    for(int i = 0; i < 100; ++i)
    {
        vItems.push_back(new Node(i));

        if(!plast)
        {
            plast = vItems[i];

            continue;
        }

        if(!pfirst)
        {
            pfirst        = vItems[i];
            plast->pprev  = pfirst;
            pfirst->pnext = plast;

            continue;
        }

        pfirst->pprev = vItems[i];
        pfirst->pprev->pnext = pfirst;

        pfirst = vItems[i];
    }

    {
        Node* pnode = plast;
        while(pnode)
        {
            debug::log(0, "Node: ", pnode->nValue);
            pnode = pnode->pprev;
        }
    }

    debug::log(0, "Forward.....");

    {
        Node* pnode = pfirst;
        while(pnode)
        {
            debug::log(0, "Node: ", pnode->nValue);
            pnode = pnode->pnext;
        }
    }


    for(int i = 0; i < 100; ++i)
    {
        MoveForward(vItems[88]);

        debug::log(0, "Move.....");
        {
            Node* pnode = pfirst;
            while(pnode)
            {
                debug::log(0, "Node: ", pnode->nValue);
                pnode = pnode->pnext;
            }
        }
    }



    return 0;
}
