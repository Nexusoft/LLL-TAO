/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLP_INCLUDE_GLOBAL_H
#define NEXUS_LLP_INCLUDE_GLOBAL_H

#include <LLP/types/legacy.h>
#include <LLP/types/tritium.h>
#include <LLP/types/time.h>
#include <LLP/templates/server.h>

namespace LLP
{
    extern Server<LegacyNode>*   LEGACY_SERVER;
    extern Server<TritiumNode>* TRITIUM_SERVER;
    extern Server<TimeNode>*     TIME_SERVER;
}

#endif
