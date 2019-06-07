/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/
#include <unordered_set>

#include <Legacy/include/evaluate.h>

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

#include <Util/include/args.h>
#include <Util/include/hex.h>
#include <Util/include/json.h>
#include <Util/include/base64.h>
#include <Util/include/debug.h>

#include <LLC/hash/argon2.h>



/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        /* Generates a lightweight argon2 hash of the namespace string.*/
        uint256_t NamespaceHash(const SecureString& strNamespace)
        {
            /* Generate the Secret Phrase */
            std::vector<uint8_t> vNamespace(strNamespace.begin(), strNamespace.end());

            // low-level API
            std::vector<uint8_t> vHash(32);
            std::vector<uint8_t> vSalt(16);

            /* Create the hash context. */
            argon2_context context =
            {
                /* Hash Return Value. */
                &vHash[0],
                32,

                /* Password input data. */
                &vNamespace[0],
                static_cast<uint32_t>(vNamespace.size()),

                /* The salt for usernames */
                &vSalt[0],
                static_cast<uint32_t>(vSalt.size()),

                /* Optional secret data */
                NULL, 0,

                /* Optional associated data */
                NULL, 0,

                /* Computational Cost. */
                10,

                /* Memory Cost (4 MB). */
                (1 << 12),

                /* The number of threads and lanes */
                1, 1,

                /* Algorithm Version */
                ARGON2_VERSION_13,

                /* Custom memory allocation / deallocation functions. */
                NULL, NULL,

                /* By default only internal memory is cleared (pwd is not wiped) */
                ARGON2_DEFAULT_FLAGS
            };

            /* Run the argon2 computation. */
            int32_t nRet = argon2id_ctx(&context);
            if(nRet != ARGON2_OK)
                throw std::runtime_error(debug::safe_printstr(FUNCTION, "Argon2 failed with code ", nRet));

            /* Set the bytes for the key. */
            uint256_t hashKey;
            hashKey.SetBytes(vHash);

            return hashKey;
        }


        /* Creates a new Name Object register for the given name and register address */
        void CreateName(const uint256_t& hashGenesis, const std::string strName,
                        const uint256_t& hashRegister, TAO::Operation::Contract& contract)
        {
            uint256_t hashNameAddress;

            /* Obtain the name register address for the genesis/name combination */
            TAO::Register::GetNameAddress(hashGenesis, strName, hashNameAddress);

            /* Check to see whether the name already exists  */
            TAO::Register::Object object;
            if(LLD::Register->ReadState(hashNameAddress, object, TAO::Ledger::FLAGS::MEMPOOL))
                throw APIException(-23, "An object with this name already exists for this user.");

            /* Create the Name register object pointing to hashRegister */
            TAO::Register::Object name = TAO::Register::CreateName(strName, hashRegister);

            /* Add the Name object register operation to the transaction */
            contract << uint8_t(TAO::Operation::OP::CREATE) << hashNameAddress << uint8_t(TAO::Register::REGISTER::OBJECT) << name.GetState();
        }


        /* Creates a new Name Object register for an object being transferred */
        TAO::Operation::Contract CreateNameFromTransfer(const uint512_t& hashTransfer, const uint256_t& hashGenesis)
        {
            /* Declare the contract for the response */
            TAO::Operation::Contract contract;

            /* Firstly retrieve the transfer transaction that is being claimed so that we can get the address of the object */
            TAO::Ledger::Transaction txPrev;;

            /* Check disk of writing new block. */
            if(!LLD::Ledger->ReadTx(hashTransfer, txPrev, TAO::Ledger::FLAGS::MEMPOOL))
                debug::error(FUNCTION, hashTransfer.SubString(), " transfer tx doesn't exist or not indexed");

            /* Ensure we are not claiming our own Transfer.  If we are then no need to create a Name object as we already have one */
            if(txPrev.hashGenesis != hashGenesis)
            {
                /* Loop through the contracts. */
                for(uint32_t nContract = 0; nContract < txPrev.Size(); ++nContract)
                {
                    /* Get a reference of the contract. */
                    const TAO::Operation::Contract& check = txPrev[nContract];

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
                    std::string strAssetName = GetRegisterName(hashAddress, txPrev.hashGenesis, txPrev.hashGenesis);

                    /* If a name was found then create a Name record for the new owner using the same name */
                    if(!strAssetName.empty())
                        CreateName(hashGenesis, strAssetName, hashAddress, contract);

                    /* If found break. */
                    break;
                }
            }

            return contract;
        }


        /* Updates the register address of a Name object */
        void UpdateName(const uint256_t& hashGenesis, const std::string strName,
                        const uint256_t& hashRegister, TAO::Operation::Contract& contract)
        {
            uint256_t hashNameAddress;

            /* Obtain a new name register address for the updated genesis/name combination */
            TAO::Register::GetNameAddress(hashGenesis, strName, hashNameAddress);

            /* Check to see whether the name already. */
            TAO::Register::Object object;
            if(LLD::Register->ReadState(hashNameAddress, object, TAO::Ledger::FLAGS::MEMPOOL))
                throw APIException(-23, "An object with this name already exists for this user.");

            /* Create a new Name register object pointing to hashRegister */
            TAO::Register::Object name = TAO::Register::CreateName(strName, hashRegister);

            /* Add the Name object register operation to the transaction */
            contract << uint8_t(TAO::Operation::OP::CREATE) << hashNameAddress << uint8_t(TAO::Register::REGISTER::OBJECT) << name.GetState();
        }


        /* Resolves a register address from a name by looking up the Name object. */
        uint256_t RegisterAddressFromName(const json::json& params, const std::string& strObjectName)
        {
            uint256_t hashRegister = 0;

            /* In order to resolve an object name to a register address we also need to know the namespace.
            *  This must either be provided by the caller explicitly in a namespace parameter or by passing
            *  the name in the format namespace:name.  However since the default namespace is the username
            *  of the sig chain that created the object, if no namespace is explicitly provided we will
            *  also try using the username of currently logged in sig chain */
            std::string strName = strObjectName;
            std::string strNamespace = "";

            /* Declare the namespace hash to use for this object. */
            uint256_t nNamespaceHash = 0;

            /* First check to see if the name parameter has been provided in the namespace:name format*/
            size_t nPos = strName.find(":");
            if(nPos != std::string::npos)
            {
                strNamespace = strName.substr(0, nPos);
                strName = strName.substr(nPos+1);

                /* get the namespace hash from the namespace name */
                nNamespaceHash = TAO::Ledger::SignatureChain::Genesis(SecureString(strNamespace.c_str()));
            }

            // ***namespace parameter removed***
            // /* If not then check for the explicit namespace parameter*/
            // else if(params.find("namespace") != params.end())
            // {
            //     strNamespace = params["namespace"].get<std::string>();

            //     /* Get the namespace hash from the namespace name */
            //     nNamespaceHash = TAO::Ledger::SignatureChain::Genesis(SecureString(strNamespace.c_str()));
            // }

            /* If neither of those then check to see if there is an active session to access the sig chain */
            else
            {
                /* Get the session to be used for this API call.  Note we pass in false for fThrow here so that we can
                   throw a missing namespace exception if no valid session could be found */
                uint64_t nSession = users->GetSession(params, false);

                /* Get the account. */
                memory::encrypted_ptr<TAO::Ledger::SignatureChain>& user = users->GetAccount(nSession);
                if(!user)
                    throw APIException(-23, "Missing namespace parameter");

                nNamespaceHash = user->Genesis();
            }

            /* Read the Name Object */
            TAO::Register::Object object;
            if(!TAO::Register::GetNameRegister(nNamespaceHash, strName, object))
            {
                if(strNamespace.empty())
                    throw APIException(-24, debug::safe_printstr( "Unknown name: ", strName));
                else
                    throw APIException(-24, debug::safe_printstr( "Unknown name: ", strNamespace, ":", strName));
            }

            /* Get the address that this name register is pointing to */
            hashRegister = object.get<uint256_t>("address");

            return hashRegister;
        }


        /* Determins whether a string value is a register address.
        *  This only checks to see if the value is 64 characters in length and all hex characters (i.e. can be converted to a uint256).
        *  It does not check to see whether the register address exists in the database
        */
        bool IsRegisterAddress(const std::string& strValueToCheck)
        {
            return strValueToCheck.length() == 64 && strValueToCheck.find_first_not_of("0123456789abcdefABCDEF", 0) == std::string::npos;
        }


        /* Retrieves the number of digits that applies to amounts for this token or account object.
        *  If the object register passed in is a token account then we need to look at the token definition
        *  in order to get the digits.  The token is obtained by looking at the identifier field,
        *  which contains the register address of the issuing token
        */
        uint64_t GetDigits(const TAO::Register::Object& object)
        {
            /* Declare the nDigits to return */
            uint64_t nDigits = 0;

            /* Get the object standard. */
            uint8_t nStandard = object.Standard();

            /* Check the object standard. */
            if(nStandard == TAO::Register::OBJECTS::TOKEN)
            {
                nDigits = object.get<uint64_t>("digits");
            }
            else if(nStandard == TAO::Register::OBJECTS::TRUST)
            {
                    nDigits = TAO::Ledger::NXS_DIGITS; // NXS token default digits
            }
            else if(nStandard == TAO::Register::OBJECTS::ACCOUNT)
            {

                /* If debiting an account we need to look at the token definition in order to get the digits.
                   The token is obtained by looking at the token_address field, which contains the register address of
                   the issuing token */
                uint256_t nIdentifier = object.get<uint256_t>("token");

                /* Edge case for NXS token which has identifier 0, so no look up needed */
                if(nIdentifier == 0)
                    nDigits = TAO::Ledger::NXS_DIGITS;
                else
                {

                    TAO::Register::Object token;
                    if(!LLD::Register->ReadState(nIdentifier, token))
                        throw APIException(-24, "Token not found");

                    /* Parse the object register. */
                    if(!token.Parse())
                        throw APIException(-24, "Object failed to parse");

                    nDigits = token.get<uint64_t>("digits");
                }
            }
            else
            {
                throw APIException(-27, "Unknown token / account.");
            }

            return nDigits;
        }


        /* Retrieves the token name for the token that this account object is used for.
        *  The token is obtained by looking at the token_address field,
        *  which contains the register address of the issuing token */
        std::string GetTokenNameForAccount(const uint256_t& hashCaller, const TAO::Register::Object& object)
        {
            /* Declare token name to return  */
            std::string strTokenName;

            /* Get the object standard. */
            uint8_t nStandard = object.Standard();

            /* Check the object register standard. */
            if(nStandard == TAO::Register::OBJECTS::ACCOUNT)
            {
                /* If debiting an account we need to look at the token definition in order to get the digits.
                   The token is obtained by looking at the token_address field, which contains the register address of
                   the issuing token */
                uint256_t nIdentifier = object.get<uint256_t>("token");

                /* Edge case for NXS token which has identifier 0, so no look up needed */
                if(nIdentifier == 0)
                    strTokenName = "NXS";
                else
                {

                    TAO::Register::Object token;
                    if(!LLD::Register->ReadState(nIdentifier, token))
                        throw APIException(-24, "Token not found");

                    /* Parse the object register. */
                    if(!token.Parse())
                        throw APIException(-24, "Object failed to parse");

                    /* Look up the token name based on the Name records in thecaller's sig chain */
                    strTokenName = GetRegisterName(nIdentifier, hashCaller, token.hashOwner);

                }
            }
            else
                throw APIException(-27, "Object is not an account.");

            return strTokenName;
        }


        /* In order to work out which registers are currently owned by a particular sig chain
         * we must iterate through all of the transactions for the sig chain and track the history
         * of each register.  By iterating through the transactions from most recent backwards we
         * can make some assumptions about the current owned state.  For example if we find a debit
         * transaction for a register before finding a transfer then we must know we currently own it.
         * Similarly if we find a transfer transaction for a register before any other transaction
         * then we must know we currently to NOT own it.
         */
        bool ListRegisters(const uint256_t& hashGenesis, std::vector<uint256_t>& vRegisters)
        {
            /* Get the last transaction. */
            uint512_t hashLast = 0;

            /* Get the last transaction for this genesis.  NOTE that we include the mempool here as there may be registers that
               have been created recently but not yet included in a block*/
            if(!LLD::Ledger->ReadLast(hashGenesis, hashLast, TAO::Ledger::FLAGS::MEMPOOL))
                return false;

            /* Keep a running list of owned and transferred registers. We use a set to store these registers because
             * we are going to be checking them frequently to see if a hash is already in the container,
             * and a set offers us near linear search time
             */
            std::unordered_set<uint256_t> vTransferred;
            std::unordered_set<uint256_t> vOwnedRegisters;

            /* Loop until genesis. */
            while(hashLast != 0)
            {
                /* Get the transaction from disk. */
                TAO::Ledger::Transaction tx;
                if(!LLD::Ledger->ReadTx(hashLast, tx, TAO::Ledger::FLAGS::MEMPOOL))
                    throw APIException(-28, "Failed to read transaction");

                /* Set the next last. */
                hashLast = tx.hashPrevTx;

                /* Iterate through all contracts. */
                for(uint32_t nContract = 0; nContract < tx.Size(); ++nContract)
                {
                    /* Get the contract output. */
                    const TAO::Operation::Contract& contract = tx[nContract];

                    /* Seek to start of the operation stream in case this a transaction from mempool that has already been read*/
                    contract.Seek(0);

                    /* Deserialize the OP. */
                    uint8_t OP = 0;
                    contract >> OP;

                    /* Check the current opcode. */
                    switch(OP)
                    {

                        /* These are the register-based operations that prove ownership if encountered before a transfer*/
                        case TAO::Operation::OP::WRITE:
                        case TAO::Operation::OP::APPEND:
                        case TAO::Operation::OP::CREATE:
                        case TAO::Operation::OP::DEBIT:
                        {
                            /* Extract the address from the contract. */
                            uint256_t hashAddress = 0;
                            contract >> hashAddress;

                            /* for these operations, if the address is NOT in the transferred list
                               then we know that we must currently own this register */
                           if(vTransferred.find(hashAddress)    == vTransferred.end()
                           && vOwnedRegisters.find(hashAddress) == vOwnedRegisters.end())
                           {
                               /* Add to owned set. */
                               vOwnedRegisters.insert(hashAddress);

                               /* Add to return vector. */
                               vRegisters.push_back(hashAddress);
                           }

                            break;
                        }


                        /* Check for credits here. */
                        case TAO::Operation::OP::CREDIT:
                        {
                            /* Seek past irrelevant data. */
                            contract.Seek(69);

                            /* The account that is being credited. */
                            uint256_t hashAddress = 0;
                            contract >> hashAddress;

                            /* If we find a credit before a transfer transaction for this register then
                               we can know for certain that we must own it */
                           if(vTransferred.find(hashAddress)    == vTransferred.end()
                           && vOwnedRegisters.find(hashAddress) == vOwnedRegisters.end())
                           {
                               /* Add to owned set. */
                               vOwnedRegisters.insert(hashAddress);

                               /* Add to return vector. */
                               vRegisters.push_back(hashAddress);
                           }

                            break;

                        }


                        /* Check for a transfer here. */
                        case TAO::Operation::OP::TRANSFER:
                        {
                            /* Extract the address from the contract. */
                            uint256_t hashAddress = 0;
                            contract >> hashAddress;

                            /* Read the register transfer recipient. */
                            uint256_t hashTransfer = 0;
                            contract >> hashTransfer;

                            /* Read the force transfer flag */
                            uint8_t nType = 0;
                            contract >> nType;

                            /* If we have transferred to a token that we own then we ignore the transfer as we still
                               technically own the register */
                            if(nType == TAO::Operation::TRANSFER::FORCE)
                            {
                                TAO::Register::Object newOwner;
                                if(!LLD::Register->ReadState(hashTransfer, newOwner))
                                    throw APIException(-24, "Transfer recipient object not found");

                                if(newOwner.hashOwner == hashGenesis)
                                    break;
                            }

                            /* If we find a TRANSFER then we can know for certain that we no longer own it */
                            if(vTransferred.find(hashAddress)    == vTransferred.end())
                                vTransferred.insert(hashAddress);

                            break;
                        }


                        /* Check for claims here. */
                        case TAO::Operation::OP::CLAIM:
                        {
                            /* Seek past irrelevant data. */
                            contract.Seek(69);

                            /* Extract the address from the contract. */
                            uint256_t hashAddress = 0;
                            contract >> hashAddress;

                            /* If we find a CLAIM transaction before a TRANSFER, then we know that we must currently own this register */
                            if(vTransferred.find(hashAddress)    == vTransferred.end()
                            && vOwnedRegisters.find(hashAddress) == vOwnedRegisters.end())
                            {
                                /* Add to owned set. */
                                vOwnedRegisters.insert(hashAddress);

                                /* Add to return vector. */
                                vRegisters.push_back(hashAddress);
                            }

                            break;
                        }

                    }
                }
            }

            return true;
        }


        /* Scans the Name records associated with the hashCaller sig chain to find an entry with a matching hashObject address */
        std::string GetRegisterName(const uint256_t& hashObject, const uint256_t& hashCaller, const uint256_t& hashOwner)
        {
            /* Declare the return val */
            std::string strName = "";

            /* If the owner of the object is not the caller, then check to see whether the owner is another object owned by
               the caller.  This would be the case for a tokenized asset */
            uint256_t hashOwnerOwner = 0;
            if(hashCaller != hashOwner)
            {
                TAO::Register::Object owner;
                if(LLD::Register->ReadState(hashOwner, owner, TAO::Ledger::FLAGS::MEMPOOL))
                   hashOwnerOwner = owner.hashOwner;
            }

            /* If the caller is the object owner then attempt to find a Name record to look up the Name of this object */
            if(hashCaller == hashOwner || hashCaller == hashOwnerOwner)
            {
                /* Firstly get all object registers owned by this sig chain */
                std::vector<uint256_t> vRegisters;
                if(!ListRegisters(hashCaller, vRegisters))
                    throw APIException(-24, "No registers found");

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
                            throw APIException(-24, "Failed to parse object register");

                        /* Check the object register standards. */
                        if(object.Standard() != TAO::Register::OBJECTS::NAME)
                            continue;

                        /* Check to see whether the address stored in this Name matches the object being conveted to JSON */
                        if(object.get<uint256_t>("address") == hashObject)
                        {
                            /* Get the name from the Name register and break out since we have a match */
                            strName = object.get<std::string>("name");
                            break;
                        }
                    }
                }
            }

            return strName;
        }
    }
}
