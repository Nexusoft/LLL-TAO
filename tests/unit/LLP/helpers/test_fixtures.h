/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once

#include <LLP/include/stateless_miner.h>
#include <LLP/include/falcon_auth.h>
#include <LLP/include/disposable_falcon.h>
#include <LLP/packets/packet.h>

#include <LLC/include/random.h>
#include <LLC/hash/SK.h>
#include <Util/include/runtime.h>
#include <Util/include/hex.h>

#include <vector>
#include <string>
#include <cstdint>

namespace TestFixtures
{
    /** Test Constants **/
    namespace Constants
    {
        /* Sample genesis hashes for testing */
        const char* GENESIS_1 = "a174011c93ca1c80bca5388382b167cacd33d3154395ea8f45ac99a8308cd122";
        const char* GENESIS_2 = "b285122d04db2d91cdb6499493c278dbe244e4265506fb9f56bd0ab409de0233";
        const char* GENESIS_3 = "c396233e15ec3ea2dec7510504d389ecf355f5376617ca0a67ce1bc50aef1344";
        const char* GENESIS_4 = "d407344f26fd4fb3efd8621615e49afde466f6487728db1b78df2cd61bf02455";
        
        /* Sample register addresses (base58 decoded to uint256_t) */
        const char* REGISTER_ADDR_1 = "e518455f37fe5fb4f0e9732726f5abfgf577g7598839ec2c89eg3de720g13566";
        const char* REGISTER_ADDR_2 = "f629566g48gf6gc5g1fa843837g6bcghg688h8609940fd3d90fh4ef831h24677";
        
        /* Sample Falcon public key sizes */
        const size_t FALCON_PUBKEY_SIZE = 897;
        const size_t FALCON_SIGNATURE_SIZE = 690;
        
        /* Sample nonce sizes */
        const size_t AUTH_NONCE_SIZE = 32;
        
        /* ChaCha20 key size */
        const size_t CHACHA_KEY_SIZE = 32;
        
        /* Default session timeout (24 hours) */
        const uint64_t DEFAULT_SESSION_TIMEOUT = 86400;
    }
    
    /** CreateTestGenesis
     *
     *  Creates a deterministic genesis hash from a hex string.
     *
     *  @param[in] hexString The hex string to convert (default: GENESIS_1)
     *  @return A uint256_t genesis hash
     *
     **/
    inline uint256_t CreateTestGenesis(const char* hexString = Constants::GENESIS_1)
    {
        uint256_t hash;
        hash.SetHex(hexString);
        return hash;
    }
    
    /** CreateTestRegisterAddress
     *
     *  Creates a test register address (simulating a decoded base58 NXS address).
     *
     *  @param[in] hexString The hex string to convert
     *  @return A uint256_t register address
     *
     **/
    inline uint256_t CreateTestRegisterAddress(const char* hexString = Constants::REGISTER_ADDR_1)
    {
        uint256_t hash;
        hash.SetHex(hexString);
        return hash;
    }
    
    /** CreateRandomHash
     *
     *  Creates a random uint256_t hash for testing.
     *
     *  @return A random uint256_t hash
     *
     **/
    inline uint256_t CreateRandomHash()
    {
        return LLC::GetRand256();
    }
    
    /** CreateTestNonce
     *
     *  Creates a test authentication nonce.
     *
     *  @param[in] size The size of the nonce (default: 32)
     *  @return A vector of random bytes
     *
     **/
    inline std::vector<uint8_t> CreateTestNonce(size_t size = Constants::AUTH_NONCE_SIZE)
    {
        if(size == 32)
            return LLC::GetRand256().GetBytes();
        
        /* For non-standard sizes, build from multiple random values */
        std::vector<uint8_t> vNonce;
        while(vNonce.size() < size)
        {
            std::vector<uint8_t> vChunk = LLC::GetRand256().GetBytes();
            size_t remaining = size - vNonce.size();
            size_t toAdd = (remaining < vChunk.size()) ? remaining : vChunk.size();
            vNonce.insert(vNonce.end(), vChunk.begin(), vChunk.begin() + toAdd);
        }
        return vNonce;
    }
    
    /** CreateTestFalconPubKey
     *
     *  Creates a mock Falcon public key for testing.
     *  NOTE: This is not a valid cryptographic key, just for structural testing.
     *
     *  @param[in] size The size of the public key (default: FALCON_PUBKEY_SIZE)
     *  @return A vector representing a mock public key
     *
     **/
    inline std::vector<uint8_t> CreateTestFalconPubKey(size_t size = Constants::FALCON_PUBKEY_SIZE)
    {
        /* Build from multiple random values */
        std::vector<uint8_t> vPubKey;
        while(vPubKey.size() < size)
        {
            std::vector<uint8_t> vChunk = LLC::GetRand256().GetBytes();
            size_t remaining = size - vPubKey.size();
            size_t toAdd = (remaining < vChunk.size()) ? remaining : vChunk.size();
            vPubKey.insert(vPubKey.end(), vChunk.begin(), vChunk.begin() + toAdd);
        }
        return vPubKey;
    }
    
