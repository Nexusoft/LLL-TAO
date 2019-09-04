/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLD_KEYCHAIN_KEYCHAIN_H
#define NEXUS_LLD_KEYCHAIN_KEYCHAIN_H

#include <LLD/templates/key.h>

namespace LLD
{

    /** Keychain
     *
     *  Pure abstract base class for all keychain objects.
     *  Ensure support for core methods for all keychains.
     *
     **/
    class Keychain
    {
    public:

        /** Virtual Destructor. **/
        virtual ~Keychain() {}


        /** Get
         *
         *  Read a key from the keychain.
         *
         *  @param[in] vKey The binary data of key.
         *  @param[out] cKey The key object to return.
         *
         *  @return True if the key was found, false otherwise.
         *
         **/
        virtual bool Get(const std::vector<uint8_t>& vKey, SectorKey &cKey) = 0;


        /** Put
         *
         *  Write a key to the keychain.
         *
         *  @param[in] cKey The key object to write.
         *
         *  @return True if the key was written, false otherwise.
         *
         **/
        virtual bool Put(const SectorKey& cKey) = 0;


        /** Flush
         *
         *  Flush all buffers to disk if using ACID transaction.
         *
         **/
        virtual void Flush() = 0;


        /** Restore
         *
         *  Restore an erased key from keychain.
         *
         *  @param[in] vKey the key to restore.
         *
         *  @return True if the key was restored.
         *
         **/
        virtual bool Restore(const std::vector<uint8_t>& vKey) = 0;


        /** Erase
         *
         *  Erase a key from the keychain
         *
         *  @param[in] vKey the key to erase.
         *
         *  @return True if the key was erased, false otherwise.
         *
         **/
        virtual bool Erase(const std::vector<uint8_t>& vKey) = 0;
    };
}

#endif
