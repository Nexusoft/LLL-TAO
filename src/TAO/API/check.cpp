/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/include/global.h>
#include <TAO/API/include/check.h>

#include <TAO/Register/types/object.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /*  Determines whether a string value is a valid base58 encoded register address.
     *  This only checks to see if the value an be decoded into a valid Register::Address with a valid Type.
     *  It does not check to see whether the register address exists in the database
     */
    bool CheckAddress(const std::string& strValue)
    {
        /* Decode the incoming string into a register address */
        TAO::Register::Address address;
        address.SetBase58(strValue);

        /* Check to see whether it is valid */
        return address.IsValid();

    }


    /*  Utilty method that checks that the signature chain is mature and can therefore create new transactions.
     *  Throws an appropriate APIException if it is not mature. */
    void CheckMature(const uint256_t& hashGenesis)
    {
        /* No need to check this in private mode as there is no PoS/Pow */
        if(!config::fHybrid.load())
        {
            /* Get the number of blocks to maturity for this sig chain */
            const uint32_t nBlocksToMaturity = users->BlocksToMaturity(hashGenesis);
            if(nBlocksToMaturity > 0)
                throw APIException(-202, debug::safe_printstr( "Signature chain not mature after your previous mined/stake block. ", nBlocksToMaturity, " more confirmation(s) required."));
        }
    }


    /* Checks if the designated object matches the explicet type specified in parameters. */
    void CheckType(const json::json& params, const uint256_t& hashCheck)
    {
        /* Let's grab our object to check against and throw if it's missing. */
        TAO::Register::Object objCheck;
        if(!LLD::Register->ReadObject(hashCheck, objCheck))
            throw APIException(-33, "Incorrect or missing name / address");

        /* Execute now that we have the object. */
        CheckType(params, objCheck);
    }


    /*  Checks if the designated object matches the explicet type specified in parameters.
     *  Doesn't do a register database lookup like prior overload does. */
    void CheckType(const json::json& params, const TAO::Register::Object& objCheck)
    {
        /* If the user requested a particular object type then check it is that type */
        if(params.find("type") != params.end())
        {
            /* Grab a copy of our type to check against. */
            const std::string& strType = params["type"].get<std::string>();

            /* Let's check against the types required now. */
            const uint8_t nStandard = objCheck.Standard();
            if(strType == "token" && nStandard != TAO::Register::OBJECTS::TOKEN)
                throw APIException(-49, "Unexpected type for name / address");

            /* Check for expected account type now. */
            if(strType == "account" && nStandard != TAO::Register::OBJECTS::ACCOUNT)
                throw APIException(-49, "Unexpected type for name / address");
        }
    }


    /* For use in list commands that check for 'accounts' or 'tokens' */
    bool CheckTypes(const json::json& params, const TAO::Register::Object& objCheck)
    {
        /* If the user requested a particular object type then check it is that type */
        if(params.find("type") == params.end())
            throw APIException(-118, "Missing type");

        /* Grab a copy of our type to check against. */
        const std::string& strType = params["type"].get<std::string>();

        /* Let's check against the types required now. */
        const uint8_t nStandard = objCheck.Standard();
        if(strType == "tokens" && nStandard != TAO::Register::OBJECTS::TOKEN)
            return false;

        /* Check for expected account type now. */
        if(strType == "accounts" && nStandard != TAO::Register::OBJECTS::ACCOUNT)
            return false;

        return true;
    }

} // End TAO namespace
