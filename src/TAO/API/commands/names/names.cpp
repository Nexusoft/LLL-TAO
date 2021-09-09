/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <unordered_set>

#include <LLD/include/global.h>

#include <LLP/include/global.h>
#include <LLP/types/tritium.h>
#include <LLP/templates/trigger.h>

#include <TAO/API/include/list.h>
#include <TAO/API/types/commands.h>

#include <TAO/API/users/types/users.h>
#include <TAO/API/types/commands/names.h>

#include <TAO/API/types/session-manager.h>
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


/* Global TAO namespace. */
namespace TAO
{
    /* API Layer namespace. */
    namespace API
    {
        /* Creates a new Name Object register for the given name and register
           address adds the register operation to the contract */
        TAO::Operation::Contract Names::CreateName(const uint256_t& hashGenesis,
                                                   const std::string& strName,
                                                   const std::string& strNamespace,
                                                   const TAO::Register::Address& hashRegister)
        {
            /* Check name length */
            if(strName.length() == 0)
                throw Exception(-88, "Missing or empty name.");

            /* Name can't start with : */
            if(strName[0]== ':' )
                throw Exception(-161, "Names cannot start with a colon");

            /* Declare the contract for the response */
            TAO::Operation::Contract contract;

            /* The hash representing the namespace that the Name will be created in.  For user local this will be the genesis hash
               of the callers sig chain.  For global namespace this will be a SK256 hash of the namespace name. */
            TAO::Register::Address hashNamespace;

            /* The register address of the Name object. */
            TAO::Register::Address hashNameAddress;

            /* Check to see whether the name is being created in a namesace */
            if(strNamespace.length() > 0)
            {
                /* Namespace hash is a SK256 hash of the namespace name */
                hashNamespace = TAO::Register::Address(strNamespace, TAO::Register::Address::NAMESPACE);

                /* If the name is not being created in the global namespace then check that the caller owns the namespace */
                if(strNamespace != TAO::Register::NAMESPACE::GLOBAL)
                {
                    /* Namespace object to retrieve*/
                    TAO::Register::Object namespaceObject;

                    /* Retrieve the Namespace object by name */
                    if(!TAO::Register::GetNamespaceRegister(strNamespace, namespaceObject))
                        throw Exception(-95, "Namespace does not exist: " + strNamespace);

                    /* Check the owner is the hashGenesis */
                    if(namespaceObject.hashOwner != hashGenesis)
                        throw Exception(-96, "Cannot create a name in namespace " + strNamespace + " as you are not the owner.");
                }
                else
                {
                    /* If it is a global name then check it doesn't contain any colons */
                    if(strNamespace.find(":") != strNamespace.npos )
                        throw Exception(-171, "Global names cannot cannot contain a colon");
                }
            }
            else
                /* If no namespace has been passed in then use the callers genesis hash for the hashNamespace. */
                hashNamespace = hashGenesis;


            /* Obtain the name register address for the genesis/name combination */
            hashNameAddress = TAO::Register::Address(strName, hashNamespace, TAO::Register::Address::NAME);

            /* Check to see whether the name already exists  */
            TAO::Register::Object object;
            if(LLD::Register->ReadState(hashNameAddress, object, TAO::Ledger::FLAGS::MEMPOOL))
            {
                if(!strNamespace.empty())
                    throw Exception(-97, "An object with this name already exists in this namespace.");
                else
                    throw Exception(-98, "An object with this name already exists for this user.");
            }


            /* Create the Name register object pointing to hashRegister */
            TAO::Register::Object name = TAO::Register::CreateName(strNamespace, strName, hashRegister);

            /* Add the Name object register operation to the transaction */
            contract << uint8_t(TAO::Operation::OP::CREATE) << hashNameAddress << uint8_t(TAO::Register::REGISTER::OBJECT) << name.GetState();

            return contract;
        }


        /* Creates a new Name Object register for an object being transferred */
        TAO::Operation::Contract Names::CreateName(const uint256_t& hashGenesis,
                                                   const uint512_t& hashTransfer)
        {
            /* Declare the contract for the response */
            TAO::Operation::Contract contract;

            /* Firstly retrieve the transfer transaction that is being claimed so that we can get the address of the object */
            TAO::Ledger::Transaction txTransfer;

            /* Check disk of writing new block. */
            if(!LLD::Ledger->ReadTx(hashTransfer, txTransfer, TAO::Ledger::FLAGS::MEMPOOL))
                throw Exception(-99, "Transfer transaction not found.");

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
                    TAO::Register::Address hashAddress;
                    check >> hashAddress;

                    /* Check to see if the caller already has a name for this register in their sig chain */
                    if(!ResolveName(hashGenesis, hashAddress).empty())
                        continue;

                    /* Now check the previous owners Name records to see if there was a Name for this object */
                    std::string strAssetName = ResolveName(txTransfer.hashGenesis, hashAddress);

                    /* If a name was found then create a Name record for the new owner using the same name */
                    if(!strAssetName.empty())
                    {
                        contract = Names::CreateName(hashGenesis, strAssetName, "", hashAddress);
                    }

                    /* If found break. */
                    break;
                }
            }

