/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2017] ++

            (c) Copyright The Nexus Developers 2014 - 2017

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "fides in stellis, virtus in numeris" - Faith in the Stars, Power in Numbers

____________________________________________________________________________________________*/

#ifndef NEXUS_UTIL_MACRO_HEADER_H
#define NEXUS_UTIL_MACRO_HEADER_H


//Use this in the header file to keep dependencies clean
#define SERIALIZE_HEADER \
    uint32_t GetSerializeSize(int nType, int nVersion) const;   \
    template<typename Stream>                                       \
    void Serialize(Stream& s, int nType, int nVersion) const;       \
    template<typename Stream>                                       \
    void Unserialize(Stream& s, int nType, int nVersion);


#endif
