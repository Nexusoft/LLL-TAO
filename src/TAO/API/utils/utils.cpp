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

#include <TAO/Operation/include/stream.h>
#include <TAO/Operation/include/enum.h>
#include <TAO/Operation/include/operations.h>

#include <TAO/Register/include/create.h>

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



        /* Creates a new Name Object register for the given name and register address 
           adds the register operation to the transaction */
        void CreateName( const uint256_t& hashGenesis, const std::string strName, const uint256_t& hashRegister, TAO::Ledger::Transaction& tx)
        {
            /* Build vector to hold the genesis + name data for hashing */
            std::vector<uint8_t> vData;

            /* Insert the geneses hash of the transaction */
            vData.insert(vData.end(), (uint8_t*)&hashGenesis, (uint8_t*)&hashGenesis + 32);
            
            /* Insert the name of from the Name object */
            vData.insert(vData.end(), strName.begin(), strName.end());
            
            /* Hash this in the same was as the caller would have to generate hashAddress */
            uint256_t hashName = LLC::SK256(vData);

            /* Create the Name register object pointing to hashRegister */
            TAO::Register::Object name = TAO::Register::CreateName(strName, hashRegister);

            /* Add the Name object register operation to the transaction */
            tx << uint8_t(TAO::Operation::OP::REGISTER) << hashName << uint8_t(TAO::Register::REGISTER::OBJECT) << name.GetState();
        }


        /* Resolves a register address from a name by looking up the Name object. */
        uint256_t RegisterAddressFromName(const json::json& params, const std::string& strObjectName )
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

            /* If not then check for the explicit namespace parameter*/
            else if(params.find("namespace") != params.end())
            {
                strNamespace = params["namespace"].get<std::string>();
                /* get the namespace hash from the namespace name */
                nNamespaceHash = TAO::Ledger::SignatureChain::Genesis(SecureString(strNamespace.c_str()));
            }

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

            /* Build vector to hold the genesis + name data for hashing */
            std::vector<uint8_t> vData;

            /* Insert the geneses hash of the transaction */
            vData.insert(vData.end(), (uint8_t*)&nNamespaceHash, (uint8_t*)&nNamespaceHash + 32);
            
            /* Insert the name of from the Name object */
            vData.insert(vData.end(), strName.begin(), strName.end());
            
            /* Build the address from an SK256 hash of namespace + name. */
            uint256_t hashName = LLC::SK256(vData);

            /* Read the Name Object */
            TAO::Register::Object object;
            if(!LLD::regDB->ReadState(hashName, object, TAO::Register::FLAGS::MEMPOOL))
                throw APIException(-24, "Unknown name.");

            if(object.nType != TAO::Register::REGISTER::OBJECT)
                throw APIException(-24, "Unknown name.");
            
            object.Parse();

            /* Check that this is a Name register */
            if( object.Standard() != TAO::Register::OBJECTS::NAME)
                throw APIException(-24, "Unknown name");
        
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
        uint64_t GetTokenOrAccountDigits(const TAO::Register::Object& object)
        {
            /* Declare the nDigits to return */
            uint64_t nDigits = 0;

            /* Get the object standard. */
            uint8_t nStandard = object.Standard();

            /* Check the object standard. */
            if( nStandard == TAO::Register::OBJECTS::TOKEN)
            {
                nDigits = object.get<uint64_t>("digits");
            }
            else if(nStandard == TAO::Register::OBJECTS::TRUST)
            {
                    nDigits = 1000000; // NXS token default digits
            }
            else if(nStandard == TAO::Register::OBJECTS::ACCOUNT)
            {

                /* If debiting an account we need to look at the token definition in order to get the digits. 
                   The token is obtained by looking at the token_address field, which contains the register address of
                   the issuing token */
                uint256_t nIdentifier = object.get<uint256_t>("token_address");

                /* Edge case for NXS token which has identifier 0, so no look up needed */
                if( nIdentifier == 0)
                    nDigits = 1000000;
                else
                {
                    
                    TAO::Register::Object token;
                    if(!LLD::regDB->ReadState(nIdentifier, token))
                        throw APIException(-24, "Token not found");

                    /* Parse the object register. */
                    if(!token.Parse())
                        throw APIException(-24, "Object failed to parse");

                    nDigits = token.get<uint64_t>("digits");
                }   
            }
            else
            {
                throw APIException(-27, "Unknown token / account." );
            }

            return nDigits;
        }


        /* Retrieves the token name for the token that this account object is used for. 
        *  The token is obtained by looking at the token_address field, 
        *  which contains the register address of the issuing token */
        std::string GetTokenNameForAccount(const TAO::Register::Object& object)
        {
            /* Declare token name to return  */
            std::string strTokenName;
            /* Get the object standard. */
            uint8_t nStandard = object.Standard();

            if(nStandard == TAO::Register::OBJECTS::ACCOUNT)
            {

                /* If debiting an account we need to look at the token definition in order to get the digits. 
                   The token is obtained by looking at the token_address field, which contains the register address of
                   the issuing token */
                uint256_t nIdentifier = object.get<uint256_t>("token_address");

                /* Edge case for NXS token which has identifier 0, so no look up needed */
                if( nIdentifier == 0)
                    strTokenName = "NXS";
                else
                {
                    
                    TAO::Register::Object token;
                    if(!LLD::regDB->ReadState(nIdentifier, token))
                        throw APIException(-24, "Token not found");

                    /* Parse the object register. */
                    if(!token.Parse())
                        throw APIException(-24, "Object failed to parse");

                    strTokenName = "TODO";
                }   
            }
            else
            {
                throw APIException(-27, "Object is not an account." );
            }

            return strTokenName;
        }


        /* Scans a signature chain to work out all registers that it owns */
        std::vector<uint256_t> GetRegistersOwnedBySigChain(uint512_t hashGenesis)
        {
            /* Get the last transaction. */
            uint512_t hashLast = 0;
            if(!LLD::legDB->ReadLast(hashGenesis, hashLast))
                throw APIException(-28, "No transactions found");

            /* In order to work out which registers are currently owned by a particular sig chain
               we must iterate through all of the transactions for the sig chain and track the history 
               of each register.  By  iterating through the transactions from most recent backwards we 
               can make some assumptions about the current owned state.  For example if we find a debit 
               transaction for a register before finding a transfer then we must know we currently own it. 
               Similarly if we find a transfer transaction for a register before any other transaction 
               then we must know we currently to NOT own it. */

            /* Keep a running list of owned and transferred registers. We use a set to store these registers because 
               we are going to be checking them frequently to see if a hash is already in the container, 
               and a set offers us near linear search time*/
            std::unordered_set<uint256_t> vTransferredRegisters;
            std::unordered_set<uint256_t> vOwnedRegisters;
            
            /* For also put the owned register hashes in a vector as we want to maintain the insertion order, with the 
               most recently updated register first*/
            std::vector<uint256_t> vRegisters;

            /* Loop until genesis. */
            while(hashLast != 0)
            {
                /* Get the transaction from disk. */
                TAO::Ledger::Transaction tx;
                if(!LLD::legDB->ReadTx(hashLast, tx))
                    throw APIException(-28, "Failed to read transaction");

                /* Set the next last. */
                hashLast = tx.hashPrevTx;

                /* Iterate through the operations in each transaction */
                /* Start the stream at the beginning. */
                tx.ssOperation.seek(0, STREAM::BEGIN);

                while(!tx.ssOperation.end())
                {
                    uint8_t OPERATION;
                    tx.ssOperation >> OPERATION;

                    /* Check the current opcode. */
                    switch(OPERATION)
                    {
                        /* These are the register-based operations that prove ownership if encountered before a transfer*/
                        case TAO::Operation::OP::WRITE:
                        case TAO::Operation::OP::APPEND:
                        case TAO::Operation::OP::REGISTER:
                        case TAO::Operation::OP::DEBIT:
                        case TAO::Operation::OP::TRUST:
                        {
                            /* Extract the address from the ssOperation. */
                            uint256_t hashAddress;
                            tx.ssOperation >> hashAddress;

                            /* for these operations, if the address is NOT in the transferred list 
                               then we know that we must currently own this register */
                            if( vTransferredRegisters.find(hashAddress) == vTransferredRegisters.end() 
                            && vOwnedRegisters.find(hashAddress) == vOwnedRegisters.end())
                            {
                                vOwnedRegisters.insert(hashAddress);
                                vRegisters.push_back(hashAddress);
                            }

                            break;
                        }
                        case TAO::Operation::OP::CREDIT:
                        {
                            /* The transaction that this credit is claiming. */
                            uint512_t hashTx;
                            tx.ssOperation >> hashTx;

                            /* The proof this credit is using to make claims. */
                            uint256_t hashProof;
                            tx.ssOperation >> hashProof;

                            /* The account that is being credited. */
                            uint256_t hashAddress;
                            tx.ssOperation >> hashAddress;

                            /* If we find a credit before a transfer transaction for this register then 
                               we can know for certain that we must own it */
                            if( vTransferredRegisters.find(hashAddress) == vTransferredRegisters.end() 
                            && vOwnedRegisters.find(hashAddress) == vOwnedRegisters.end())
                            {
                                vOwnedRegisters.insert(hashAddress);
                                vRegisters.push_back(hashAddress);
                            }

                            break;

                        }
                        case TAO::Operation::OP::TRANSFER:
                        {
                            /* Extract the address from the ssOperation. */
                            uint256_t hashAddress;
                            tx.ssOperation >> hashAddress;

                            /* If we find a TRANSFER before any other transaction for this register then we can know 
                               for certain that we no longer own it */
                            if( vOwnedRegisters.find(hashAddress) == vOwnedRegisters.end() 
                            && vTransferredRegisters.find(hashAddress) == vTransferredRegisters.end())
                            {
                                vTransferredRegisters.insert(hashAddress);
                            }
                            
                            if( std::find(vOwnedRegisters.begin(), vOwnedRegisters.end(), hashAddress) == vOwnedRegisters.end() )
                                vTransferredRegisters.insert(hashAddress);
                            
                            break;
                        }

                        case TAO::Operation::OP::CLAIM:
                        {
                            /* Extract the tx id of the corresponding transfer from the ssOperation. */
                            uint512_t hashTransferTx;
                            tx.ssOperation >> hashTransferTx;

                            /* Read the claimed transaction. */
                            TAO::Ledger::Transaction txClaim;
                            
                            /* Check disk of writing new block. */
                            if((!LLD::legDB->ReadTx(hashTransferTx, txClaim) || !LLD::legDB->HasIndex(hashTransferTx)))
                                debug::error(FUNCTION, hashTransferTx.ToString(), " tx doesn't exist or not indexed");

                            /* Check mempool or disk if not writing. */
                            else if(!TAO::Ledger::mempool.Get(hashTransferTx, txClaim)
                            && !LLD::legDB->ReadTx(hashTransferTx, txClaim))
                                debug::error(FUNCTION, hashTransferTx.ToString(), " tx doesn't exist");

                            /* Extract the state from tx. */
                            uint8_t TX_OP;
                            txClaim.ssOperation >> TX_OP;

                            /* Extract the address  */
                            uint256_t hashAddress;
                            txClaim.ssOperation >> hashAddress;
                            

                            /* If we find a CLAIM transaction before a TRANSFER, then we know that we must currently own this register */
                            if( vTransferredRegisters.find(hashAddress) == vTransferredRegisters.end() 
                            && vOwnedRegisters.find(hashAddress) == vOwnedRegisters.end())
                            {
                                vOwnedRegisters.insert(hashAddress);
                                vRegisters.push_back(hashAddress);
                            }
                            
                            break;
                        }

                    }

                    /* Seek the operation stream forward to the end of the operation by 
                       deserializing the data based on the operation type */
                    switch(OPERATION)
                    {
                        case TAO::Operation::OP::WRITE:
                        case TAO::Operation::OP::APPEND:
                        {
                            std::vector<uint8_t> vchData;
                            tx.ssOperation >> vchData;
                            break;
                        }
                        case TAO::Operation::OP::REGISTER:
                        {
                            uint8_t nType;
                            tx.ssOperation >> nType;

                            std::vector<uint8_t> vchData;
                            tx.ssOperation >> vchData;
                            break;

                        }
                        case TAO::Operation::OP::DEBIT:
                        {
                            uint256_t hashTransfer;   
                            tx.ssOperation >> hashTransfer;

                            uint64_t  nAmount;  
                            tx.ssOperation >> nAmount;
                            break;

                        }
                        case TAO::Operation::OP::CREDIT:
                        {
                            /* Nothing to seek forward for a CREDIT as we already deserialized to the 
                               end of the operation in order to get the account address.   */
                            break;
                        }
                        case TAO::Operation::OP::TRUST:
                        {
                            uint1024_t hashLastTrust;
                            tx.ssOperation >> hashLastTrust;

                            uint32_t nSequence;
                            tx.ssOperation >> nSequence;

                            uint64_t nTrust;
                            tx.ssOperation >> nTrust;

                            uint64_t  nStake;
                            tx.ssOperation >> nStake;
                            break;

                        }
                        case TAO::Operation::OP::TRANSFER:
                        {
                            uint256_t hashTransfer;
                            tx.ssOperation >> hashTransfer;
                            
                            break;
                        }
                        case TAO::Operation::OP::CLAIM:
                        {
                            /* Nothing to seek forward for a CLAIM as we already deserialized to the 
                               end of the operation in order to get the txid. */
                            
                            break;
                        }

                    }
                }

            }

            return vRegisters;
        }
    }
}
