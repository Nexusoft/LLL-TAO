/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/types/client.h>

#include <LLP/include/global.h>

#include <TAO/API/types/authentication.h>

#include <TAO/Ledger/types/merkle.h>
#include <TAO/Ledger/types/client.h>

#include <Legacy/types/merkle.h>

namespace LLD
{

    /** The Database Constructor. To determine file location and the Bytes per Record. **/
    ClientDB::ClientDB(const Config::Static& sector, const Config::Hashmap& keychain)
    : StaticDatabase(sector, keychain)
    {
    }


    /** Default Destructor **/
    ClientDB::~ClientDB()
    {
    }


    /* Writes the best chain pointer to the ledger DB. */
    bool ClientDB::WriteBestChain(const uint1024_t& hashBest)
    {
        return Write(std::string("hashbestchain"), hashBest);
    }


    /* Reads the best chain pointer from the ledger DB. */
    bool ClientDB::ReadBestChain(uint1024_t &hashBest)
    {
        return Read(std::string("hashbestchain"), hashBest);
    }


    /* Reads the best chain pointer from the ledger DB. */
    bool ClientDB::ReadBestChain(memory::atomic<uint1024_t> &atomicBest)
    {
        uint1024_t hashBest = 0;
        if(!Read(std::string("hashbestchain"), hashBest))
            return false;

        atomicBest.store(hashBest);
        return true;
    }


    /* Writes a transaction to the client DB. */
    bool ClientDB::WriteTx(const uint512_t& hashTx, const TAO::Ledger::MerkleTx& tx)
    {
        return Write(hashTx, tx, "tritium");
    }


    /* Reads a transaction from the client DB. */
    bool ClientDB::ReadTx(const uint512_t& hashTx, TAO::Ledger::MerkleTx &tx, const uint8_t nFlags)
    {
        return Read(hashTx, tx);
    }


    /* Writes a transaction to the client DB. */
    bool ClientDB::WriteTx(const uint512_t& hashTx, const Legacy::MerkleTx& tx)
    {
        return Write(hashTx, tx, "legacy");
    }


    /* Reads a transaction from the client DB. */
    bool ClientDB::ReadTx(const uint512_t& hashTx, Legacy::MerkleTx &tx, const uint8_t nFlags)
    {
        return Read(hashTx, tx);
    }


    /* Checks client DB if a transaction exists. */
    bool ClientDB::HasTx(const uint512_t& hashTx, const uint8_t nFlags)
    {
        return Exists(hashTx);
    }


    /* Writes a proof to disk. Proofs are used to keep track of spent temporal proofs. */
    bool ClientDB::WriteProof(const uint256_t& hashProof, const uint512_t& hashTx, const uint32_t nContract)
    {
        /* Handle for single user -client mode. */
        if(config::fClient.load())
        {
            /* Get our caller to check proof under. */
            const uint256_t hashGenesis =
                TAO::API::Authentication::Caller();

            /* Check that we have active session. */
            if(hashGenesis == 0)
                throw debug::exception(FUNCTION, "no session available...");

            /* Get the key typle. */
            const std::tuple<uint256_t, uint256_t, uint512_t, uint32_t> tIndex =
                std::make_tuple(hashGenesis, hashProof, hashTx, nContract);

            return Write(tIndex);
        }

        /* Get the key typle. */
        const std::tuple<uint256_t, uint512_t, uint32_t> tIndex =
            std::make_tuple(hashProof, hashTx, nContract);

        return Write(tIndex);
    }


