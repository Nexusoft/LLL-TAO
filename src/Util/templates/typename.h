/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2023

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once

#include <LLC/types/uint1024.h>

namespace util::types
{
    /* Output the string names using type overloading to help with templates. */
    inline std::string name(uint8_t n)     { return "uint8_t";    }
    inline std::string name(uint16_t n)    { return "uint16_t";   }
    inline std::string name(uint32_t n)    { return "uint32_t";   }
    inline std::string name(uint64_t n)    { return "uint64_t";   }
    inline std::string name(uint256_t n)   { return "uint256_t";  }
    inline std::string name(uint512_t n)   { return "uint512_t";  }
    inline std::string name(uint1024_t n)  { return "uint1024_t"; }

    inline std::string name(int8_t n)     { return "int8_t";    }
    inline std::string name(int16_t n)    { return "int16_t";   }
    inline std::string name(int32_t n)    { return "int32_t";   }
    inline std::string name(int64_t n)    { return "int64_t";   }

    inline std::string name(float n)   { return "float";  }
    inline std::string name(double n)  { return "double"; }
}
