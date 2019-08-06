#include <unordered_set>

#include <LLD/include/global.h>

#include <TAO/API/include/global.h>
#include <TAO/API/include/utils.h>
#include <TAO/API/types/exception.h>

#include <TAO/Ledger/include/constants.h>
#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/include/difficulty.h>

#include <TAO/Ledger/types/tritium.h>
#include <TAO/Ledger/types/mempool.h>
#include <TAO/Ledger/types/sigchain.h>

#include <TAO/Operation/types/stream.h>
#include <TAO/Operation/include/enum.h>

#include <TAO/Register/include/create.h>
#include <TAO/Register/include/names.h>
#include <TAO/Register/types/address.h>

#include <Util/include/args.h>
#include <Util/include/hex.h>
#include <Util/include/json.h>
#include <Util/include/base64.h>
#include <Util/include/debug.h>

#include <regex>


/* Global TAO namespace. */
namespace TAO
{
    /* API Layer namespace. */
    namespace API
    {
        /* Creates a new Name Object register for the given name and register
           address adds the register operation to the contract */
        TAO::Operation::Contract Names::CreateName(const uint256_t& hashGenesis,
                                                   const std::string& strFullName,
                                                   const uint256_t& hashRegister)
        {
            /* Declare the contract for the response */
            TAO::Operation::Contract contract;

            /* The hash representing the namespace that the Name will be created in.  For user local this will be the genesis hash
               of the callers sig chain.  For global namespace this will be a SK256 hash of the namespace name. */
            uint256_t hashNamespace = 0;

            /* The register address of the Name object. */
            TAO::Register::Address hashNameAddress;

            /* The namespace string, populated if the caller has passed the name in name.namespace format */
            std::string strNamespace = "";

            /* The name of the Name object */
            std::string strName = strFullName;

            /* Edge case check for name starting with @, which will pass the escape regex and is essentially a zero-length name*/
            if(strName.find("@", 0) == 0)
                throw APIException(-167, "Name cannot be less than 3 characters long");

            /* Check to see whether the name has a namesace suffix.  This will be the last instance of an @ that is not 
               escaped with '!' */
            std::regex search("[^!]@");
            std::smatch match;
            
            /* Search the string for any occurrences of @ that are not escaped with '!'.  The first check here is ensure that the
               string does not start with an @, which would pass the regex. */
            if( std::regex_search( strName, match, search))
            {
                /* If there is more than one match then error, as the unescaped @ is not allowed in the name */
                if(match.size() > 1)
                    throw APIException(-161, "Name contains invalid characters.  Please escape them with '\'");

                /* Get the postition of the @ sign that we found */
                size_t nPos = match.position(0)+1; 

                /* If so then strip off the namespace so that we can check that this user has permission to use it */
                strName = strName.substr(0, nPos);
                strNamespace = strFullName.substr(nPos+1);

                if(strNamespace.length() == 0)
                    throw APIException(-94, "Invalid namespace");

                /* Namespace hash is a SK256 hash of the namespace name */
                hashNamespace = TAO::Register::Address(strNamespace, TAO::Register::Address::NAMESPACE);

                /* Namespace object to retrieve*/
                TAO::Register::Object namespaceObject;

                /* Retrieve the Namespace object by name */
                if(!TAO::Register::GetNamespaceRegister(strNamespace, namespaceObject))
                    throw APIException(-95, "Namespace does not exist: " + strNamespace);

                /* Check the owner is the hashGenesis */
                if(namespaceObject.hashOwner != hashGenesis)
                    throw APIException(-96, "Cannot create a name in namespace " + strNamespace + " as you are not the owner.");
            }
            else
                /* If no global namespace suffix has been passed in then use the callers genesis hash for the hashNamespace. */
                hashNamespace = hashGenesis;


            /* Obtain the name register address for the genesis/name combination */
            TAO::Register::GetNameAddress(hashNamespace, strName, hashNameAddress);

            /* Check to see whether the name already exists  */
            TAO::Register::Object object;
            if(LLD::Register->ReadState(hashNameAddress, object, TAO::Ledger::FLAGS::MEMPOOL))
            {
                if(!strNamespace.empty())
                    throw APIException(-97, "An object with this name already exists in this namespace.");
                else
                    throw APIException(-98, "An object with this name already exists for this user.");
            }

            if(strName.length() < 3)
                throw APIException(-167, "Name cannot be less than 3 characters long");


            /* Create the Name register object pointing to hashRegister */
            TAO::Register::Object name = TAO::Register::CreateName(strNamespace, strName, hashRegister);

            /* Add the Name object register operation to the transaction */
            contract << uint8_t(TAO::Operation::OP::CREATE) << hashNameAddress << uint8_t(TAO::Register::REGISTER::OBJECT) << name.GetState();

            return contract;
        }