    /* Writes a proof to disk. Proofs are used to keep track of spent temporal proofs. */
    bool ClientDB::HasProof(const uint256_t& hashProof, const uint512_t& hashTx, const uint32_t nContract, const uint8_t nFlags)
    {
        /* Additional routine if the proof doesn't exist. */
        if(config::fClient.load())
        {
            /* Get our caller to check proof under. */
            const uint256_t hashGenesis =
                TAO::API::Authentication::Caller();

            /* Check that we have active session. */
            if(hashGenesis == 0)
                throw debug::exception(FUNCTION, "no session available...");

            /* Get the key typle. */
            const std::tuple<uint256_t, uint256_t, uint512_t, uint32_t> tIndex =
                std::make_tuple(hashGenesis, hashProof, hashTx, nContract);

            /* Check our proof based on active tuple. */
            if(Exists(tIndex))
                return true;

            /* Handle if we are performing a lookup. */
            if(nFlags == TAO::Ledger::FLAGS::LOOKUP)
            {
                /* Check for -client mode or active server object. */
                if(!LLP::TRITIUM_SERVER || !LLP::LOOKUP_SERVER)
                    throw debug::exception(FUNCTION, "tritium or lookup servers inactive");

                /* Try to find a connection first. */
                std::shared_ptr<LLP::LookupNode> pConnection = LLP::LOOKUP_SERVER->GetConnection();
                if(pConnection == nullptr)
                {
                    /* Attempt to get an active tritium connection for lookup. */
                    std::shared_ptr<LLP::TritiumNode> pNode = LLP::TRITIUM_SERVER->GetConnection();
                    if(pNode != nullptr)
                    {
                        /* Get our lookup address now. */
                        const std::string strAddress =
                            pNode->GetAddress().ToStringIP();

                        /* Make our new connection now. */
                        if(!LLP::LOOKUP_SERVER->ConnectNode(strAddress, pConnection))
                            throw debug::exception(FUNCTION, "failed to connect to node");
                    }
                }

                /* Check that we were able to make a connection. */
                if(!pConnection)
                    throw debug::exception(FUNCTION, "no connections found");

                /* Debug output to console. */
                //debug::log(2, FUNCTION, "CLIENT MODE: Requesting ACTION::GET::PROOF for ", hashTx.SubString());
                pConnection->BlockingLookup
                (
                    5000,
                    LLP::LookupNode::REQUEST::PROOF,
                    uint8_t(LLP::LookupNode::SPECIFIER::TRITIUM),
                    hashProof, hashTx, nContract
                );
                //debug::log(2, FUNCTION, "CLIENT MODE: TYPES::PROOF received for ", hashTx.SubString());
            }

            return Exists(tIndex);
        }

        /* Get the key typle. */
        const std::tuple<uint256_t, uint512_t, uint32_t> tIndex =
            std::make_tuple(hashProof, hashTx, nContract);

        /* Once we have reached this far we will know if proof exists or not. */
        return Exists(tIndex);
    }


    /* Writes a proof to disk. Proofs are used to keep track of spent temporal proofs. */
    bool ClientDB::EraseProof(const uint256_t& hashProof, const uint512_t& hashTx, const uint32_t nContract)
    {
        /* Handle for single user -client mode. */
        if(config::fClient.load())
        {
            /* Get our caller to check proof under. */
            const uint256_t hashGenesis =
                TAO::API::Authentication::Caller();

            /* Check that we have active session. */
            if(hashGenesis == 0)
                throw debug::exception(FUNCTION, "no session available...");

            /* Get the key typle. */
            const std::tuple<uint256_t, uint256_t, uint512_t, uint32_t> tIndex =
                std::make_tuple(hashGenesis, hashProof, hashTx, nContract);

            return Erase(tIndex);
        }

        /* Get the key typle. */
        const std::tuple<uint256_t, uint512_t, uint32_t> tIndex =
            std::make_tuple(hashProof, hashTx, nContract);

        return Erase(tIndex);
    }


    /* Writes an output as spent. */
    bool ClientDB::WriteSpend(const uint512_t& hashTx, const uint32_t nOutput)
    {
        /* Handle for single user -client mode. */
        if(config::fClient.load())
        {
            /* Get our caller to check proof under. */
            const uint256_t hashGenesis =
                TAO::API::Authentication::Caller();

            /* Check that we have active session. */
            if(hashGenesis == 0)
                throw debug::exception(FUNCTION, "no session available...");

            /* Get the key typle. */
            const std::tuple<uint256_t, uint512_t, uint32_t> tIndex =
                std::make_tuple(hashGenesis, hashTx, nOutput);

            return Write(tIndex);
        }

        return Write(std::make_pair(hashTx, nOutput));
    }


    /* Removes a spend flag on an output. */
    bool ClientDB::EraseSpend(const uint512_t& hashTx, const uint32_t nOutput)
    {
        /* Handle for single user -client mode. */
        if(config::fClient.load())
        {
            /* Get our caller to check proof under. */
            const uint256_t hashGenesis =
                TAO::API::Authentication::Caller();

            /* Check that we have active session. */
            if(hashGenesis == 0)
                throw debug::exception(FUNCTION, "no session available...");

            /* Get the key typle. */
            const std::tuple<uint256_t, uint512_t, uint32_t> tIndex =
                std::make_tuple(hashGenesis, hashTx, nOutput);

            return Erase(tIndex);
        }

        return Erase(std::make_pair(hashTx, nOutput));
    }