            return contract;
        }

        /* Retrieves a Name object by name. */
        TAO::Register::Object Names::GetName(const encoding::json& params, const std::string& strObjectName,
                                             TAO::Register::Address& hashNameObject, const bool fThrow)
        {
            /* Declare the name object to return */
            TAO::Register::Object nameObject;

            /* Flag indicating we found a name */
            bool fFound = false;

            /* In order to resolve an object name to a register address we also need to know the namespace.
            *  First we check to see whether a local name exists in the caller's sig chain.
            *  If that doesn't exist then we check to see whether the name contains a single : denoting a username qualified namespace.
            *  If so then we check to see whether the name exists in that users sig chain
            *  If the name contains a double colon then we assume that is namespace qualified and search the name in that namespace
            *  If the name contains no colons and is not a local name then we check to see whether the name exists in the global namespace   */
            std::string strName = strObjectName;
            std::string strNamespace = "";

            /* Declare the namespace hash to use for this object. */
            TAO::Register::Address hashNamespace;

            /* First check the callers local namespace to see if it exists */
            /* Get the session to be used for this API call.  Note we pass in false for fThrow here so that we can check the
               other namespaces after */
            Session& session = Commands::Get<Users>()->GetSession(params, false);

            /* Get the account. */
            const memory::encrypted_ptr<TAO::Ledger::SignatureChain>& user = session.GetAccount();
            if(!user.IsNull())
            {
                /* Set the namespace name to be the user's genesis ID */
                hashNamespace = user->Genesis();

                /* Attempt to Read the Name Object */
                fFound = TAO::Register::GetNameRegister(hashNamespace, strName, nameObject);
            }

            /* If it wasn't in the callers local namespace then check to see if the name was provided in either
               the userspace:name or namespace::name format */
            if(!fFound)
            {
                size_t nNamespacePos = strName.find("::");
                size_t nUserspacePos = strName.find(":");

                /* If there is a namespace delimiter and it appears before the userspace delimiter (if there is one) */
                if(nNamespacePos != std::string::npos
                && (nUserspacePos == std::string::npos || nNamespacePos <= nUserspacePos))
                {
                    /* If the name is namespace::name format then split the namespace and name into separate variables at the first ::*/
                    strNamespace = strName.substr(0, nNamespacePos);
                    strName = strName.substr(nNamespacePos+2);

                    /* Namespace hash is a SK256 hash of the namespace name */
                    hashNamespace = TAO::Register::Address(strNamespace, TAO::Register::Address::NAMESPACE);
                }
                /* If there is a userspace delimiter and it appears before any namespace delimiter (if there is one) */
                else if(nUserspacePos != std::string::npos
                && (nNamespacePos == std::string::npos || nUserspacePos < nNamespacePos))
                {
                    /* If the name is in username:name format then split the username and name into separate variables at the first :*/
                    strNamespace = strName.substr(0, nUserspacePos);
                    strName = strName.substr(nUserspacePos+1);

                    /* get the namespace hash from the user name */
                    hashNamespace = TAO::Ledger::SignatureChain::Genesis(SecureString(strNamespace.c_str()));
                }

                /* Attempt to Read the Name Object if we have either a user or namespace name*/
                if(hashNamespace != 0)
                    fFound = TAO::Register::GetNameRegister(hashNamespace, strName, nameObject);
            }

            /* If it still hasn't been resolved then check the global namespace */
            if(!fFound)
            {
                /* Get the hashNamespace for the global namespace */
                hashNamespace = TAO::Register::Address(TAO::Register::NAMESPACE::GLOBAL, TAO::Register::Address::NAMESPACE);

                /* Attempt to Read the Name Object */
                fFound = TAO::Register::GetNameRegister(hashNamespace, strName, nameObject);
            }

            /* If it wasn't resolved then error */
            if(!fFound && fThrow)
                throw Exception(-101, debug::safe_printstr("Unknown name: ", strObjectName));

            /* Get the address of the Name object to return */
            if(fFound)
                hashNameObject = TAO::Register::Address(strName, hashNamespace, TAO::Register::Address::NAME);

            return nameObject;
        }


        /* Scans the Name records associated with the hashGenesis sig chain to find an entry with a matching hashObject address */
        TAO::Register::Object Names::GetName(const uint256_t& hashGenesis, const TAO::Register::Address& hashObject, TAO::Register::Address& hashNameObject)
        {
            /* Declare the return val */
            TAO::Register::Object nameObject;

            /* LRU register cache by genesis hash.  This caches the vector of register addresses along with the last txid of the
               sig chain, so that we can determine whether any new transactions have been added, invalidating the cache.  */
            static LLD::TemplateLRU<uint256_t, std::pair<uint512_t, std::map<TAO::Register::Address, TAO::Register::Address>>> cache(10);

            /* Get the last transaction. */
            uint512_t hashLast = 0;

            /* Get the last transaction for this genesis.  NOTE that we include the mempool here as there may be registers that
               have been created recently but not yet included in a block*/
            if(!LLD::Ledger->ReadLast(hashGenesis, hashLast, TAO::Ledger::FLAGS::MEMPOOL))
                return nameObject;

            /* Flag indicating the cache is intact */
            bool bCacheValid = false;

            /* Check the cache to see if we have already cached the registers for this sig chain and it is still valid. */
            if(cache.Has(hashGenesis))
            {
                /* The cached register list */
                std::pair<uint512_t, std::map<TAO::Register::Address, TAO::Register::Address>> cacheEntry;

                /* Retrieve the cached name-address map from the LRU cache */
                cache.Get(hashGenesis, cacheEntry);

                /* Check that the hashlast hasn't changed */
                if(cacheEntry.first == hashLast)
                {
                    bCacheValid = true;

                    /* Lookup the name by address if it exists */
                    if(cacheEntry.second.count(hashObject) > 0)
                    {
                        hashNameObject = cacheEntry.second[hashObject];

                        /* Get the object from the register DB.  We can read it as an Object and then check its nType
                        to determine whether or not it is a Name. */
                        TAO::Register::Object object;
                        if(!LLD::Register->ReadState(hashNameObject, object, TAO::Ledger::FLAGS::MEMPOOL))
                            throw Exception(-92, "Name not found.");

                        if(object.nType == TAO::Register::REGISTER::OBJECT)
                        {
                            /* parse object so that the data fields can be accessed */
                            if(!object.Parse())
                                throw Exception(-36, "Failed to parse object register");

                            /* Set the return values */
                            nameObject = object;

                            /* found the name in the cache so return */
                            return nameObject;
                        }
                    }
                }

            }

            /* If we've never cached names for this sig chain or if the cache is out of date, add a blank map to the cache */
            if(!bCacheValid)
                cache.Put(hashGenesis, std::make_pair(hashLast, std::map<TAO::Register::Address, TAO::Register::Address>()));


            /* Not found in cache so have to look it up by scanning all registers */

            /* Get all object registers owned by this sig chain */
            std::vector<TAO::Register::Address> vRegisters;
            if(ListRegisters(hashGenesis, vRegisters))
            {
                /* Iterate through these to find all Name registers */
                for(const auto& hashRegister : vRegisters)
                {
                    /* Initial check that it is a name before we hit the DB to get the address */
                    if(!hashRegister.IsName())
                        continue;

                    /* Get the object from the register DB.  We can read it as an Object and then check its nType
                    to determine whether or not it is a Name. */
                    TAO::Register::Object object;
                    if(!LLD::Register->ReadState(hashRegister, object, TAO::Ledger::FLAGS::MEMPOOL))
                        continue;

                    if(object.nType == TAO::Register::REGISTER::OBJECT)
                    {
                        /* parse object so that the data fields can be accessed */
                        if(!object.Parse())
                            throw Exception(-36, "Failed to parse object register");

                        /* Check the object register standards. */
                        if(object.Standard() != TAO::Register::OBJECTS::NAME)
                            continue;

                        /* Check to see whether the address stored in this Name matches the hash we are searching for*/
                        if(object.get<uint256_t>("address") == hashObject)
                        {
                            /* Set the return values */
                            nameObject = object;
                            hashNameObject = hashRegister;

                            /* The cached register list */
                            std::pair<uint512_t, std::map<TAO::Register::Address, TAO::Register::Address>> cacheEntry;

                            /* Retrieve the cached name-address map from the LRU cache */
                            cache.Get(hashGenesis, cacheEntry);

                            /* Add this name-address to the map */
                            cacheEntry.second[hashObject] = hashNameObject;

                            /* update the address-name map to the LRU cache */
                            cache.Put(hashGenesis, cacheEntry);

                            /* break out since we have a match */
                            break;
                        }
                    }
                }
            }

            return nameObject;
        }
    } /* End API namespace */
} /* End TAO namespace */
