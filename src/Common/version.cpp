/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <Common/include/version.h>
#include <Util/include/debug.h>

namespace version
{
    /* Client Version Outputs. */
    const std::string CLIENT_NAME{"Tritium"};

    /* The version number */
    const std::string CLIENT_VERSION_STRING = debug::safe_printstr(CLIENT_MAJOR, ".", CLIENT_MINOR, ".", CLIENT_PATCH);


    /* The interface used Qt, CLI, or Tritium) */
    #if defined QT_GUI
        const std::string CLIENT_INTERFACE{"Qt"};
    #elif defined TRITIUM_GUI
        const std::string CLIENT_INTERFACE{"GUI"};
    #else
        const std::string CLIENT_INTERFACE{"CLI"};
    #endif


    /* The database type used (Berkeley DB, or Lower Level Database) */
    #if defined USE_LLD
        const std::string CLIENT_DATABASE{"[LLD]"};
    #else
        const std::string CLIENT_DATABASE{"[BDB]"};
    #endif


    /* The Architecture (32-Bit, ARM 64, or 64-Bit) */
    #if defined x86
        const std::string BUILD_ARCH{"[x86]"};
    #elif defined aarch64
        const std::string BUILD_ARCH{"[ARM aarch64]"};
    #else
        const std::string BUILD_ARCH{"[x64]"};
    #endif

    const std::string CLIENT_VERSION_BUILD_STRING{CLIENT_VERSION_STRING + "-pre " + CLIENT_NAME  + " " + CLIENT_INTERFACE + " " + CLIENT_DATABASE + BUILD_ARCH};
}
