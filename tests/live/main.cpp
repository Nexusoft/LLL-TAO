/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/types/uint1024.h>
#include <LLC/include/random.h>

#include <LLD/cache/binary_lfu.h>

#include <Util/include/debug.h>

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
    Node* pnext = pthis->pnext;

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

/* This is for prototyping new code. This main is accessed by building with LIVE_TESTS=1. */
int main(int argc, char** argv)
{

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
