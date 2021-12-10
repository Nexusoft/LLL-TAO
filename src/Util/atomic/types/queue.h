/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once

#include <atomic>

namespace util::atomic
{
	/** Lockless and Atomic Queue.
     *
     *  A lock-free and wait-free atomic queue for high performance shared data.
     *
     **/
    template<typename Type> class queue
    {
        /** Node Struct
         *
         *  Internal structure to store node data and pointers.
         *
         **/
        struct Node
        {
            /** The raw data. **/
            Type tData;

            /** The pointer to next. */
            std::atomic<Node*> pNext;

            /** Basic Constructor. **/
            Node()
            : tData ( )
            , pNext (nullptr)
            {
            }

            /** Default Constructor
             *
             *  Construct new node based on data input.
             *
             *  @param[in] rData The data input.
             *
             **/
            Node(const Type& rData)
            : tData (rData)
            , pNext (nullptr)
            {
            }
        };


        /** The head of the single linked list. **/
        std::atomic<Node*> aHead;


        /** The tail of the single linked list. **/
        std::atomic<Node*> aTail;


        /** Atomic to keep track of queue's size. **/
        std::atomic<size_t> nSize;


    public:


        /** Basic Constructor. **/
        queue()
        : aHead (nullptr)
        , aTail (nullptr)
        , nSize (0)
        {
        }


        /** front
         *
         *  Get the first element in the queue.
         *
         *  @return a const reference to first item.
         *
         **/
        const Type& front() const
        {
            return aHead.load()->tData;
        }


        /** back
         *
         *  Get the last element in the queue.
         *
         *  @return a const reference to first item.
         *
         **/
        const Type& back() const
        {
            return aTail.load()->tData;
        }


        /** size
         *
         *  Get the total number of elements in the queue.
         *
         *  @return the current size of the queue.
         *
         **/
        size_t size() const
        {
            return nSize.load();
        }


        /** empty
         *
         *  Detect if the queue is currently empty.
         *
         *  @return true if the queue is empty.
         *
         **/
        bool empty() const
        {
            return (nSize.load() == 0);
        }


        /** push
         *
         *  Push a new item to the back of the queue.
         *
         *  @param[in] rData a reference to the data to add to the queue.
         *
         *  @return the current size of the queue.
         *
         **/
        void push(const Type& rData)
        {
            /* Create our new node to add to the queue. */
    		Node* pNode = new Node(rData);

            /* Check for our first insert. */
            if(aHead.load() == nullptr)
            {
                /* Set our initial head and tail values to new element. */
                aHead.store(pNode);
                aTail.store(pNode);

                /* Incrememnt our size atomically. */
                ++nSize;

                return;
            }

            /* We need this to hold our tail value for CAS for our tail pointer. */
            Node* pTail;
            while(true)
            {
                /* Load our current tail pointer. */
                pTail = aTail.load();

                /* Check that tail doesn't have a next node. */
                Node* pTailNext = pTail->pNext.load();
                if(pTailNext)
                {
                    /* Bring tail forward to next node. */
                    while(!aTail.compare_exchange_weak(pTail, pTailNext));
                    continue;
                }

                /* Swing our tail next pointer forward to our new node. */
                if(pTail->pNext.compare_exchange_weak(pTailNext, pNode))
                    break;
            }

            /* Swing our new tail pointer to our new node. */
            while(!aTail.compare_exchange_weak(pTail, pNode));

            /* Incrememnt our size atomically. */
            ++nSize;
        }


        /** pop
         *
         *  Pop an element off the front of the queue.
         *
         **/
        void pop()
        {
            /* Grab our current head. */
            Node* pHead = aHead.load();

            /* Loop to handle our CAS since we are using weak which can spuriously fail. */
            while(true)
            {
                /* Load our next head pointer. */
                Node* pHeadNext = pHead->pNext.load();

                /* Swing our new atomic head to our next pointer. */
                if(aHead.compare_exchange_weak(pHead, pHeadNext))
                    break;
            }

            /* Decrement our size atomically. */
            --nSize;
        }


        /** print
         *
         *  Print all elements in the queue. FOR DEBUGGING ONLY
         *
         **/
        void print() const
        {
            uint32_t nCount = 0;
            Node* pStart = aHead.load();

            while(pStart != nullptr)
            {
                debug::log(0, "[", nCount++, "] ", pStart->tData);
                pStart = pStart->pNext;
            }
        }
    };
}
