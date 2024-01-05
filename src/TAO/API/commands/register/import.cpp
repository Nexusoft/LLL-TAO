/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2023

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/types/commands/register.h>
#include <TAO/API/types/commands.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Copies objects of an API instance, indexed by our name. */
    void Register::Import(const std::string& strAPI)
    {
        /* Catch if API doesn't exist. */
        if(!Commands::Has(strAPI))
            return;

        /* Copy our standards from given API objects. */
        Commands::Instance(strAPI)->Export(mapStandards, strAPI);
    }
}
