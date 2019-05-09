/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/include/assets.h>

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        /* Get the data from a digital asset */
        json::json Assets::Get(const json::json& params, bool fHelp)
        {
            json::json ret;

            /* Get the Register ID. */
            uint256_t hashRegister = 0;

            /* Check for data parameter. */
            if(params.find("name") != params.end())
            {
                /* Get the address from the name. */
                std::string strName = GetName() + ":" + params["name"].get<std::string>();

                /* Build the address from an SK256 hash of API:NAME. */
                hashRegister = LLC::SK256(std::vector<uint8_t>(strName.begin(), strName.end()));
            }

            /* Otherwise try to find the raw hex encoded address. */
            else if(params.find("address") != params.end())
                hashRegister.SetHex(params["address"].get<std::string>());

            /* Fail if no required parameters supplied. */
            else
                throw APIException(-23, "Missing memory address");


            /* Check to see whether the caller has requested a specific data field to return */
            std::string strDataField = "";
            if(params.find("datafield") != params.end())
                strDataField =  params["datafield"].get<std::string>();

            /* Get the asset from the register DB.  We can read it as an Object.  
               If this fails then we try to read it as a base State type and assume it was
               created as a raw format asset */
            TAO::Register::Object object;
            if(!LLD::regDB->ReadState(hashRegister, object))
                throw APIException(-24, "No state found");
            
            if(object.nType == TAO::Register::REGISTER::RAW)
            {
                /* Build the response JSON for the raw format asset. */
                ret["timestamp"]  = object.nTimestamp;
                ret["owner"]      = object.hashOwner.ToString();

                /* raw state assets only have one data member containing the raw hex-encoded data*/
                std::string data;
                object >> data;
                ret["data"] = data;
            }
            else if(object.nType == TAO::Register::REGISTER::OBJECT)
            {
                
                /* Build the response JSON. */
                if(strDataField.empty())
                {
                    ret["timestamp"]  = object.nTimestamp;
                    ret["owner"]      = object.hashOwner.ToString();
                }
                /* parse object */
                object.Parse();
                
                /* Get List of field names in this asset object */
                std::vector<std::string> vFieldNames = object.GetFieldNames();

                /* Declare type and data variables for unpacking the Object fields */
                uint8_t nType;
                uint8_t nUint8;
                uint16_t nUint16;
                uint32_t nUint32;
                uint64_t nUint64;
                uint256_t nUint256;
                uint512_t nUint512;
                uint1024_t nUint1024;
                std::string strValue;

                for( const auto& strFieldName : vFieldNames)
                {
                    /* Only return requested data field if one was specifically requested */
                    if(!strDataField.empty() && strDataField != strFieldName)
                        continue;

                    /* First get the type*/
                    object.Type(strFieldName, nType);

                    if(nType == TAO::Register::TYPES::UINT8_T )
                    {
                        object.Read<uint8_t>(strFieldName, nUint8);
                        ret[strFieldName] = nUint8;
                    }
                    else if(nType == TAO::Register::TYPES::UINT16_T )
                    {
                        object.Read<uint16_t>(strFieldName, nUint16);
                        ret[strFieldName] = nUint16;
                    }
                    else if(nType == TAO::Register::TYPES::UINT32_T )
                    {
                        object.Read<uint32_t>(strFieldName, nUint32);
                        ret[strFieldName] = nUint32;
                    }
                    else if(nType == TAO::Register::TYPES::UINT64_T )
                    {
                        object.Read<uint64_t>(strFieldName, nUint64);
                        ret[strFieldName] = nUint64;
                    }
                    else if(nType == TAO::Register::TYPES::UINT256_T )
                    {
                        object.Read<uint256_t>(strFieldName, nUint256);
                        ret[strFieldName] = nUint256.GetHex();
                    }
                    else if(nType == TAO::Register::TYPES::UINT512_T )
                    {
                        object.Read<uint512_t>(strFieldName, nUint512);
                        ret[strFieldName] = nUint512.GetHex();
                    }
                    else if(nType == TAO::Register::TYPES::UINT1024_T )
                    {
                        object.Read<uint1024_t>(strFieldName, nUint1024);
                        ret[strFieldName] = nUint1024.GetHex();
                    }
                    else if(nType == TAO::Register::TYPES::STRING || nType == TAO::Register::TYPES::BYTES)
                    {
                        object.Read<std::string>(strFieldName, strValue);
                        ret[strFieldName] = strValue;
                    }
                    
                }
            
                
            }
            else
            {
                throw APIException(-24, "Specified name/address is not an asset.");
            }
            
            return ret;
        }
    }
}