        /* Creates a new Name Object register for an object being transferred */
        TAO::Operation::Contract Names::CreateName(const uint256_t& hashGenesis,
                                                   const json::json& params,
                                                   const uint512_t& hashTransfer)
        {
            /* Declare the contract for the response */
            TAO::Operation::Contract contract;

            /* Firstly retrieve the transfer transaction that is being claimed so that we can get the address of the object */
            TAO::Ledger::Transaction txTransfer;

            /* Check disk of writing new block. */
            if(!LLD::Ledger->ReadTx(hashTransfer, txTransfer, TAO::Ledger::FLAGS::MEMPOOL))
                throw APIException(-99, "Transfer transaction not found.");

            /* Ensure we are not claiming our own Transfer.  If we are then no need to create a Name object as we already have one */
            if(txTransfer.hashGenesis != hashGenesis)
            {
                /* Loop through the contracts. */
                for(uint32_t nContract = 0; nContract < txTransfer.Size(); ++nContract)
                {
                    /* Get a reference of the contract. */
                    const TAO::Operation::Contract& check = txTransfer[nContract];

                    /* Extract the OP from tx. */
                    uint8_t OP = 0;
                    check >> OP;

                    /* Check for proper op. */
                    if(OP != TAO::Operation::OP::TRANSFER)
                        continue;

                    /* Extract the object register address  */
                    uint256_t hashAddress = 0;
                    check >> hashAddress;

                    /* Now check the previous owners Name records to see if there was a Name for this object */
                    std::string strAssetName = ResolveName(txTransfer.hashGenesis, hashAddress);

                    /* If a name was found then create a Name record for the new owner using the same name */
                    if(!strAssetName.empty())
                        contract = Names::CreateName(hashGenesis, strAssetName, hashAddress);

                    /* If found break. */
                    break;
                }
            }

            return contract;
        }

        /* Retrieves a Name object by name. */
        TAO::Register::Object Names::GetName(const json::json& params, const std::string& strObjectName, uint256_t& hashNameObject)
        {
            /* Declare the name object to return */
            TAO::Register::Object nameObject;

            /* In order to resolve an object name to a register address we also need to know the namespace.
            *  This must be provided by the caller by passing the name in the name@namespace format.  
            *  If no namespace is explicitly provided we will search the names of currently logged in sig chain */
            std::string strName = strObjectName;
            std::string strNamespace = "";

            /* Declare the namespace hash to use for this object. */
            uint256_t hashNamespace = 0;

            /* First check to see if the name parameter has been provided in name@namespace format */
            std::regex search("[^!]@");
            std::smatch match;
            
            /* Search the string for any occurrences of @ that are not escaped */
            if(std::regex_search( strName, match, search))
            {
                /* If there is more than one match then error, as the unescaped @ is not allowed in the name */
                if(match.size() > 1)
                    throw APIException(-161, "Name contains invalid characters.  Please escape them with '\'");

                /* Get the postition of the @ sign that we found */
                size_t nNamespacePos = match.position(0) +1; 

                /* If the name is in name@namespace format then split the namespace and name into separate variables */
                strNamespace = strName.substr(nNamespacePos+1);
                strName = strName.substr(0, nNamespacePos);

                /* Namespace hash is a SK256 hash of the namespace name */
                hashNamespace = TAO::Register::Address(strNamespace, TAO::Register::Address::NAMESPACE);
            }
            /* If namespace is not provided then check to see if there is an active session to access the sig chain */
            else
            {
                /* Get the session to be used for this API call.  Note we pass in false for fThrow here so that we can
                   throw a missing namespace exception if no valid session could be found */
                uint64_t nSession = users->GetSession(params, false);

                /* Get the account. */
                memory::encrypted_ptr<TAO::Ledger::SignatureChain>& user = users->GetAccount(nSession);
                if(!user)
                    throw APIException(-100, "Missing username prefix before name");

                /* Set the namespace name to be the user's genesis ID */
                hashNamespace = user->Genesis();
            }

            /* Read the Name Object */
            if(!TAO::Register::GetNameRegister(hashNamespace, strName, nameObject))
            {
                if(strNamespace.empty())
                    throw APIException(-101, debug::safe_printstr("Unknown name: ", strObjectName));
            }

            /* Get the address of the Name object to return */
            TAO::Register::GetNameAddress(hashNamespace, strName, hashNameObject);

            return nameObject;
        }


