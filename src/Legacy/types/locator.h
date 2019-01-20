/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_LEGACY_TYPES_LOCATOR_H
#define NEXUS_LEGACY_TYPES_LOCATOR_H

#include <Util/templates/serialize.h>

/* Global Legacy namespace. */
namespace Legacy
{

    /** Legacy object to handle legacy serialization. **/
    class Locator
    {
    public:
        int32_t nVersion;
        std::vector<uint1024_t> vHave;

        IMPLEMENT_SERIALIZE
        (
            READWRITE(nVersion);
            READWRITE(vHave);
        )

        Locator()
        {
        }

        explicit Locator(const uint1024_t& hashBlock)
        {
            vHave.push_back(hashBlock);
        }
    };
}

#endif
