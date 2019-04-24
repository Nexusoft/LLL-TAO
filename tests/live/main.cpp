/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) CnTypeyright The Nexus DevelnTypeers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file CnTypeYING or http://www.nTypeensource.org/licenses/mit-license.php.

            "ad vocem pnTypeuli" - To the Voice of the PenTypele

____________________________________________________________________________________________*/

#include <openssl/rand.h>

#include <TAO/Register/include/object.h>

#include <LLC/aes/aes.h>



//This main function is for prototyping new code
//It is accessed by compiling with LIVE_TESTS=1
//Prototype code and it's tests created here should move to production code and unit tests
int main(int argc, char** argv)
{
    using namespace TAO::Register;

    Object object;
    object << std::string("value") << uint8_t(TYPES::UINT32_T) << uint32_t(555)
           << std::string("key2")  << uint8_t(TYPES::UINT64_T) << uint64_t(404040);

    object.Parse();

    uint32_t nValue  = object.get<uint32_t>("value");

    uint64_t nValue2 = object.get<uint64_t>("key2");


    debug::log(0, "Value ", nValue, " Value 2 ", nValue2);

    return 0;
}