        /* Scans the Name records associated with the hashGenesis sig chain to find an entry with a matching hashObject address */
        TAO::Register::Object Names::GetName(const uint256_t& hashGenesis, const uint256_t& hashObject, uint256_t& hashNameObject)
        {
            /* Declare the return val */
            TAO::Register::Object nameObject;

            /* Get all object registers owned by this sig chain */
            std::vector<uint256_t> vRegisters;
            if(ListRegisters(hashGenesis, vRegisters))
            {
                /* Iterate through these to find all Name registers */
                for(const auto& hashRegister : vRegisters)
                {
                    /* Get the object from the register DB.  We can read it as an Object and then check its nType
                    to determine whether or not it is a Name. */
                    TAO::Register::Object object;
                    if(!LLD::Register->ReadState(hashRegister, object, TAO::Ledger::FLAGS::MEMPOOL))
                        continue;

                    if(object.nType == TAO::Register::REGISTER::OBJECT)
                    {
                        /* parse object so that the data fields can be accessed */
                        if(!object.Parse())
                            throw APIException(-36, "Failed to parse object register");

                        /* Check the object register standards. */
                        if(object.Standard() != TAO::Register::OBJECTS::NAME)
                            continue;

                        /* Check to see whether the address stored in this Name matches the hash we are searching for*/
                        if(object.get<uint256_t>("address") == hashObject)
                        {
                            /* Set the return values */
                            nameObject = object;
                            hashNameObject = hashRegister;

                            /* break out since we have a match */
                            break;
                        }
                    }
                }
            }

            return nameObject;
        }


        /* Resolves a register address from a name by looking up the Name object. */
        uint256_t Names::ResolveAddress(const json::json& params, const std::string& strName)
        {
            /* Declare the return register address hash */
            uint256_t hashRegister = 0;

            /* Register address of nameObject.  Not used by this method */
            uint256_t hashNameObject = 0;

            /* Get the Name object by name */
            TAO::Register::Object name = Names::GetName(params, strName, hashNameObject);

            if(!name.IsNull())
                /* Get the address that this name register is pointing to */
                hashRegister = name.get<uint256_t>("address");

            return hashRegister;
        }


        /* Scans the Name records associated with the hashGenesis sig chain to find an entry with a matching hashRegister address */
        std::string Names::ResolveName(const uint256_t& hashGenesis, const uint256_t& hashRegister)
        {
            /* Declare the return val */
            std::string strName = "";

            /* Register address of nameObject.  Not used by this method */
            uint256_t hashNameObject = 0;

            /* Look up the Name object for the register address hash */
            TAO::Register::Object name = Names::GetName(hashGenesis, hashRegister, hashNameObject);

            if(!name.IsNull())
            {
                /* Get the name from the Name register */
                strName = name.get<std::string>("name");
            }

            return strName;
        }


        /* Retrieves the token name for the token that this account object is used for.
        *  The token is obtained by looking at the token_address field,
        *  which contains the register address of the issuing token */
        std::string Names::ResolveAccountTokenName(const json::json& params, const TAO::Register::Object& account)
        {
            /* Declare token name to return  */
            std::string strTokenName;

            /* Get the object standard. */
            uint8_t nStandard = account.Standard();

            /* Check the object register standard. */
            if(nStandard == TAO::Register::OBJECTS::ACCOUNT)
            {
                /* The token name is obtained by first looking at the token field int the account,
                   which contains the register address of the issuing token */
                uint256_t hashToken = account.get<uint256_t>("token");

                /* Edge case for NXS token which has identifier 0, so no look up needed */
                if(hashToken == 0)
                    strTokenName = "NXS";
                else
                {
                    /* Get the session to be used for this API call. */
                    uint64_t nSession = users->GetSession(params, false);

                    /* Don't attempt to resolve the object name if there is no logged in user as there will be no sig chain  to scan */
                    if(nSession != -1)
                    {
                        /* Get the account. */
                        memory::encrypted_ptr<TAO::Ledger::SignatureChain>& user = users->GetAccount(nSession);

                        /* Look up the token name based on the Name records in the caller's sig chain */
                        strTokenName = Names::ResolveName(user->Genesis(), hashToken);
                    }
                }
            }
            else
                throw APIException(-65, "Object is not an account.");

            return strTokenName;
        }


    } /* End API namespace */

} /* End TAO namespace */
