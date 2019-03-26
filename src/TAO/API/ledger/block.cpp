
#include <TAO/API/include/ledger.h>

#include <LLD/include/global.h>

#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/types/tritium.h>

#include <TAO/API/include/apiutils.h>

#include <Util/include/string.h>

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {
        /* Retrieves the blockhash for the given height. */
        json::json Ledger::GetBlockHash(const json::json& params, bool fHelp)
        {
            /* Check that the node is configured to index blocks by height */ 
            if(!config::GetBoolArg("-indexheight"))
            {
                throw APIException(-25, "getblockhash requires the daemon to be started with the -indexheight flag.");
            }

            /* Check for the block height parameter. */
            if(params.find("height") == params.end())
                throw APIException(-25, "Missing height");

            /* Check that the height parameter is numeric*/
            std::string strHeight = params["height"].get<std::string>(); 

            if( !IsAllDigit(strHeight))
                throw APIException(-25, "Invalid height parameter");
            
            /* Convert the incoming height string to an int*/                
            uint32_t nHeight = std::stoul(strHeight);

            /* Check that the requested height is within our chain range*/
            if (nHeight > TAO::Ledger::ChainState::nBestHeight.load())
                throw APIException(-25, "Block number out of range.");

            TAO::Ledger::BlockState blockState;
            /* Read the block state from the the ledger DB using the height index */
            if(!LLD::legDB->ReadBlock(nHeight, blockState))
                throw APIException(-25, "Block not found");

            json::json ret;
            ret["hash"] = blockState.GetHash().GetHex();

            return ret;
        }


        /* Retrieves the block data for a given hash or height. */
        json::json Ledger::GetBlock(const json::json& params, bool fHelp)
        {
            /* Check for the block height parameter. */
            if(params.find("hash") == params.end() && params.find("height") == params.end())
                throw APIException(-25, "Missing hash or height");

            /* Declare the BlockState to load from the DB */
            TAO::Ledger::BlockState blockState;

            /* look up by height*/ 
            if(params.find("height") != params.end())
            { 
                /* Check that the node is configured to index blocks by height */ 
                if( !config::GetBoolArg("-indexheight"))
                    throw APIException(-25, "getblock by height requires the daemon to be started with the -indexheight flag.");
            
                /* Check that the height parameter is numeric*/
                std::string strHeight = params["height"].get<std::string>(); 

                if( !IsAllDigit(strHeight))
                    throw APIException(-25, "Invalid height parameter");

                /* Convert the incoming height string to an int*/                
                uint32_t nHeight = std::stoul(strHeight);

                /* Check that the requested height is within our chain range*/
                if (nHeight > TAO::Ledger::ChainState::nBestHeight.load())
                    throw APIException(-25, "Block number out of range.");

                /* Read the block state from the the ledger DB using the height index */
                if(!LLD::legDB->ReadBlock(nHeight, blockState))
                    throw APIException(-25, "Block not found");
            }
            else if(params.find("hash") != params.end())
            {
                uint1024_t blockHash;
                /* Load the hash from the params*/
                blockHash.SetHex(params["hash"].get<std::string>());

                /* Read the block state from the the ledger DB using the hash index */
                if(!LLD::legDB->ReadBlock(blockHash, blockState))
                    throw APIException(-25, "Block not found");
            }
            
            int nTransactionVerbosity = 0;
            if( params.count("txverbose") > 0 && IsAllDigit(params["txverbose"].get<std::string>())) 
                nTransactionVerbosity = atoi(params["txverbose"].get<std::string>().c_str());

            json::json ret = TAO::API::Utils::blockToJSON(blockState, nTransactionVerbosity);

            return ret;
        }


        /* Retrieves the block data for a sequential range of blocks starting at a given hash or height. */
        json::json Ledger::GetBlocks(const json::json& params, bool fHelp)
        {
            /* Check for the block height parameter. */
            if(params.find("hash") == params.end() && params.find("height") == params.end())
                throw APIException(-25, "Missing hash or height");

            /* Declare the BlockState to load from the DB */
            TAO::Ledger::BlockState blockState;

            /* The number of blocks to return, default 10*/
            int nCount = 10;
            if(params.find("count") != params.end())
            {
                std::string strCount = params["count"].get<std::string>();
                if( !IsAllDigit(strCount) )
                    throw APIException(-25, "Invalid count parameter");
                nCount = std::stoi( strCount);

                if( nCount < 1 || nCount > 1000 )
                    throw APIException(-25, "Invalid count parameter");
            }
            /* look up by height*/ 
            if(params.find("height") != params.end())
            { 
                /* Check that the node is configured to index blocks by height */ 
                if( !config::GetBoolArg("-indexheight"))
                    throw APIException(-25, "getblock by height requires the daemon to be started with the -indexheight flag.");
            
                /* Check that the height parameter is numeric*/
                std::string strHeight = params["height"].get<std::string>(); 

                if( !IsAllDigit(strHeight))
                    throw APIException(-25, "Invalid height parameter");

                /* Convert the incoming height string to an int*/                
                uint32_t nHeight = std::stoul(strHeight);

                /* Check that the requested height is within our chain range*/
                if (nHeight > TAO::Ledger::ChainState::nBestHeight.load())
                    throw APIException(-25, "Block number out of range.");

                /* Read the block state from the the ledger DB using the height index */
                if(!LLD::legDB->ReadBlock(nHeight, blockState))
                    throw APIException(-25, "Block not found");
            }
            else if(params.find("hash") != params.end())
            {
                uint1024_t blockHash;
                /* Load the hash from the params*/
                blockHash.SetHex(params["hash"].get<std::string>());

                /* Read the block state from the the ledger DB using the hash index */
                if(!LLD::legDB->ReadBlock(blockHash, blockState))
                    throw APIException(-25, "Block not found");
            }
            
            /* Get the transaction verbosity level from the request*/
            int nTransactionVerbosity = 0;
            if( params.count("txverbose") > 0 && IsAllDigit(params["txverbose"].get<std::string>())) 
                nTransactionVerbosity = atoi(params["txverbose"].get<std::string>().c_str());

            /* Declare the JSON array to return */ 
            json::json ret = json::json::array();

            /* Iterate through nCount number of blocks*/
            for(int i=0; i<nCount; i++)
            {
                /* convert the block to JSON data and add it to the return JSON array*/
                ret.push_back(TAO::API::Utils::blockToJSON(blockState, nTransactionVerbosity));

                /* Move on to the next block in the sequence*/
                blockState = blockState.Next();

                /* Ensure the next block exists*/
                if( !blockState)
                    break;
            }
            return ret;
        }
    }

}