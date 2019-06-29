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
#include <LLC/hash/SK.h>
#include <LLC/include/flkey.h>
#include <LLC/types/bignum.h>

#include <Util/include/hex.h>

#include <iostream>

#include <TAO/Register/types/address.h>

#include <list>

/** Address
 *
 *  An object that keeps track of a register address type.
 *
 **/
class Genesis : public uint256_t
{
public:

    /** Default constructor. **/
    Genesis()
    : uint256_t(0)
    {
    }


    /** Copy Constructor */
    Genesis(const uint256_t& hashAddress)
    : uint256_t(hashAddress)
    {
    }


    /** Address Constructor
     *
     *  Build an address from a hex encoded string.
     *
     *  @param[in] strName The name to assign to this address.
     *  @param[in] nType The type of the address (Name or Namespace)
     *
     **/
    Genesis(const std::string& strAddress)
    : uint256_t(strAddress)
    {
        /* Check for valid address types. */
        if(!IsValid())
            throw debug::exception(FUNCTION, "invalid type");
    }


    /** Assignment operator.
     *
     *  @param[in] addr Address to assign this to.
     *
     **/
    Genesis& operator=(const Genesis& addr)
    {
        /* Copy each word. */
        for(uint32_t i = 0; i < WIDTH; ++i)
            pn[i] = addr.pn[i];

        return *this;
    }


    /** SetType
     *
     *  Set the type byte into the address.
     *
     *  @param[in] nType The type byte for address.
     *
     **/
    void SetType(uint8_t nType)
    {
        /* Check for testnet. */
        if(config::fTestNet.load())
            ++nType;

        /* Mask off most significant byte (little endian). */
        pn[WIDTH -1] = (pn[WIDTH - 1] & 0x00ffffff) + (nType << 24);
    }


    /** GetType
     *
     *  Get the type byte from the address.
     *
     *  @param[in] nType The type byte for address.
     *
     **/
    uint8_t GetType() const
    {
        /* Get the type. */
        uint8_t nType = (pn[WIDTH -1] >> 24);

        /* Check for testnet. */
        if(config::fTestNet.load())
            --nType;

        return nType;
    }


    /** IsValid
     *
     *  Check if genesis has a valid indicator byte.
     *
     *  @return True if type has valid header byte.
     *
     **/
    bool IsValid() const
    {
        return GetType() == 0xa1;
    }
};


/* This is for prototyping new code. This main is accessed by building with LIVE_TESTS=1. */
int main(int argc, char** argv)
{
    config::fTestNet.store(true);

    uint256_t hashTest = LLC::GetRand256();

    debug::log(0, "Hash: ", hashTest.ToString());

    TAO::Register::Address addr = hashTest;

    addr.SetType(TAO::Register::Address::NAME);

    debug::log(0, "Hash: ", addr.ToString());

    printf("BYTE: %x\n", addr.GetType());

    debug::log(0, "Valid: ", addr.IsValid() ? "Yes" : "No");

    TAO::Register::Address name = TAO::Register::Address("colin2", TAO::Register::Address::NAMESPACE);

    debug::log(0, "Hash: ", name.ToString());


    return 0;
}
