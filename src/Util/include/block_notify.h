/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once

#include <LLC/types/uint1024.h>

#include <Util/include/config.h>
#include <Util/include/debug.h>
#include <Util/include/string.h>

/** BlockNotify
 *
 *  Call an external process or command for new blocks 
 *
 **/
void BlockNotify(const uint1024_t& hashBlock)
{
#ifndef IPHONE
    /* Block notify. */
    std::string strCmd = config::GetArg("-blocknotify", "");
    if(!strCmd.empty())
    {
        ReplaceAll(strCmd, "%s", hashBlock.GetHex());
        int32_t nRet = std::system(strCmd.c_str());
        debug::log(0, FUNCTION, "Block Notify Executed with code ", nRet);
    }
#endif
}

