/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_UTIL_MACRO_HEADER_H
#define NEXUS_UTIL_MACRO_HEADER_H


//Use this in the header file to keep dependencies clean
#define SERIALIZE_HEADER \
    uint32_t GetSerializeSize(int nSerType, int nSerVersion) const;   \
    template<typename Stream>                                       \
    void Serialize(Stream& s, int nSerType, int nSerVersion) const;       \
    template<typename Stream>                                       \
    void Unserialize(Stream& s, int nSerType, int nSerVersion);


#endif