    /* Checks if an output was spent. */
    bool ClientDB::IsSpent(const uint512_t& hashTx, const uint32_t nOutput, const uint8_t nFlags)
    {
        /* Return if we have it on our disk. */
        if(Exists(std::make_pair(hashTx, nOutput)))
            return true;

        /* Additional routine if the proof doesn't exist. */
        if(config::fClient.load())
        {
            /* Get our caller to check proof under. */
            const uint256_t hashGenesis =
                TAO::API::Authentication::Caller();

            /* Check that we have active session. */
            if(hashGenesis == 0)
                throw debug::exception(FUNCTION, "no session available...");

            /* Get the key typle. */
            const std::tuple<uint256_t, uint512_t, uint32_t> tIndex =
                std::make_tuple(hashGenesis, hashTx, nOutput);

            /* Return if the proof exists or not now. */
            if(Exists(tIndex))
                return true;

            /* Handle if we are performing a lookup. */
            if(nFlags == TAO::Ledger::FLAGS::LOOKUP)
            {
                /* Check for -client mode or active server object. */
                if(!LLP::TRITIUM_SERVER || !LLP::LOOKUP_SERVER)
                    throw debug::exception(FUNCTION, "tritium or lookup servers inactive");

                /* Try to find a connection first. */
                std::shared_ptr<LLP::LookupNode> pConnection = LLP::LOOKUP_SERVER->GetConnection();
                if(pConnection == nullptr)
                {
                    /* Attempt to get an active tritium connection for lookup. */
                    std::shared_ptr<LLP::TritiumNode> pNode = LLP::TRITIUM_SERVER->GetConnection();
                    if(pNode != nullptr)
                    {
                        /* Get our lookup address now. */
                        const std::string strAddress =
                            pNode->GetAddress().ToStringIP();

                        /* Make our new connection now. */
                        if(!LLP::LOOKUP_SERVER->ConnectNode(strAddress, pConnection))
                            throw debug::exception(FUNCTION, "failed to connect to node");
                    }
                }

                /* Check that we were able to make a connection. */
                if(!pConnection)
                    throw debug::exception(FUNCTION, "no connections found");

                /* Debug output to console. */
                //debug::log(2, FUNCTION, "CLIENT MODE: Requesting ACTION::GET::PROOF for ", hashTx.SubString());
                pConnection->BlockingLookup
                (
                    5000,
                    LLP::LookupNode::REQUEST::PROOF,
                    uint8_t(LLP::LookupNode::SPECIFIER::LEGACY),
                    hashTx, nOutput
                );
                //debug::log(2, FUNCTION, "CLIENT MODE: TYPES::PROOF received for ", hashTx.SubString());
            }

            return Exists(tIndex);
        }

        return false;
    }


    /* Writes a block block object to disk. */
    bool ClientDB::WriteBlock(const uint1024_t& hashBlock, const TAO::Ledger::ClientBlock& block)
    {
        return Write(hashBlock, block, "block");
    }


    /* Reads a block block object from disk. */
    bool ClientDB::ReadBlock(const uint1024_t& hashBlock, TAO::Ledger::ClientBlock &block)
    {
        return Read(hashBlock, block);
    }

    /* Reads a client block from disk from a tx index. */
    bool ClientDB::ReadBlock(const uint512_t& hashTx, TAO::Ledger::ClientBlock &block)
    {
        return Read(std::make_pair(std::string("index"), hashTx), block);
    }


    /* Checks if a client block exisets on disk. */
    bool ClientDB::HasBlock(const uint1024_t& hashBlock)
    {
        return Exists(hashBlock);
    }


    /* Erase a block from disk. */
    bool ClientDB::EraseBlock(const uint1024_t& hashBlock)
    {
        return Erase(hashBlock);
    }


    /* Determine if a transaction has already been indexed. */
    bool ClientDB::HasIndex(const uint512_t& hashTx)
    {
        return Exists(std::make_pair(std::string("index"), hashTx));
    }


    /* Index a transaction hash to a block in keychain. */
    bool ClientDB::IndexBlock(const uint512_t& hashTx, const uint1024_t& hashBlock)
    {
        return Index(std::make_pair(std::string("index"), hashTx), hashBlock);
    }


    /* Checks if a genesis transaction exists. */
    bool ClientDB::HasFirst(const uint256_t& hashGenesis)
    {
        return Exists(std::make_pair(std::string("first"), hashGenesis));
    }


    /* Writes a genesis transaction-id to disk. */
    bool ClientDB::WriteFirst(const uint256_t& hashGenesis, const uint512_t& hashTx)
    {
        return Write(std::make_pair(std::string("first"), hashGenesis), hashTx, "first");
    }


    /* Reads a genesis transaction-id from disk. */
    bool ClientDB::ReadFirst(const uint256_t& hashGenesis, uint512_t& hashTx)
    {
        return Read(std::make_pair(std::string("first"), hashGenesis), hashTx);
    }


    /* Writes the last txid of sigchain to disk indexed by genesis. */
    bool ClientDB::WriteLast(const uint256_t& hashGenesis, const uint512_t& hashLast)
    {
        return Write(std::make_pair(std::string("last"), hashGenesis), hashLast, "last");
    }


    /* Erase the last txid of sigchain to disk indexed by genesis. */
    bool ClientDB::EraseLast(const uint256_t& hashGenesis)
    {
        return Erase(std::make_pair(std::string("last"), hashGenesis));
    }


    /* Reads the last txid of sigchain to disk indexed by genesis. */
    bool ClientDB::ReadLast(const uint256_t& hashGenesis, uint512_t& hashLast, const uint8_t nFlags)
    {
        /* Special check for memory pool. */
        if(nFlags == TAO::Ledger::FLAGS::MEMPOOL)
        {
            //get from client mempool
        }

        return Read(std::make_pair(std::string("last"), hashGenesis), hashLast);
    }

}
