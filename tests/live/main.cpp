/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/types/uint1024.h>
#include <LLC/include/random.h>
#include <LLC/prime/fermat.h>

#include <TAO/Ledger/include/prime.h>

#include <TAO/Ledger/types/stream.h>

#include <Util/include/runtime.h>
#include <Util/include/debug.h>

#include <limits>




/* This is for prototyping new code. This main is accessed by building with LIVE_TESTS=1. */
int main(int argc, char** argv)
{

    {
        uint256_t hashFalcon, hashFalcon2; //these act as hardcoded values for us to set a predefined set of credentials

        hashFalcon.SetType(AUTH::TYPES::FALCON);

        using namespace TAO::Ledger;

        TAO::Ledger::Stream ssAuthorization;

        //create an auth script for a single falcon key
        //(NextHash.Falcon(OP::ALL) == hashFalcon)

        //single signature authentication
        ssAuthorization << AUTH::ENABLE_IF << uint16_t(OP::ALL);
        ssAuthorization << AUTH::VERIFY::NEXTHASH << hashFalcon;
        ssAuthorization << AUTH::OP::THEN;

        ssAuthorization << AUTH::RETURN << AUTH::GRANTED << uint16_t(OP::ALL);
        ssAuthorization << AUTH::ENDIF;




        //multisignature authentication with user generated credentials
        ssAuthorization << AUTH::ENABLE_IF << uint16_t(OP::ALL);
        ssAuthorization << AUTH::VERIFY::NEXTHASH << hashFalcon;
        ssAuthorization << AUTH::OP::AND;
        ssAuthorization << AUTH::VERIFY::NEXTHASH << hashFalcon2;
        ssAuthorization << AUTH::OP::THEN;

        ssAuthorization << AUTH::RETURN << AUTH::GRANTED << uint16_t(OP::ALL);
        ssAuthorization << AUTH::ENDIF;



        //multisignature authentication with multiple sigchains
        uint256_t hashGenesis2, hashGenesis3;
        ssAuthorization << AUTH::ENABLE_IF << uint16_t(OP::ALL);
        ssAuthorization << AUTH::VERIFY::NEXTHASH << AUTH::PARAM::GENESIS << hashGenesis2 << AUTH::CALLER::CRYPTO::SIGN;
        ssAuthorization << AUTH::OP::AND;
        ssAuthorization << AUTH::VERIFY::NEXTHASH << AUTH::PARAM::GENESIS << hashGenesis3 << AUTH::CALLER::CRYPTO::SIGN;
        ssAuthorization << AUTH::OP::THEN;

        ssAuthorization << AUTH::RETURN << AUTH::GRANTED << uint16_t(OP::ALL);
        ssAuthorization << AUTH::ENDIF;




        //staking and credit only, no unstake coins
        ssAuthorization << AUTH::ENABLE_IF << uint16_t(TRUST::ENABLED | CREDIT::ENABLED);
        ssAuthorization << AUTH::VERIFY::NEXTHASH << hashStaking;
        ssAuthorization << AUTH::OP::THEN;

        //check that we are not removing stake with these credentials
        ssAuthorization << AUTH::OP::IF;
        ssAuthorization << AUTH::TRANSACTION::OP << AUTH::OP::EQUALS << TAO::Operation::OP::TRUST;
        ssAuthorization << AUTH::OP::AND;
        ssAuthorization << AUTH::TRUST::PARAM::CHANGE << AUTH::OP::LESSTHAN << AUTH::TYPES::INT64 << int64_t(0);
        ssAuthorization << AUTH::OP::THEN;

        ssAuthorization << AUTH::RETURN << AUTH::DENIED;
        ssAuthorization << AUTH::ENDIF;

        //default exit from authorization routine
        ssAuthorization << AUTH::RETURN << AUTH::GRANTED << uint16_t(TRUST::ENABLED | CREDIT::ENABLED);
        ssAuthorization << AUTH::ENDIF;



        //system authentication for processing methods for augmented contracts
        ssAuthorization << AUTH::ENABLE_IF << uint16_t(INVOKE::ENABLED);

        //we can use this access pattern to allow any external sigchain to invoke a function in any object register we have
        ssAuthorization << AUTH::VERIFY::NEXTHASH << AUTH::CALLER::CRYPTO::SIGN; //this is a 256 bit hash of any caller
        ssAuthorization << AUTH::OP::THEN;

        //default exit from authorization routine
        ssAuthorization << AUTH::RETURN << AUTH::GRANTED << uint16_t(INVOKE::ENABLED);
        ssAuthorization << AUTH::ENDIF;



        //system authentication for processing methods for augmented contracts for a single individual
        uint256_t hashGenesis;
        ssAuthorization << AUTH::ENABLE_IF << uint16_t(INVOKE::ENABLED);

        //we can use this access pattern to allow any external sigchain to invoke a function in any object register we have
        ssAuthorization << AUTH::VERIFY::NEXTHASH << AUTH::CALLER::CRYPTO::SIGN; //this is a 256 bit hash of any caller

        //only allow invoke from a specific sigchain
        ssAuthorization << AUTH::OP::AND;
        ssAuthorization << AUTH::CALLER::GENESIS << AUTH::OP::EQUALS << AUTH::TYPES::UINT256 << hashGenesis;
        ssAuthorization << AUTH::OP::THEN;

        ssAuthorization << AUTH::RETURN << AUTH::GRANTED << uint16_t(INVOKE::ENABLED);
        ssAuthorization << AUTH::ENDIF;



        //system authentication for processing methods for augmented contracts for a single individual with single operation code
        ssAuthorization << AUTH::ENABLE_IF << uint16_t(INVOKE::ENABLED);

        //by adding param genesis in here we can skip the checks for auth::caller::genesis and integrate in the single call
        ssAuthorization << AUTH::VERIFY::NEXTHASH << AUTH::PARAM::GENESIS << hashGenesis << AUTH::CALLER::CRYPTO::SIGN;
        ssAuthorization << AUTH::OP::THEN;

        ssAuthorization << AUTH::RETURN << AUTH::GRANTED << uint16_t(INVOKE::ENABLED);
        ssAuthorization << AUTH::ENDIF;



        //recovery authentication
        ssAuthorization << AUTH::ENABLE_IF << uint16_t(OP::ALL);
        ssAuthorization << AUTH::TYPES::RECOVERY << AUTH::TYPES::FALCON << hashFalcon;
        ssAuthorization << AUTH::OP::THEN;

        ssAuthorization << AUTH::RETURN << AUTH::GRANTED << uint16_t(OP::NONE);
        ssAuthorization << AUTH::ENDIF;



        //multisig recovery authentication with two recovery phrases (can wipe entire ledger VM script except own recovery hash)
        ssAuthorization << AUTH::ENABLE_IF << uint16_t(OP::ALL);
        ssAuthorization << AUTH::TYPES::RECOVERY << hashRecovery1;
        ssAuthorization << AUTH::OP::AND;
        ssAuthorization << AUTH::TYPES::RECOVERY << hashRecovery2;
        ssAuthorization << AUTH::OP::THEN;

        ssAuthorization << AUTH::RETURN << AUTH::GRANTED << uint16_t(OP::NONE);
        ssAuthorization << AUTH::ENDIF;
    }


    {
        //let's make some functions now for an object register

        TAO::Register::Object account;

        /* Generate the object register values. */
        account << std::string("balance")       << uint8_t(TYPES::MUTABLE)   << uint8_t(TYPES::UINT64_T) << uint64_t(0)
                << std::string("token")         << uint8_t(TYPES::UINT256_T) << uint256_t(0);

        //let us now create a function of binary data.
        account << std::string("total") << uint8_t(TYPES::UINT64_T) << uint64_t(0);

        TAO::Operation::Stream ssMethod;
        account << std::string("::DEBIT") << uint8_t(TYPES::METHOD) << ssMethod.Bytes();
    }

    return 0;
}
