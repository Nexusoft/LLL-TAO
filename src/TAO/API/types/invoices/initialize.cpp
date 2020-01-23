/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/types/invoices.h>
#include <TAO/API/include/utils.h>

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {
        
        /* Allows derived API's to handle custom/dynamic URL's where the strMethod does not
         *  map directly to a function in the target API.  Insted this method can be overriden to
         *  parse the incoming URL and route to a different/generic method handler, adding parameter
         *  values if necessary.  E.g. get/myasset could be rerouted to get/asset with name=myasset
         *  added to the jsonParams
         *  The return json contains the modifed method URL to be called.
         */
        std::string Invoices::RewriteURL(const std::string& strMethod, json::json& jsonParams)
        {
            std::string strMethodRewritten = strMethod;

            /* support passing the item name after the method e.g. get/invoice/myitem */
            std::size_t nPos = strMethod.find("/invoice/");
            if(nPos != std::string::npos)
            {
                std::string strNameOrAddress;

                /* Edge case for /invoices/list/invoice/history/invoicename */
                if(strMethod == "list/invoice/history")
                    return strMethod;
                else if(strMethod.find("list/invoice/history/") != std::string::npos)
                {
                    strMethodRewritten = "list/invoice/history";

                    /* Get the name or address that comes after the list/invoice/history part */
                    strNameOrAddress = strMethod.substr(21); 
                }
                else
                {
                    /* get the method name from the incoming string */
                    strMethodRewritten = strMethod.substr(0, nPos+8);

                    /* Get the name or address that comes after the /invoice/ part */
                    strNameOrAddress = strMethod.substr(nPos +9);
                }

                /* Check to see whether there is a fieldname after the token name, i.e. invoices/get/invoice/myinvoice/somefield */
                nPos = strNameOrAddress.find("/");

                if(nPos != std::string::npos)
                {
                    /* Passing in the fieldname is only supported for the /get/  so if the user has 
                        requested a different method then just return the requested URL, which will in turn error */
                    if(strMethodRewritten != "get/invoice")
                        return strMethod;

                    std::string strFieldName = strNameOrAddress.substr(nPos +1);
                    strNameOrAddress = strNameOrAddress.substr(0, nPos);
                    jsonParams["fieldname"] = strFieldName;
                }

                
                /* Edge case for pay/invoice/txid */
                if(strMethodRewritten == "pay/invoice")
                    jsonParams["txid"] = strNameOrAddress;
                /* Determine whether the name/address is a valid register address and set the name or address parameter accordingly */
                else if(IsRegisterAddress(strNameOrAddress))
                    jsonParams["address"] = strNameOrAddress;
                else
                    jsonParams["name"] = strNameOrAddress;
                    
            }

            return strMethodRewritten;
        }

        /* Standard initialization function. */
        void Invoices::Initialize()
        {
            mapFunctions["create/invoice"]          = Function(std::bind(&Invoices::Create, this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["get/invoice"]             = Function(std::bind(&Invoices::Get,    this, std::placeholders::_1, std::placeholders::_2));
            //mapFunctions["send/invoice"]            = Function(std::bind(&Invoices::Send,   this, std::placeholders::_1, std::placeholders::_2));
            //mapFunctions["pay/invoice"]             = Function(std::bind(&Invoices::Pay,      this, std::placeholders::_1, std::placeholders::_2));
            //mapFunctions["cancel/invoice"]          = Function(std::bind(&Invoices::Cancel, this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["list/invoice/history"]    = Function(std::bind(&Invoices::History,    this, std::placeholders::_1, std::placeholders::_2));
        }
    }
}