    /** CreateTestChaChaKey
     *
     *  Creates a test ChaCha20 session key.
     *
     *  @param[in] size The size of the key (default: CHACHA_KEY_SIZE)
     *  @return A vector representing a session key
     *
     **/
    inline std::vector<uint8_t> CreateTestChaChaKey(size_t size = Constants::CHACHA_KEY_SIZE)
    {
        if(size == 32)
            return LLC::GetRand256().GetBytes();
        
        /* For non-standard sizes, build from multiple random values */
        std::vector<uint8_t> vKey;
        while(vKey.size() < size)
        {
            std::vector<uint8_t> vChunk = LLC::GetRand256().GetBytes();
            size_t remaining = size - vKey.size();
            size_t toAdd = (remaining < vChunk.size()) ? remaining : vChunk.size();
            vKey.insert(vKey.end(), vChunk.begin(), vChunk.begin() + toAdd);
        }
        return vKey;
    }
    
    /** CreateBasicMiningContext
     *
     *  Creates a basic mining context with default values for testing.
     *
     *  @return A default MiningContext
     *
     **/
    inline LLP::MiningContext CreateBasicMiningContext()
    {
        return LLP::MiningContext();
    }
    
    /** CreateAuthenticatedContext
     *
     *  Creates an authenticated mining context for testing.
     *
     *  @param[in] genesis Optional genesis hash
     *  @return An authenticated MiningContext
     *
     **/
    inline LLP::MiningContext CreateAuthenticatedContext(const uint256_t& genesis = uint256_t(0))
    {
        uint256_t testGenesis = (genesis == uint256_t(0)) ? CreateTestGenesis() : genesis;
        uint256_t keyId = CreateRandomHash();
        
        return LLP::MiningContext()
            .WithAuth(true)
            .WithGenesis(testGenesis)
            .WithKeyId(keyId)
            .WithTimestamp(runtime::unifiedtimestamp())
            .WithSessionStart(runtime::unifiedtimestamp())
            .WithSessionTimeout(Constants::DEFAULT_SESSION_TIMEOUT);
    }
    
    /** CreateRewardBoundContext
     *
     *  Creates a context with both authentication and reward address bound.
     *
     *  @param[in] authGenesis The genesis hash for authentication
     *  @param[in] rewardAddress The reward payout address
     *  @return A fully initialized MiningContext
     *
     **/
    inline LLP::MiningContext CreateRewardBoundContext(
        const uint256_t& authGenesis = uint256_t(0),
        const uint256_t& rewardAddress = uint256_t(0))
    {
        uint256_t testAuthGenesis = (authGenesis == uint256_t(0)) ? CreateTestGenesis(Constants::GENESIS_1) : authGenesis;
        uint256_t testRewardAddr = (rewardAddress == uint256_t(0)) ? CreateTestGenesis(Constants::GENESIS_2) : rewardAddress;
        
        return CreateAuthenticatedContext(testAuthGenesis)
            .WithRewardAddress(testRewardAddr);
    }
    
    /** CreateFullMiningContext
     *
     *  Creates a fully configured mining context ready for block generation.
     *
     *  @param[in] nChannel The mining channel (1=Prime, 2=Hash)
     *  @return A complete MiningContext
     *
     **/
    inline LLP::MiningContext CreateFullMiningContext(uint32_t nChannel = 2)
    {
        uint256_t authGenesis = CreateTestGenesis(Constants::GENESIS_1);
        uint256_t rewardAddr = CreateTestGenesis(Constants::GENESIS_2);  // Must be TritiumGenesis

        return CreateRewardBoundContext(authGenesis, rewardAddr)
            .WithChannel(nChannel)
            .WithHeight(100000)
            .WithSession(12345)
            .WithChaChaKey(CreateTestChaChaKey());
    }
    
    /** MiningContextBuilder
     *
     *  Fluent builder for creating complex test mining contexts.
     *  Provides a more expressive way to build test scenarios.
     *
     **/
    class MiningContextBuilder
    {
    private:
        LLP::MiningContext context;
        
    public:
        MiningContextBuilder() : context() {}
        
        MiningContextBuilder& WithRandomGenesis()
        {
            context = context.WithGenesis(CreateRandomHash());
            return *this;
        }
        
        MiningContextBuilder& WithRandomReward()
        {
            context = context.WithRewardAddress(CreateRandomHash());
            return *this;
        }
        
        MiningContextBuilder& WithRandomKeyId()
        {
            context = context.WithKeyId(CreateRandomHash());
            return *this;
        }
        
        MiningContextBuilder& WithAuthenticated()
        {
            context = context.WithAuth(true);
            return *this;
        }
        
        MiningContextBuilder& WithChannel(uint32_t nChannel)
        {
            context = context.WithChannel(nChannel);
            return *this;
        }
        
        MiningContextBuilder& WithHeight(uint32_t nHeight)
        {
            context = context.WithHeight(nHeight);
            return *this;
        }
        
        MiningContextBuilder& WithCurrentTimestamp()
        {
            context = context.WithTimestamp(runtime::unifiedtimestamp());
            return *this;
        }
        
        MiningContextBuilder& WithEncryption()
        {
            context = context
                .WithChaChaKey(CreateTestChaChaKey());
            return *this;
        }
        
        MiningContextBuilder& AsFullyConfigured()
        {
            return WithRandomGenesis()
                .WithRandomReward()
                .WithRandomKeyId()
                .WithAuthenticated()
                .WithChannel(2)
                .WithHeight(100000)
                .WithCurrentTimestamp()
                .WithEncryption();
        }
        
        LLP::MiningContext Build()
        {
            return context;
        }
    };
    
} // namespace TestFixtures
