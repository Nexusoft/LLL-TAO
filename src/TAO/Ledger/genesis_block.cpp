/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/types/bignum.h>

#include <TAO/Ledger/include/genesis_block.h>
#include <TAO/Ledger/include/constants.h>
#include <TAO/Ledger/types/block.h>

#include <Legacy/types/legacy.h>


/* Global TAO namespace. */
namespace TAO
{

    /* Ledger Layer namespace. */
    namespace Ledger
    {

        /* Creates the legacy genesis block. */
        const BlockState LegacyGenesis()
        {
            /* Build the first transaction for genesis. */
            const char* pszTimestamp = "Silver Doctors [2-19-2014] BANKER CLEAN-UP: WE ARE AT THE PRECIPICE OF SOMETHING BIG";

            /* Main coinbase genesis. */
            Legacy::Transaction genesis;
            genesis.nVersion = 1;
            genesis.nTime = 1409456199;
            genesis.vin.resize(1);
            genesis.vout.resize(1);
            genesis.vin[0].scriptSig = Legacy::Script() << std::vector<uint8_t>((const uint8_t*)pszTimestamp,
                (const uint8_t*)pszTimestamp + strlen(pszTimestamp));
            genesis.vout[0].SetEmpty();

            /* Build the hashes to calculate the merkle root. */
            std::vector<uint512_t> vHashes;
            vHashes.push_back(genesis.GetHash());

            /* Create the genesis block. */
            BlockState block;
            block.hashPrevBlock = 0;
            block.hashMerkleRoot = block.BuildMerkleTree(vHashes);
            block.nVersion = 1;
            block.nHeight  = 0;
            block.nChannel = 2;
            block.nTime    = 1409456199;
            block.nBits    = LLC::CBigNum(bnProofOfWorkLimit[2]).GetCompact();
            block.nNonce   = config::fTestNet.load() ? 122999499 : 2196828850;
            block.nChannelHeight = 1;
            block.hashCheckpoint = block.GetHash();

            /* Ensure the hard coded merkle root is the same calculated merkle root. */
            assert(block.hashMerkleRoot == uint512_t("0x8a971e1cec5455809241a3f345618a32dc8cb3583e03de27e6fe1bb4dfa210c413b7e6e15f233e938674a309df5a49db362feedbf96f93fb1c6bfeaa93bd1986"));

            /* Ensure the time of transaction is the same time as the block time. */
            assert(genesis.nTime == block.nTime);

            return block;
        }



        /* Creates the tritium genesis block which is the first Tritium block mined. */
        const BlockState TritiumGenesis()
        {
            /* The block header. */
            BlockState block;
            block.nVersion = 7;
            block.hashPrevBlock = uint1024_t("0xeacd7b76a83947c61c53706e10e487649bae127bc6afd1b51dc279a3e155658aa878c25117d660ce3b3c13b709f081da8795cd8848524dc6c510c7e77ab6e5db3f11ae617224e6da2a8ffd82efd483f1ccb564e883ca008c0caa57918456271b7aa40880b17564993a10ad4a4757a3692bddbb9fd712b46234919aafbd86382d");
            block.nHeight = 2944207;
            block.nChannel = 1;
            block.nTime = 1573566343;
            block.nBits = 82276795;
            block.nNonce = 3544415792015049862;

            /* Hashes for building merkle root. */
            std::vector<uint512_t> vHashes;

            /* Hardcoded VALUE for INDEX 0. */
            vHashes.push_back(uint512_t("0x01e2793b8c193e8beb7fb476087cbf3ccaf05aa2c3e5eef8c62fec6e06b04eccae74f14d9d057bbafb828cc0a952e01fea07ab5c5bd68725fb566db2f4559712"));

            /* Hardcoded VALUE for INDEX 1. */
            vHashes.push_back(uint512_t("0x01129452a22088e3a2aae1fdfba1bbc8d92f7397f61c349e694a9c2518eb45dbc4db9b10c438080ec84f6ce3095786c221807b03fe602c6a525571400117f663"));

            /* Hardcoded VALUE for INDEX 2. */
            vHashes.push_back(uint512_t("0x016f62ee4e9ef16159d3c9272bfec0c9cdc7db546b548a02f77773701a27e943ee129fbbbca7e981a362a4e55c75b87bf99a4bf599c88c195cc54eb7851bd291"));

            /* Hardcoded VALUE for INDEX 3. */
            vHashes.push_back(uint512_t("0x019a3516feca6727c3ced1101c8ab3bf3a9bb98274b7f892a44fe7097e4306cf9b06fceadb831135db47df6c164a951878f1b20728a01e3b2c56b5bf04d40297"));

            /* Hardcoded VALUE for INDEX 4. */
            vHashes.push_back(uint512_t("0x015a9c4b8c65304ac1285dc67d7fc241434eab6155b73b078ba6f251a13ba121cfb5a2084baff7c55207794e6349b40c0bfcb2286cc71578cbdd579315cddef5"));

            /* Hardcoded VALUE for INDEX 5. */
            vHashes.push_back(uint512_t("0x01fcba899209442a3d7db4d10e07019e77687159fe6c0755991abcb23b1bb0b51063139411dfbdbfe53020ad16fa17ac777d3ccc92402c366f3288ff0e720ed8"));

            /* Hardcoded VALUE for INDEX 6. */
            vHashes.push_back(uint512_t("0x0137d80eba4044f23659bef142e8f6a1dae8124dcf81b156f5c48e4b1c96e7567ff0efa429e263f2d1d8ff066022784d9ace68f0bb674ed9773691900529b969"));

            /* Hardcoded VALUE for INDEX 7. */
            vHashes.push_back(uint512_t("0x0199e2bd5c335a243c8e1b3e0d5be4a97f5ddd8b998a9adb367c5e179999facea6082a0b282b86e556c96f1997a2458b423080b7b67dc0506aaa4f719fdc9a22"));

            /* Hardcoded VALUE for INDEX 8. */
            vHashes.push_back(uint512_t("0x011e6d5409588c16b6412c5225425969b654c3907f7a40ce45ad3e156f862e439fb7e9dd6a29653d59228cc13bcebe3d9e4db365ea8eba436299be553afb4cda"));

            /* Hardcoded VALUE for INDEX 9. */
            vHashes.push_back(uint512_t("0x0130928ae50c8daca06b8534182ce9e47b1e56126d799e75a520a6397af5e200bb36b9a93aedf343ce689d2c775b4b26018995d9b92f38427fd524255183d2cc"));

            /* Hardcoded VALUE for INDEX 10. */
            vHashes.push_back(uint512_t("0x012e3f0f36759416925bab8f2190179664e1f4549921480914c55089757d97d5b1eadab1675668744af3563541fa4a0149aace9c0b29efe57a7eec261755f3e8"));

            /* Hardcoded VALUE for INDEX 11. */
            vHashes.push_back(uint512_t("0x01de1e769fb24a2ea487911d01eb49f0471345a144545b282def9234f234b812b2659b3e34978abde2b817e57c51b1c12c09ca86d96294a6b32b6afe613b902b"));

            /* Hardcoded VALUE for INDEX 12. */
            vHashes.push_back(uint512_t("0x01b68f802d640199944984fa51b448dc5fba2317d99a5eb9a65bbbb214434a217385b1cc2769e98a26463b19482a2ce956646dfe52824a90c1ed8642b675a93f"));

            /* Hardcoded VALUE for INDEX 13. */
            vHashes.push_back(uint512_t("0x01548b2768d831c8fb1378f14b941ff8c545687df74f4ab3882594e81441f8bea57d89be186d842c1866d3ad8d8920fa2c4fa76be87f9b654564886156044a58"));

            /* Hardcoded VALUE for INDEX 14. */
            vHashes.push_back(uint512_t("0x019a5ef5de78b143db0639638d6e05bf0540625144039dd6f7342383d95b52bb529e1d96339e5b603fe6d9d5a0343fe42cbe6e5913aa50257e7151cf39bb060b"));

            /* Hardcoded VALUE for INDEX 15. */
            vHashes.push_back(uint512_t("0x01230c01f63dfe0888f6eea76776622c0043e3df7457d0dd2e3f592d3fdfafadcde371ee6e3be2fb5449fca34a3b4bb77041e459ff7713a2f2513ed44c4f3cb3"));

            /* Hardcoded VALUE for INDEX 16. */
            vHashes.push_back(uint512_t("0x013876384c6b0bb2e754db31b45072f8eb0cc4b4ae33b116cbc5fd82e7f6b1870d75a28cdbee01019c142e8c3ec9ab9b4702ec4cfa5666be748406490975d1f9"));

            /* Hardcoded VALUE for INDEX 17. */
            vHashes.push_back(uint512_t("0x01df552278e3fa59b8cefb8a2701e81774fb327b65920373e86ead93fa9d601a854232d0beb73febf8ac631a7248eb9ff96da99289b0880a0eb1e127256a41f4"));

            /* Hardcoded VALUE for INDEX 18. */
            vHashes.push_back(uint512_t("0x014127f1ea5a8590a9b65240dc31d8df9b8e2d2647bafec3a4084a1951484d816b10df9bae8f7bb02803c8d588aac5603d44c8fcb2bcadcf1943bd23a4fe03b2"));

            /* Hardcoded VALUE for INDEX 19. */
            vHashes.push_back(uint512_t("0x017a51ddc9e11c073c21b49ab1782705df2f6091106be6390fe6018f6b92aa6eea951366030d935558377d59c247f9cdaba5e5983f4d300343c952080bef42b7"));

            /* Hardcoded VALUE for INDEX 20. */
            vHashes.push_back(uint512_t("0x01a5e561735aaa293ca53595078db79b40eff32d9652b07efdfc9e3b574afaef088cc2c60740b2bcf60e3f4ef5bb80adde3ef39f23b743a7d8f02da72431935e"));

            /* Hardcoded VALUE for INDEX 21. */
            vHashes.push_back(uint512_t("0x013a0a855da1b4583b9a3aeb8972d1fba9a63cae139ef952d4cfc825da91831659f832274053a7d07385f1419a75a2b55f4b2402ae1d9aa17ce3cbf8d3380188"));

            /* Hardcoded VALUE for INDEX 22. */
            vHashes.push_back(uint512_t("0x01b255af3311367236fb94141c0ee18e68d908af88b971568dab25a584b9669d5c85616893b24271d5bf5aa4441e87950c3b0af9eb2e6776eeb2b3e9d7f5774a"));

            /* Hardcoded VALUE for INDEX 23. */
            vHashes.push_back(uint512_t("0x0144fa1549b5dc6b75355859fd640a5872d0ffb10a7d0dd969080edf55221090353c5b73b19be86891c8511481d1e9e72eeedbb7bfcde7854a1e8ca27ce9f27c"));

            /* Hardcoded VALUE for INDEX 24. */
            vHashes.push_back(uint512_t("0x010444a546c957454898bb31e5d4fd268faa379b3cc37b7fe962b5529180504d1558fcfe9d1717d99722b9ecd1146314b57c2a4db24819d56a6f0064979f519f"));

            /* Hardcoded VALUE for INDEX 25. */
            vHashes.push_back(uint512_t("0x01851d42a6882e28217dfd41d92ae85c858b0b8055e9009cd3c2993a73aea0104679446a96fdecca8d0a80dbad6e08d69055916dd3bc905f3dfbb7ff3275c68c"));

            /* Hardcoded VALUE for INDEX 26. */
            vHashes.push_back(uint512_t("0x0163d0e378694adb1c77cd8a5b2df28a2bacb244b5df5a599c70c621b116a7abdcb9e441f4ece671e83dde7908e97e40fcb2e8c8b53baf095f613fd3ec81b2db"));

            /* Hardcoded VALUE for INDEX 27. */
            vHashes.push_back(uint512_t("0x01bff19cbe87392290754eafe5c064264f6c299dbbf70c18d392287127dd25d4588b125061f96e2153cd86d2894d6c5dba7a3ded49cef10104eb367581fa8704"));

            /* Hardcoded VALUE for INDEX 28. */
            vHashes.push_back(uint512_t("0x01502bbcc35b7fa9c11a39e3e4e1038ea14bb06411c42a5d5fb3d60deaa55800e3513021e1d86c60f408af9c817cff7cdc744933039bf3bf9e2a44bbedfa1350"));

            /* Hardcoded VALUE for INDEX 29. */
            vHashes.push_back(uint512_t("0x0175ff17c216451ed206896f45529114a0476504f12d292ce02430c8431b505e20cf6e17c058cc7f835d27b0f9dc065576260b810d3406b572b9ca1bf53b76dd"));

            /* Hardcoded VALUE for INDEX 30. */
            vHashes.push_back(uint512_t("0x0122215c9787822612f194de196e1a5802e220cee0240a26c8464e1ed30ab3e1cebec82dc176247735a21fd1d7a421925cb36bcc3e1cfb2c638e2b6876e34176"));

            /* Hardcoded VALUE for INDEX 31. */
            vHashes.push_back(uint512_t("0x01dfe1d8c0bc3fb85a700a232da1dd0071ee6bc3c11b811a8f48ca32a77dd02966a8359140d4debf54908f8329f5a59f35eea614a27dda9b5d1135c433096596"));

            /* Hardcoded VALUE for INDEX 32. */
            vHashes.push_back(uint512_t("0x01f9f430f1dcc942eea4d276a22ea383ca286c8432a29ea64b7c78168b467976cfb18da148e64e8ca7fb1ba05763392b49f80fe3dc48336d86c47dd32a4b3713"));

            /* Hardcoded VALUE for INDEX 33. */
            vHashes.push_back(uint512_t("0x018f77845a2e270c3355d65ecd5ec91384d188dde3a877aa5672fde650477ca7ba2b8483ff32fced92fb3cf223c7e645240edc14923a71e16eb6a531cee7da10"));

            /* Hardcoded VALUE for INDEX 34. */
            vHashes.push_back(uint512_t("0x01055eab418ee1c1573642a290ba182a4b2be47d4166ebbe4ab902c612c78a6118739bd9eb7a39b60aea9941835a2ed70a1c88a9a84c9d8c36a4ec6d225e0228"));

            /* Hardcoded VALUE for INDEX 35. */
            vHashes.push_back(uint512_t("0x01411da65356d4807a7b1a84fa5f91ae7a7a436567bb8c7ca87a04e2132ecd7bd38443df6b0ed7e9d405c13609d40d2e378759118cfc15abdd0c84f5d6d4d16d"));

            /* Hardcoded VALUE for INDEX 36. */
            vHashes.push_back(uint512_t("0x012d5f4f1bf4c4d3184542c2125df8b96fbe45485968a8286a2510ec7088b6f470839878dbd88647f5efd9abd94cd2a3500a57e7248c2215269cc4d3cb49ad16"));

            /* Hardcoded VALUE for INDEX 37. */
            vHashes.push_back(uint512_t("0x01a4256a8e884e2ef2e14cad0268c2a557dc9f98d792a907737b9a3b17f2cdf7e365b94b471d02bd84fb357b306652f2366fa48d7fdb4b36435f478cbc1a5dbd"));

            /* Hardcoded VALUE for INDEX 38. */
            vHashes.push_back(uint512_t("0x018ff4093a93e006cbe69aef0f9d135e3b055c577391ff214bd29a5ac3424ac96753165a961b2908a31c2e79a6bb6128e7456f567fb2811b1ee4feee8d332d0c"));

            /* Hardcoded VALUE for INDEX 39. */
            vHashes.push_back(uint512_t("0x0136a86c9ce1981d8f91f57996b7eb2ae047a7c2cebdfd443cc7e120a14fabaf5cb88c69020884d523e0877a1a4807b50aff8d45c19d257b88946a9d43485819"));

            /* Hardcoded VALUE for INDEX 40. */
            vHashes.push_back(uint512_t("0x018b76433f51f55ba86d2fc2e1b6c223bccc1c30d5fb0bb9b00d062066bdd2e0fb366aadf7cc43267c77ced74353cff6d97d4ae6ad5d3fc6147f2409d5c965b1"));

            /* Hardcoded VALUE for INDEX 41. */
            vHashes.push_back(uint512_t("0x01fc3d82e2129d66357e3a7f1c1ca249aa413d56a5fcfe7f774f079b5034cc0a849361973a52e055e0f7b4e3cd0469b9ef8f31a7cf7fb54577135dd0eac7f4ad"));

            /* Hardcoded VALUE for INDEX 42. */
            vHashes.push_back(uint512_t("0x016caec9ee13add0d3ae93b128751bcb39a3910c2cfee1fd2272cdec63487f3721c22e1984bcfa7db030c82de591ed68a24bb4f5208103d12b7c201515db1c8e"));

            /* Hardcoded VALUE for INDEX 43. */
            vHashes.push_back(uint512_t("0x01d72e9e9ccf6369c5a3a2985446d32e1b9480799a426fc174e2de8e2d172b2799acc63ebea012c7b48656296eccffa03ae8dfe25eea9d7d567396ee7f87ad29"));

            /* Hardcoded VALUE for INDEX 44. */
            vHashes.push_back(uint512_t("0x01ad773ca9f7be6220d70b229315348b205e758544c86a769aa1106f5b549d9c854a5cffee7008874cb0dce554700927a41592d6c434f927eec3fdd94d5cce4b"));

            /* Hardcoded VALUE for INDEX 45. */
            vHashes.push_back(uint512_t("0x013f6f41fe434bcb159ed976f132b28bd5aec559cf873a241bea6d3cf16320ee86f61c14fc9bd879ff55f4c1b9516fa0d054ff03da75ce90099750fd350b78f7"));

            /* Hardcoded VALUE for INDEX 46. */
            vHashes.push_back(uint512_t("0x01f97f3cc4d4ec5a55b37c029973aad2143d9f55935434b80d5cba4ad6e0b3e7e6964a7ff92e8eaaaa37a2b2274a08eb072477c6b8c84af0af47c83f83268f5d"));

            /* Hardcoded VALUE for INDEX 47. */
            vHashes.push_back(uint512_t("0x01389d55e9958e5bd4449443af88752504d86a5b9a9e64a0c281570cd3e04341964ef730f16721b517d99c4a7c2f0d5e7585263bdddf49b530410feab9b59604"));

            /* Hardcoded VALUE for INDEX 48. */
            vHashes.push_back(uint512_t("0x010c8dca60fa1009932f93001ef39ebf01aca732e3c9c260da9db80b4a9be7d41229e362cf7f0c1b9d4ebe6649ea8c06a4cbe10a0a0df111e013d3ac074e85e1"));

            /* Hardcoded VALUE for INDEX 49. */
            vHashes.push_back(uint512_t("0x0143d3dd2254793a63abc25ab4ce3d0f63cb00dd0a9225c5728d1a802a534962d2e4d71a8da7c619a57cbc1a1d5e8165d1dee06254bba050cd87f3e5b1be8b00"));

            /* Hardcoded VALUE for INDEX 50. */
            vHashes.push_back(uint512_t("0x0135868d51c768ac7b16b96a4fbc595eba22fb7db096f241232d15aadf3d2a77925a36c7217870e0abab4449d9828b026b458c6b7f1ed15b43b21d8d60cd257a"));

            /* Hardcoded VALUE for INDEX 51. */
            vHashes.push_back(uint512_t("0x01748bed17a352a2d14b5072591e8b4761d732fc3c9081f7736976e73cd9811e647404de29afb31b96b30d3f1b8ebc80661789f4f07763fcd13304490ca92316"));

            /* Hardcoded VALUE for INDEX 52. */
            vHashes.push_back(uint512_t("0x017b5a619da6af0fffc27f7a1fb6fdbfe1d3886961610118c11359dba908ac48baf5936c5eebfa62a1ee6bdcba199d0e8135de90e9aacdde3ae836faf02f0854"));

            /* Hardcoded VALUE for INDEX 53. */
            vHashes.push_back(uint512_t("0x01c1d1437b31fe72c2590a4bd12d27a1bae97b85cb33e2bb724c0ddfc437cfa6f0d30389812656fa52c7f74179aef4dee0b5f005a9f5529753d7582370c93fa7"));

            /* Hardcoded VALUE for INDEX 54. */
            vHashes.push_back(uint512_t("0x01b5b1780eff29c5e78973af4e8c7f53d3df3a8764583423d9df148eebc90d72f4976ed063eec4610a62bd37b0418c3ffe97a21d6dd8180459a1db8b6eb1faa2"));

            /* Hardcoded VALUE for INDEX 55. */
            vHashes.push_back(uint512_t("0x01a64e0f93b56eecb8bdc281d09015ada7b7ead015727e6661c05c55ffedf57121abae8c06ccf9860aa33fb80cf560f96539b95a27296d10a4c7c5a1e08a265a"));

            /* Hardcoded VALUE for INDEX 56. */
            vHashes.push_back(uint512_t("0x01a33cf2b00b7e8041524ac89820b64a7bb226521d9f534e561a4862275ea40ff150c53251147604cc23b3577e21d74715c9fecf0e0723c35400411a2fecbb71"));

            /* Hardcoded VALUE for INDEX 57. */
            vHashes.push_back(uint512_t("0x016d336d163846514ceac64ea85d28fc13016f54b890d3e133cd74639ccddb35864c96c58ba61f477dfe573598d1fa07d8d61a912ce9a5c588ab55860feeb211"));

            /* Hardcoded VALUE for INDEX 58. */
            vHashes.push_back(uint512_t("0x01783441aa02737b778c68d3492aafc3974ae2477011ef1d22c64303f3a176ada1a732a2b374c64b8d19769528704bb03166b2085d9f9da7db271b473aeb2856"));

            /* Hardcoded VALUE for INDEX 59. */
            vHashes.push_back(uint512_t("0x01ae4f3b95a0a02bd9015e917850e0bbdc1d3115b932217ed6566e044d76e5b3f23383b3e84ec2f472a037d96579d0364c5d0b2c5a8f9df21942aeeeaa87fb53"));

            /* Hardcoded VALUE for INDEX 60. */
            vHashes.push_back(uint512_t("0x018d0f2b87d47fd12502329e08191378ac89f72387214eb07dd1605ebe9ca1ef3d9160cc2e1e30ab3e3a483accc3b137853d2fa22017fc3e70f88f28b43ee3e7"));

            /* Hardcoded VALUE for INDEX 61. */
            vHashes.push_back(uint512_t("0x0141480329d02ee94afbec2823b6f85e06959c14b4d305e1dc2d5ad96cf064051fc003fb4df08bb3f6f687cddadda3af0204749a4b3382ef831dee7cc911afe5"));

            /* Hardcoded VALUE for INDEX 62. */
            vHashes.push_back(uint512_t("0x011905aa6d3bc064c5d09ca1c35c4dbb5d0fd35f161f3d3ade72c600366eb5f6d7d18853b5bd2d4bbd7ea4503fa25e7d8a4ba013fb6d969f011e5a80604d399f"));

            /* Hardcoded VALUE for INDEX 63. */
            vHashes.push_back(uint512_t("0x0111c72a7947b64159f32e3f8e870f2bee190bc075ff0bc16cba83069443f992284930a15938ee16640ce014d554778e76ea308de51c0c7700de140848a69556"));

            /* Hardcoded VALUE for INDEX 64. */
            vHashes.push_back(uint512_t("0x01f63ce9eb2ada6fc1cf1aad2d7d2a835045ca7fa3d2f920c574eee25a7645c0a86ad1d3d88755d54a953b3e971b0df1e513002aaf3d8f5d84f4b0c0ee014673"));

            /* Hardcoded VALUE for INDEX 65. */
            vHashes.push_back(uint512_t("0x015b3d25d82db7b1a82bbfaf86b2f51dff7698a2d03b734bba41b3feab3e37a4df06592d947de59cb9c4f9564f9be8ad68135efe99789e8b5130551fe99cec61"));

            /* Hardcoded VALUE for INDEX 66. */
            vHashes.push_back(uint512_t("0x015c18583bfff88da06c1f0247ff62796cb4fc5f51b5ca8d3e568853b13137047c27118c5c446a99dceb11af329471243db5d713218dc7efa6ea0760780c23d2"));

            /* Hardcoded VALUE for INDEX 67. */
            vHashes.push_back(uint512_t("0x01dd1712050e76e6af1ae1de71f702593ef117b8552b8b7776915181ada55af2008791a85ee4f4c2f21507188bd7ce58e43c38b65bee0b776597dcdb7f35b396"));

            /* Hardcoded VALUE for INDEX 68. */
            vHashes.push_back(uint512_t("0x01f807d57a6e12a0187b4fa061c931f462da05938df80a1a0115f6ab2873f1f405d41e8cb615ffb201df25d9cdbd393c5c44ecf3ba309587f3ad9754f37193af"));

            /* Hardcoded VALUE for INDEX 69. */
            vHashes.push_back(uint512_t("0x01422e52895bafa15bd29151a822b155d5c8217c778594668598b1d3c0e82e95ff8864607b9d707d9edb73a83bce2bf34b4ae598a43c403f48fbc2c90b03770c"));

            /* Hardcoded VALUE for INDEX 70. */
            vHashes.push_back(uint512_t("0x013795f69a216e3e1cad8013fb5ffdd6aa7fccaa87dcfc3cd5b52e4ad63a2d5c79e608a3a01c74e54fa237a0c3572da3aeddd108cfb6ec55552ecf6c5a1635a0"));

            /* Hardcoded VALUE for INDEX 71. */
            vHashes.push_back(uint512_t("0x019393e03c321551392c30be3059b1bc9edc67b6361a5ba585043eb2edb09f558423184628e84d7959fff3e4ed74ff66bcda32f75ae2a6fd60c4801476ec30b9"));

            /* Hardcoded VALUE for INDEX 72. */
            vHashes.push_back(uint512_t("0x01f2563735dffc48f69e999f259d99517381f64258870ae7b491e6ab4f3b9c15c34456887093818b3ec6c813058bc8d45824f08a5c3b6ecaaa6512d1b4385a2b"));

            /* Hardcoded VALUE for INDEX 73. */
            vHashes.push_back(uint512_t("0x011fff078159ce160bca0be588fe5c141b6cd76c42a5ab5a2be0a24819f5bca7d9d888966696ca4ae75d523a9e53e972764c093fa860b8d084897266a0ef8f6e"));

            /* Hardcoded VALUE for INDEX 74. */
            vHashes.push_back(uint512_t("0x011389ade99d347c4096829d5ea1ba7b92bfb9cffa20f58e803b46c29ce48bd2a79830b2f3b7c82c6adac147360edaeb30cf2d21cf0ad3f1e2d2b6a98bc41351"));

            /* Hardcoded VALUE for INDEX 75. */
            vHashes.push_back(uint512_t("0x017ef865594ca130d0a5b3a509b2c0768c03eb8ccf0dfa91a140c57d33ce7b277d6051ac142986d38b0f4a77117f002fcff973025d9333358e9aedb164a7408b"));

            /* Hardcoded VALUE for INDEX 76. */
            vHashes.push_back(uint512_t("0x01295aadc7655d45259dede16bafaf21dc14e42d5918a0e5d95e8ea78e4fba5ca383b7a60694240a9dc5a6e33ab3267464ba6bc21a1af68135fa1fc184c6f764"));

            /* Hardcoded VALUE for INDEX 77. */
            vHashes.push_back(uint512_t("0x019a8ee70e768cb1741aaf3e1307cecd248e726c469a160f200068058c5840a69e54d05242cf30a0a9ac0f40a100e5ab710bf81a713712fac2d0ee38d10a1c7b"));

            /* Hardcoded VALUE for INDEX 78. */
            vHashes.push_back(uint512_t("0x0100f55a3b50e2a3dd4afde26f9287008e79d441706a6b5ca0a1a7e6ff55ed174dbc8cb5bf1bf57a5a8c4bde6247f2c40ef46ab28a55f39c853bc7f79684edb0"));

            /* Hardcoded VALUE for INDEX 79. */
            vHashes.push_back(uint512_t("0x0160a6dfe5e4dd6e514ff56efefaef97f0950f26accad642a04cd8086edb67db080d4eac78e0f7d205101db30cf16957c451ae4ae622efa40116c5ee0d2d0249"));

            /* Hardcoded VALUE for INDEX 80. */
            vHashes.push_back(uint512_t("0x01c2ded50e65288632be9dabacb5740813a3a7f54a310444f73776da5eabcfdef90586c417a0e78006f4f050f6e469b3383cce69e9475a0fcaacf9462c7782f7"));

            /* Hardcoded VALUE for INDEX 81. */
            vHashes.push_back(uint512_t("0x0181e000565309489106e7b482b81589c1f64be50e3a66043987aeaeee6cb8c6b781bbbc746ca3d11f2c9fd72bd66042922e31a3be9896c6f7663ab52e513c0c"));

            /* Hardcoded VALUE for INDEX 82. */
            vHashes.push_back(uint512_t("0x01932edda9b8259bc28ed823ef30e554209338f3dba08847e6bd10afcf6057be78de781d26831f6678552d1c6d7f0e801cc6ed70e7822e26be4dd5eaf74e9b69"));

            /* Hardcoded VALUE for INDEX 83. */
            vHashes.push_back(uint512_t("0x01987a42e364c397ef5e99e195f113b6e69224d5b054c7f532a867070263d13e1d037172178017ddf069871a9c59c43db2950c1bc061187624cac9163c2ac811"));

            /* Hardcoded VALUE for INDEX 84. */
            vHashes.push_back(uint512_t("0x011c275f9310bc6639cfe9ac34821d2bde07559cf96da28f889d98a79c16f7d401fe2f1e09627a33c572d9b6d2b2c3a77d25acf4af0710fe1c88014dfa7e0492"));

            /* Hardcoded VALUE for INDEX 85. */
            vHashes.push_back(uint512_t("0x014865b332696253df144752a1ebec4b0d7b57541540a707da09ce2177f8d7dd5b45851d83498f2a059b00f64edd93dab9c760f8fc614e3d9299e738c0b37417"));

            /* Hardcoded VALUE for INDEX 86. */
            vHashes.push_back(uint512_t("0x017b6e0c3a3d34e4d6a761723adeda240ab7fecc6c8c30ef2b7c483fea6b756416247cfa0f5f8770f0be1eb71be9c53ed78a3e64a2ea0ef356a6bc3d20196d15"));

            /* Hardcoded VALUE for INDEX 87. */
            vHashes.push_back(uint512_t("0x01849f7711f2c1981f29499e3a5a2fe3602e5c1b3a8773c53b76a9ddcd2a3668ab7ede03704d5545b565fb62187c32a6132b2a5dc9e473dc1dfb18be3b4181a6"));

            /* Hardcoded VALUE for INDEX 88. */
            vHashes.push_back(uint512_t("0x01f2e0be59cfcf800c0bb7b64ac24134d7e29612dc9af2e1ce885740f73179f3cd5c3f0252261ff2abd3b507e8102610d30416941e8dcea31b1435d29fcd5b2d"));

            /* Hardcoded VALUE for INDEX 89. */
            vHashes.push_back(uint512_t("0x01bf8544b348c72ac5dc4f250d22aafbe373b91b4f60fd2810f62e06da6b8ce54a1770f5f712ab499434725052f7a333c48a7ac6ab54df52a1bd59a9c5147065"));

            /* Hardcoded VALUE for INDEX 90. */
            vHashes.push_back(uint512_t("0x01d32845ede2e4e48404e9b1924e487deb1926f4e6b34af8a114aa3a979be8eb0304e15aa6f96a5ca20702072b16226b93c7eda75580dd3489559a84bc71f7b4"));

            /* Hardcoded VALUE for INDEX 91. */
            vHashes.push_back(uint512_t("0x016198f19ea7b9a639a8a7fddddea0fb4d22332f85bcb4cf743735559014b57e59bad8426118fd8957867e6b6ceacf66cc47aab72882d34408e555e3911ad93c"));

            /* Hardcoded VALUE for INDEX 92. */
            vHashes.push_back(uint512_t("0x012277fb9063c405d78a9acebbf4626b59700439cbd1d8bcfddb29445d2d7d088342821499b6124008ddd1d2a9a084781a7705cfa298d52543a9d5181bbe2c6d"));

            /* Hardcoded VALUE for INDEX 93. */
            vHashes.push_back(uint512_t("0x0131f202d3a0203a3b64b19cffc30c264feeb3ebd3c8f1b9791dbab3c3a1f297eee0a4c3a9be7118d6e5396bb24d059f891dc21f82e039caa2c021a072873dd7"));

            /* Hardcoded VALUE for INDEX 94. */
            vHashes.push_back(uint512_t("0x01f466dc576b58cfc709bf51fa745d5fa42e0d5f33b7db9f3865efe66d7d15988a03148001ef0ced923f071f343122b3484a1eaf8f889f8ff90b3a5ca3dd0e63"));

            /* Hardcoded VALUE for INDEX 95. */
            vHashes.push_back(uint512_t("0x01f6ca811f13b312a032c7e4c3deac9e30990897e7ca71287c4209041f9f02109e769a10fb43775d76e2bd6b08eaa699d5a1bb721d199224abb6ede0963b8e4d"));

            /* Hardcoded VALUE for INDEX 96. */
            vHashes.push_back(uint512_t("0x0116f8b6c8ce900c67a5472f1c3aae77ab29277bc5fffb4738b0dc8a0f02312a7496f40f0b478d5d21a6ada9de3881992db4309e159ea3984df083503a8e642c"));

            /* Hardcoded VALUE for INDEX 97. */
            vHashes.push_back(uint512_t("0x01a322e4697503cefdb96aaed38aaec2588576d1c9df78b311341363e40b75fad604be1b902ae1a6eeec12bcd9d40b3e3fcbeb17dfd5d5e13a1730a4ab448d63"));

            /* Hardcoded VALUE for INDEX 98. */
            vHashes.push_back(uint512_t("0x010b38e1b54488b0e878174bc135c40079bd88041d78b4eb12a862887c9d1962b5b35d634541d8030c91a74e790854fcbb13cfaab406ee3eb4a7dc8fdffb4e05"));

            /* Hardcoded VALUE for INDEX 99. */
            vHashes.push_back(uint512_t("0x0152e6a523686ac290c79b8a6085e4b7fb78236eff1e63dfaacca58f6d487833595542431d4f0f12b3a32c9144f831d142343bde210d9cc00ee41aec2f95891b"));

            /* Hardcoded VALUE for INDEX 100. */
            vHashes.push_back(uint512_t("0x01cd874e1bab59eb943cb986810d72485e1d74414a38a99d7bda066a27295a2b814606fcfa757b08087ac90bb8fd1c2b53da26158b025d0237ac8f7b5336977e"));

            /* Hardcoded VALUE for INDEX 101. */
            vHashes.push_back(uint512_t("0x013f06e21e746f695d728f355359d54b64314a601d0d259b77bd12c75ba217e1b92f90b4df02fc2c4c9dcc23fb4a36bb43cf96e67687fd347909cfd68d12b3c1"));

            /* Hardcoded VALUE for INDEX 102. */
            vHashes.push_back(uint512_t("0x016c40e6ff326f84f4cf1a1ca060a60b446826b07b287ead37d3eeaff2f88aa593fe8e816948f90b6d8866b1b230bfa0243ab7c4ea92a1608589c901fb04fb9f"));

            /* Hardcoded VALUE for INDEX 103. */
            vHashes.push_back(uint512_t("0x01c421493e6d761b537003a8293b8d7ddd2d07ff83aed0e81cde72ba734fb0d0ce921e9f84b6adeb2fc34e30ca4691893f2e3b006287673794983c1efb0477ed"));

            /* Hardcoded VALUE for INDEX 104. */
            vHashes.push_back(uint512_t("0x01089c5854c2922090ec67bf3cffc86234352724a18b9a94836389b92262729bd101e52fd8f9f1194079bb03587811a022a4924855847d2ffe2c3e7070de9456"));

            /* Hardcoded VALUE for INDEX 105. */
            vHashes.push_back(uint512_t("0x01301a336ec0a8b76024283442996ad822ac95dc717c7c763bbc4a524ab77c7c7a092c2713ce8e5c1ed9d6f5da40f57d00399d77f235fedde9f6945feb46c7c1"));

            /* Hardcoded VALUE for INDEX 106. */
            vHashes.push_back(uint512_t("0x0161a5aa4063f9e26af2fc124a49cee6da9ae5eac883fd5c9bf6225dd6045072360f50b01bda1e5c7f209a29426873e0d0e8d414506574736fbfe601ef26547e"));

            /* Hardcoded VALUE for INDEX 107. */
            vHashes.push_back(uint512_t("0x01293d799005bdb7845fa2d77c04f59405db866d3ae5f9243df4199383aa766513d8c6bc3896067b285d442b76179a6ea33905677fa62f1a824f2b2ddd5ac433"));

            /* Hardcoded VALUE for INDEX 108. */
            vHashes.push_back(uint512_t("0x01f11e653fdc9438ece3c639a428da1e31e09904b5e7feee031b63be7ad0331a90c03e9c7d0b643201ca1d5affd110b8330335b94b5da0f338bff15f9122f4ed"));

            /* Hardcoded VALUE for INDEX 109. */
            vHashes.push_back(uint512_t("0x011862d323df94573928bce5f8962e402f3b72c36602ee8b985dcafdcae029e1f127c26127b15b7b6bc6fe7c053d477eb80895ebe5a04f90d9c3a51645d0d877"));

            /* Hardcoded VALUE for INDEX 110. */
            vHashes.push_back(uint512_t("0x01298a771ecdda80847d1f7edc639ab61413b80d3eaf55cd0ccfce2dc1670383a2e079d7563831f82329ea0552ea1f7b721a2d66c08153b6d6099cc73424c082"));

            /* Hardcoded VALUE for INDEX 111. */
            vHashes.push_back(uint512_t("0x01fc8d165a158f3c86dcf560e6629feebdeea53dc7d6a0e370b05c8a8d321db7fbf13fd21b343a467815c930498ee59207b965902d1f0ff9c52e74a79d7d5c44"));

            /* Hardcoded VALUE for INDEX 112. */
            vHashes.push_back(uint512_t("0x01e9179720f796ce78988b43c3b86e79cd1203a853baa4427b16a1bef1110fc4f1e58f91e9ca50de9fed86fc517f24f6bc76df23d1e85f8555498d97090d1a03"));

            /* Hardcoded VALUE for INDEX 113. */
            vHashes.push_back(uint512_t("0x01581b74b0797e6f23f303d1e62d93cb438db2ccd729fff5d1f37e37e2aedbe4c76ac1efa44d45441f7b0c881b2a7df511deec123cd4a25ea0b5066a343b8378"));

            /* Hardcoded VALUE for INDEX 114. */
            vHashes.push_back(uint512_t("0x01ca7aff7814a705d5358077debdbc68d12a20e09928bd57e466ba46a9ae090e7d8bfdf73fe88b6d07324d8542e38b399ccac7b434684e3b3d2657f9e7674369"));

            /* Hardcoded VALUE for INDEX 115. */
            vHashes.push_back(uint512_t("0x016a47320ff5194e449fed2f5de63841f6290e5ed92e3d4393dff99449542c31b0c20887a7e6c44f504c0f890d4d58fc4082d1abed4ba92a4b92874963e46381"));

            /* Hardcoded VALUE for INDEX 116. */
            vHashes.push_back(uint512_t("0x0108e92b95722e6f1710a4f8c7162f3ccf430d9cb5623bd603f0b7b2e67868be804c553a31ac4c10b6b1ad345b7cfc4b58bd1710232b58c73b3fd59e1b15961a"));

            /* Hardcoded VALUE for INDEX 117. */
            vHashes.push_back(uint512_t("0x013ca68ad41e34e1e3f62a08cedce96bde4ebca8b4d6085505cc2c8c275f3a6c3b8e43b5718b752e8facb0b0e57ae3c579e2a748ae28790ca2dcebf2589299fd"));

            /* Hardcoded VALUE for INDEX 118. */
            vHashes.push_back(uint512_t("0x011940d8a2bc7340f7c08cfc0a39a7438e379c07ea182900d75d3aabd49b7442618e75e33a84689e801867ae4dd482f0cb05d72865beb6dda0f49e82dd959ab5"));

            /* Hardcoded VALUE for INDEX 119. */
            vHashes.push_back(uint512_t("0x01097f9e1b048bd13c1b9fb3e751586f3d63d5d1e6ea2547d04e00a02e292dc59f9a92df1036ed566d4ebc41a7c780832f7a6ec7dec41a88439072a9aa33dcfc"));

            /* Hardcoded VALUE for INDEX 120. */
            vHashes.push_back(uint512_t("0x01224abeccd816ccfa51335e9b362f887b95ee9ad65f43688665f3831bbf1896c39bc56a6dde9a0ad4b5bf3a769c199022dd1bffd455bd652816f3918a452a80"));

            /* Hardcoded VALUE for INDEX 121. */
            vHashes.push_back(uint512_t("0x013757c762fa8170b55c76fe6fdebef47a2291e37a5c32119df9b78ae4a2b5ee45bbbc16d3493be572c5631aed05cf759160511efdcf77fc3ebf277ba86b7ba1"));

            /* Hardcoded VALUE for INDEX 122. */
            vHashes.push_back(uint512_t("0x0173a906b4289e442500e7f2b04ecb653bbb2ac4c149fbee57ad483e54165e7299a7ae42063fcfda598c7281cc246f219c054388a672e00a48cfd8053ec6482c"));

            /* Hardcoded VALUE for INDEX 123. */
            vHashes.push_back(uint512_t("0x012551c259ebc8f256b68a97661b521fe000c0b9c119dee8fefe1a1d310ba9bf5706f8adeab222c71e2f95161a4ab0f914681e1377074d8637724e1567ee4fda"));

            /* Hardcoded VALUE for INDEX 124. */
            vHashes.push_back(uint512_t("0x013d1156a412e240946b1af9858c264b00d7e12d207b360bf32c3946aa9cc73fddadff989c1a3bbb97eaf2724c0731c24478dec8133ec52395352361a2c3cbdd"));

            /* Hardcoded VALUE for INDEX 125. */
            vHashes.push_back(uint512_t("0x015943d4ef59cbea5485ff397fd3d97d4878ae6882936694ac72821aeaf72db9b17fca6b7cb20e4a863fd7ee42fa1a5ac5da82d480f619414b99b334ede9191e"));

            /* Hardcoded VALUE for INDEX 126. */
            vHashes.push_back(uint512_t("0x01c8c6b99d21d9288be2d0be6148479e9fb37a3362b2608898fe49168b2f933a24d6ae039cb1865c189330d3874e6bcc110798f53a951d692985fed89024d4f3"));

            /* Hardcoded VALUE for INDEX 127. */
            vHashes.push_back(uint512_t("0x018804000470749459f0699337c3c3a2825e740da156459f9b59ab73951dd8f5d946e76720f88fafa48826a6d72ba1e4e0ae03a9cb6720cab3571b11d7608224"));

            /* Hardcoded VALUE for INDEX 128. */
            vHashes.push_back(uint512_t("0x01b5f0699b2a7f7e8f8fc24189ddcff55afb36ee75e6d5ffc95c9c26b30deba04c1278a2212f9b5595d214809edf35ce0eeb1fa740c5d5cf1d0d6d51f3b4c3b4"));

            /* Hardcoded VALUE for INDEX 129. */
            vHashes.push_back(uint512_t("0x014e5fca92a8bbecfbedbcec428634eee140edb95bfe365cb966d162842e8e12cb1dabcc3456b2577b1e6026037f5ddbd62d468f4d51848619d9566292a3ee0e"));

            /* Hardcoded VALUE for INDEX 130. */
            vHashes.push_back(uint512_t("0x0157bd26582a8e7bf4608b4a98a4f196016811bc6d9552feb5edd16f30c1ac5e8e9960c2dd1e3dbbee0c35d90c30b36539a292665b09f255ad9975769ff6518e"));

            /* Hardcoded VALUE for INDEX 131. */
            vHashes.push_back(uint512_t("0x013579bee7b7550982378ff2ab88740027d689daded3a68691b4c0cea1087be62f6eaede20999757fa75a8d7fe36e62251e31130ac3d4b31dc37cd56a808195e"));

            /* Hardcoded VALUE for INDEX 132. */
            vHashes.push_back(uint512_t("0x01ba9e15c1fc10810e2a77d98da30cc3a2c0d6dca27c4eef9378cd90f1cbaeaa8652a1f6c9ead1a3fa49f9f420d9f692bb4df29872079c034a4c096b4b79e291"));

            /* Hardcoded VALUE for INDEX 133. */
            vHashes.push_back(uint512_t("0x01693654ac0b33d8b8c5edf52d1aaafffb2b00a9c58ebcbb92f17d2e8424bb0d7af1cc10e32babafd82dd04b98de655c3330e652a61c74eb1d7ede82c9ee9db4"));

            /* Hardcoded VALUE for INDEX 134. */
            vHashes.push_back(uint512_t("0x0155326ff33b14fb1ca835cabc2be498a5827475fc96eff68c4d3a85ec42b8228979e762ac1ddfbdd9f0e53dfaae65ae7296f23a01702a89605f0eb5476ffa2e"));

            /* Hardcoded VALUE for INDEX 135. */
            vHashes.push_back(uint512_t("0x011438298cd111eb8a0d00c84053caab2d29d2612bcec0f919d175d7dcbd327ac80243fc6a4f0272686b4c7d0e8c8347a982f1fa9dcf645b11e722127f24a4a9"));

            /* Hardcoded VALUE for INDEX 136. */
            vHashes.push_back(uint512_t("0x01b2d050494cc1ffce01972aa508ab90f3016c216ef9abfb9858680a7bb3a580c98958218c321ba95119852ec7496beaf98b56129dcea8ffe89c0c3e3f596339"));

            /* Hardcoded VALUE for INDEX 137. */
            vHashes.push_back(uint512_t("0x019a0bb0e8b0bc7b353dedccdc3aade4021075776f3756d6c4fcd94ee8b3c878a76c9d4252dc891676cfc765d1ef61a8a768c08d5a515263ca26dcadf388f106"));

            /* Hardcoded VALUE for INDEX 138. */
            vHashes.push_back(uint512_t("0x01b8c20e0a8508bf49d8535ca29cc6919079e9a5b00701e08ccf0373e70122b9f3d096330e75e7ffe55c840353aaa9488dc133204e3ea7e1b266fff23bb4cfdc"));

            /* Hardcoded VALUE for INDEX 139. */
            vHashes.push_back(uint512_t("0x011dad5709c272507dcab84c9f69f6955320eb72fdae5b961b5d372fe7313cdf5a03b4f44c452846c54a79388d319a02c39cb0f27433905d217d3f591a92c605"));

            /* Hardcoded VALUE for INDEX 140. */
            vHashes.push_back(uint512_t("0x01e8f3daeb23648bd1871285eb5cee50cbbe4b585da86ead980add382f8a38038ac432321a3c34a21b1a5327017cd25de1f517728c8ac52eab2222c1a78d8c2c"));

            /* Hardcoded VALUE for INDEX 141. */
            vHashes.push_back(uint512_t("0x014d7d142fa42b8030ecfef9bfba087b7139ff35641030746bf18fe4dd4390b1f268338e8d6a625a896b6845fd2a827f3cb235ea2261f1aa7b5ce1db036a4abf"));

            /* Hardcoded VALUE for INDEX 142. */
            vHashes.push_back(uint512_t("0x019c3e396e0def9149eefad38d68f47bf34dac0bdec1cd63657d8ad7ec72da523d76429b6dde530368dbf5ce97dcef9e7ca9abb0842a04f21730a43981f6cd94"));

            /* Hardcoded VALUE for INDEX 143. */
            vHashes.push_back(uint512_t("0x015e180b445adde9fa482f0c28ffe209b64cecedfd8cf71123d4d0bd24be5a59b676c9418b4413d75ed24d8d11439ab114e694315147d1b3ca58a31dd0eaee37"));

            /* Hardcoded VALUE for INDEX 144. */
            vHashes.push_back(uint512_t("0x016b08f3408ae7372a74358f0e24a99ea95849b8f16be97ce6e3795c1d3d47452cccf3accfdb86ebf276bd8bd12cec403181fb8df96b660610f258dc6f9f3928"));

            /* Hardcoded VALUE for INDEX 145. */
            vHashes.push_back(uint512_t("0x01ab6a1feb6a68a8782b762a36a88b2dba16b3aa5a0de10216179e00a21336d97fc811bcf1187dc2123ebd76faf3347492d7c68c4df25f1af18082f1e8fc1377"));

            /* Hardcoded VALUE for INDEX 146. */
            vHashes.push_back(uint512_t("0x016cfaa1dcbee242a8a52be34d6552af4f4973816c49bf084cd4709ff53ba6a139180f4130400a7eb44e51302ab09faf72deef339248b5aa4049a238b16e0c29"));

            /* Hardcoded VALUE for INDEX 147. */
            vHashes.push_back(uint512_t("0x01fbffcaf193ad8c4f465014c84cffa8399f0b03901347ee7ef889bdb433411dfefc73c9d4f3923d75013a29297da650f95d71dc651b79f233970699f361ece4"));

            /* Hardcoded VALUE for INDEX 148. */
            vHashes.push_back(uint512_t("0x0110287537c0ce7a267c651798f9b843b8e85341c188307d612baf6ddce17fbc71ccc99a08d30e1bf3f69e20c8d507df1309ede4c0178f9ed600775ce60b7a99"));

            /* Hardcoded VALUE for INDEX 149. */
            vHashes.push_back(uint512_t("0x012fd1171cd2a9764f337861afb73f73c7831f85e79a2a3397968cd35100eed312cfab2449ffd9d028a3204e515690d2412cc8711a2ac7d054346a1cfa2bdd56"));

            /* Hardcoded VALUE for INDEX 150. */
            vHashes.push_back(uint512_t("0x01f782b6dde96668f9f8331a9fb626912642ea97dd8c9f576f9939d8fa8ee666162ff9a20a09e498c99dfeb5511bc342bd835f08f4a04bdd775f761a2e05c194"));

            /* Hardcoded VALUE for INDEX 151. */
            vHashes.push_back(uint512_t("0x011aa3a1a0aea4b0bcf9691532543bd05e4dcd55b08c49251f1a9d6c215307e0a2a0e1072cdbf70db9abfc24ce8c80ec26d714d60eab499d0c8bf219f20dc28d"));

            /* Hardcoded VALUE for INDEX 152. */
            vHashes.push_back(uint512_t("0x0180ac70ca99688eea83e0dda9dbf3c8cad9ff2f43ec326f3239354e23f343c7b7da1677ff788eb132a72483c7a4f64624fdbfb73512e8e47ebdf43f3ff13167"));

            /* Hardcoded VALUE for INDEX 153. */
            vHashes.push_back(uint512_t("0x01a59ac40ec3fd372d21f034d21fc33a5a794d55000307011f3316e0ebea3a4b709825cf30382c6da77a54c338f0949eeb3fbb60be9393a7bc9f5f3c6812c33d"));

            /* Hardcoded VALUE for INDEX 154. */
            vHashes.push_back(uint512_t("0x01954c227576e60b08f4f12742fdd8647e83366a53a8723802b151e8988679a18521df814ffecca49e397c1eade8d1931fd6a3d4d22152526b788b4b8074adc2"));

            /* Hardcoded VALUE for INDEX 155. */
            vHashes.push_back(uint512_t("0x01e45de5a166b58d89c1846e6775240ca77fb356fc8d35a6d32b66ebe4d7e79b3466449b8e1dd4eb419e043033f0707ef4b371c7c4bd868114e1b12b1178c195"));

            /* Hardcoded VALUE for INDEX 156. */
            vHashes.push_back(uint512_t("0x01d48565adbd5c736e8600187021d26fa79cfa4540bdfe1d466d4c4db286f9c199077916c76c672da82f77339e706b6311e235887632657c5982ddd5f5b12f79"));

            /* Hardcoded VALUE for INDEX 157. */
            vHashes.push_back(uint512_t("0x0177095eed695aaa5baf03aa7303c7b24d01856327fad119c6c5663f138799ef84caf3d8fe770cc7af8849c8760c2d5110c53a01622629e5271b6e8d23555792"));

            /* Hardcoded VALUE for INDEX 158. */
            vHashes.push_back(uint512_t("0x01c51eec62362d06f6933800e5a8e64d6a3a0e85b9932e4409343ad4c4496b94a40ee9ec1a5e3d0fb6a3bbc6f6ecfc41f60f59f257b111101da82f91bcf0aad4"));

            /* Hardcoded VALUE for INDEX 159. */
            vHashes.push_back(uint512_t("0x015d585a0dce5a62d9bb4eed0aed62a939dfd8fa37597e3fce6d9d5f49a4fe221c42de506d512c1212a748af0ece84c4b17aaf7c02021edefbf09bce75def25f"));

            /* Hardcoded VALUE for INDEX 160. */
            vHashes.push_back(uint512_t("0x01550c48dfa3c57f7174d0035fd790fcab7666947e95a970daf9420d760140e7091fe90babca2d6a76caa75e80cf42822be91c466bce46806906abfe9702b69d"));

            /* Hardcoded VALUE for INDEX 161. */
            vHashes.push_back(uint512_t("0x01bbb2e5daf0ea03b286bdf6c16b8f20f52e15befe5d33041057877cfb4fe60cd937bfdf8bed2e32b9a21352f83ca4d06076cc4ac51f12afd8381516ec210693"));

            /* Hardcoded VALUE for INDEX 162. */
            vHashes.push_back(uint512_t("0x010ef1be90736f09ee1ff472864df74eac739517aa816ac4f015282c2c891743aa849eca226d8c01b5f1e7213bb9a5bde9219b8ff7e1fad60e4ca889541f272a"));

            /* Hardcoded VALUE for INDEX 163. */
            vHashes.push_back(uint512_t("0x0151f77c71dc8f550274c52c7b4fbb2a03f6b5867ed08fc5b3da51c826edda760afd0687eda0f4b61d03e159a6548342234efd9b441106bb83766334ed1af8bc"));

            /* Hardcoded VALUE for INDEX 164. */
            vHashes.push_back(uint512_t("0x01613f9fe8948c4dbea14440101bb3a78c11401cad1b73f33150738168b660e1fa4cae7425ec41742ae927aa4d09fb2db32c37d7ccb1d2fbc342623eca55a783"));

            /* Hardcoded VALUE for INDEX 165. */
            vHashes.push_back(uint512_t("0x01d5c23f7506dfaec2c66593bfab3245d7f54217727dcb5c57a973dcfd66f1a40acc199c56023d85b046ec3b040c581f8e2b0addc484c89433d21bdadcef7811"));

            /* Hardcoded VALUE for INDEX 166. */
            vHashes.push_back(uint512_t("0x0134ca0fbcee3936feb405e79cdfd41965aecdee9060aaf87034d49443d48524b3d5a2562c3568fa410e45a169270bf43d1e1feb3527c672a4aaf3eac69e1177"));

            /* Hardcoded VALUE for INDEX 167. */
            vHashes.push_back(uint512_t("0x01d441b8d8e5591bdb0ac884ca611fbe2d7d3f78a18ae8db24d8b509126218ba1b2b44da2af2bb59a906da4b513b3046b2a81c5d4b75f3bfa8f6a72ff490b4ea"));

            /* Hardcoded VALUE for INDEX 168. */
            vHashes.push_back(uint512_t("0x01e333c18133fcbe570abd7631c0b052639253590faeb894d0bb015848ff1472233bc27182a78c74caf2a2c15fdd47f030a7260fd11302a484c20ba8c2a01f62"));

            /* Hardcoded VALUE for INDEX 169. */
            vHashes.push_back(uint512_t("0x0142aceb36e51f63f65fd973154acab0ad42d63edbd5c2847391b7432897f6bc82a4925f3e528e3a162971d84fc019ed8e04cad3a043c32ab5c2d5e26595eaab"));

            /* Hardcoded VALUE for INDEX 170. */
            vHashes.push_back(uint512_t("0x01c4ebd298e01be9369f25b17cf981a9df71aa16b2cc8c1ff42a5133d66bc557ab3b7c29d4e0dadd95abbe8a68f6e983b7118fe891433d2d0976a0bf9843720e"));

            /* Hardcoded VALUE for INDEX 171. */
            vHashes.push_back(uint512_t("0x013d2d12c0c4129b19cdf1a280770d21340717c7ce4272dbe58a0a66ccd68856dcf8466bcd6f657773839f5d158fd05986fac41ce0a76dd41a0f00485bdec9bc"));

            /* Hardcoded VALUE for INDEX 172. */
            vHashes.push_back(uint512_t("0x01f663df34f4ba4a061520c8cca5ddd117c05123da89b65c9f9326a99d4bba7cfe1f1ebd0b913f38ede6f1732aba4e56d281b2413f6aba0d63db3a37577175ee"));

            /* Hardcoded VALUE for INDEX 173. */
            vHashes.push_back(uint512_t("0x01ed6a2b6ffa29ad1862b7af1a2802d9a2d004aa4cdf6267baece5bd8ed75dc56c6892814115e7c3569899f61983b69a1927b95d778e935a3784b902e2f8adc5"));

            /* Hardcoded VALUE for INDEX 174. */
            vHashes.push_back(uint512_t("0x01ce9206cc15418a2f48fdeefc178ce5d3dee8354e94ea3db115513d9842439662765319798544f7b97de9b506c757ad2191d02e13eed6db917c43c5cb170bd2"));

            /* Hardcoded VALUE for INDEX 175. */
            vHashes.push_back(uint512_t("0x01f5a1ee17a9f7d4e658386777d29d5955ef09e7655ccbeb90e7259e613a2d93f991e00a5e561674cc37b26087c2d46a9c8341995118ba57c6675b6b571a614f"));

            /* Hardcoded VALUE for INDEX 176. */
            vHashes.push_back(uint512_t("0x01d675e5b430e5b72c9a7a1545d022e89d56d09f52e153d3854ad81b5b87af44608f7ba2025d5ed3c8c994a811ab1801724207256443cd049e6bfcffc513ccdb"));

            /* Hardcoded VALUE for INDEX 177. */
            vHashes.push_back(uint512_t("0x01cfe37a1c1ab99e3764dbc67c6ccd939b972d432e0c5baa36becd8791131e39a6f40fb301733b33e5a2a0e564adc971d45feb6fe9a2fe8b3e7c88b9b6a06e41"));

            /* Hardcoded VALUE for INDEX 178. */
            vHashes.push_back(uint512_t("0x01f2f401d54de47658f14c023e57c6624a9efbee9caaeb0a43ea217ebace338d52c356d1399b81a6b56645fe74e641f985813f711066e048f1d1f9be5c3249fd"));

            /* Hardcoded VALUE for INDEX 179. */
            vHashes.push_back(uint512_t("0x018c1056dba218220b7a1be8ad6b19aa1f5fe17a2314aa9e3f5f87e36b2c663bf9bc719030295ad4799fdec530b7cda0419ed7a06316dcc53e3ff67f5032ec79"));

            /* Hardcoded VALUE for INDEX 180. */
            vHashes.push_back(uint512_t("0x01b896e6b7037d1a2cac35c20a6f5c8d752eb91a102dede0a46ffc36ed65edd34dd15ef5deaf63a674035217c1e431da02e5701c7b2e85840c5bd196692039d3"));

            /* Hardcoded VALUE for INDEX 181. */
            vHashes.push_back(uint512_t("0x014c1513dde2008288409013e40cd31b523b6799ab0f439aa5e73875f48edff5271a26a6031a9856e76091e9fad6b56b1b0199d7e1cc3839985386c363b95c90"));

            /* Hardcoded VALUE for INDEX 182. */
            vHashes.push_back(uint512_t("0x011a3c2089d05a965aee2822f9b33b5c2238f1bb3f98809d545b8702830751b0fa9e936ba97c7f88b6301fdcefe2ad9ba368ab2d8805e5ef291b7f9774c57280"));

            /* Hardcoded VALUE for INDEX 183. */
            vHashes.push_back(uint512_t("0x01f02d41d909bb246c4d7109a2c42a432a3fb9e68a64842510b71881e3e090df3d3df280b69cdc245cbfdd88b3bc70306151724897e69c73f2569b7b44394db4"));

            /* Hardcoded VALUE for INDEX 184. */
            vHashes.push_back(uint512_t("0x010297a51637994aae1d85571ea406b3da3e760026c2d430f0ea96d4eff46f4ec7916ff94fd24b76323788a52a4f6905834a1724b8e563520eb78b1e2fd1e55e"));

            /* Hardcoded VALUE for INDEX 185. */
            vHashes.push_back(uint512_t("0x017d937f03644f506007103dcd9ec9d2eee2180a1496005e843acbf637c311df12e050330379c81e6467626e494450c8ad63066d726cc2c0c1e7f94ee4b9a3cd"));

            /* Hardcoded VALUE for INDEX 186. */
            vHashes.push_back(uint512_t("0x013e709a06fc56613f8dfb8ff6e3b6bf17901238ec5ba93f5e127fe293875d58364cc4262e34e7602cdf579190afca64521d1a968ded7a33d95e0892d70b7f3c"));

            /* Hardcoded VALUE for INDEX 187. */
            vHashes.push_back(uint512_t("0x017bf2785bcf3f87153e2ce745634a8a0fdf6de8bcdc4e84c2c15a759098d3780fc21242373089bee93eb1462b54e26141091c3fd41f3f02f806ba6ba901fe99"));

            /* Hardcoded VALUE for INDEX 188. */
            vHashes.push_back(uint512_t("0x01178acf482270746c23750e21506610b79333910e86b0d711023682cf82d6751be2fb2b71f6ceb46dcc384a6ac5bcb05394643783da2bcd69bf1981343a84d4"));

            /* Hardcoded VALUE for INDEX 189. */
            vHashes.push_back(uint512_t("0x01e7e5b268eab611c65d985d30a62c2ee3809fe13ef3c94ee55beff9a4f0e448ba7e6cc8cb0300731eeec6871ec22cc3c07d6cf34d083f426df30f7bf121ac39"));

            /* Hardcoded VALUE for INDEX 190. */
            vHashes.push_back(uint512_t("0x013ce5a11909f02b4b551cf193d83aff50b239cdead33e9feb4579809f808a970dcc63101d314c10437500e31e92f4730e3f1979a12cfd3eb6e7c3a6eba3de25"));

            /* Hardcoded VALUE for INDEX 191. */
            vHashes.push_back(uint512_t("0x01b43802eaea100320808db270e3a1b4882f7948fd13de1cd0a3e332832d8e561822a0499664d23f039d012e97e47b743ed00cc9acc3ff4fbabf98445738ba93"));

            /* Hardcoded VALUE for INDEX 192. */
            vHashes.push_back(uint512_t("0x0146bcdb4735e510c5c2875c3796aa7af17222071e83d18ff7d04df586959dfebe68de3d6b456ac79adda204d0ced805d945f535d7765fca61ac98bac989e2dd"));

            /* Hardcoded VALUE for INDEX 193. */
            vHashes.push_back(uint512_t("0x0158fca0408a1d92e365c91e792bd737b16cbe6f4e111d4794827728995eab1ead88cbcbb32b1afead8d3cfbe6e7433d3dad502c8ea36bcad7eb0286e8bab93c"));

            /* Hardcoded VALUE for INDEX 194. */
            vHashes.push_back(uint512_t("0x01a057387f3bd1d07b1b3b3752c188f2ceccde3b70b734475c0a7a48ed49a8b1b43b17474be847531536cfa0369018fb3227450b990af03f19ebb1c72dfccba0"));

            /* Hardcoded VALUE for INDEX 195. */
            vHashes.push_back(uint512_t("0x01264751943e19d478c51a10b1578f2990138700e1bd5d2e9d4d795ee24a22413af3265123dc00dc3cb0a2d3b4ae70c0efdee3ba396816432baa6cc7379da2f8"));

            /* Hardcoded VALUE for INDEX 196. */
            vHashes.push_back(uint512_t("0x018add54c9bc08f63656e7e2375cf5ad2904f4936482afa39d989f20740f5e3aa9aadf8e69d16f3f13c31dcf8c75253e810fdbb8331e477dce883612eb2de4aa"));

            /* Hardcoded VALUE for INDEX 197. */
            vHashes.push_back(uint512_t("0x015067688162ef411bbdc59df7c426d5ac8141cb873dde9407c42740f5116371603ec060cd1845f1ac2c28d0ae877c2c162f9d75b50b3f19d6cac1d5189a2360"));

            /* Hardcoded VALUE for INDEX 198. */
            vHashes.push_back(uint512_t("0x0187c70cb69cbb75dd3b632e946abc858ef586ef29bad7e6724782f4f07eb257aabc8eab64d778866699ce0789393806734313fb56e2fdb99b7dc43bb0345684"));

            /* Hardcoded VALUE for INDEX 199. */
            vHashes.push_back(uint512_t("0x0185cc4b11bfe769b255d3051a85d48c18d5bb427f3485600fd5b3c0551077e8c61e1ed553ec952cc3c6df1a3ca994587b3abc1c7a3cdfad67e25e7f084526b8"));

            /* Hardcoded VALUE for INDEX 200. */
            vHashes.push_back(uint512_t("0x010966c0fb7935a63e311ca2270d80bbc905b3ef0da7d0bda29e4f9eb555b7ddd71c3da707b90bd85ff3d47c7e5ecd6e6100cdd32a4dc2285c5cde6a2ba95625"));

            /* Hardcoded VALUE for INDEX 201. */
            vHashes.push_back(uint512_t("0x01763c9a6ae62bc379ec38851dcd61d731907219f2723882a992b7c098e578067cc8169c70e276baddd375e008c8580aa256c88388b52571213fce50c2fcdd4a"));

            /* Hardcoded VALUE for INDEX 202. */
            vHashes.push_back(uint512_t("0x0160326b37c604962dfe00dfd1cbc841b320b222a2230e9f00ad050f7ef9932e4bb55fda8a44699e120226085027f2631c4d13ddeda454e7ffb5c11b447c33bd"));

            /* Hardcoded VALUE for INDEX 203. */
            vHashes.push_back(uint512_t("0x011a9932797dcb14b75e8c2912b8a5dd8b9efe527dacf8117c378df8352fbb4170bbe1b06473220cbc9a1ee92c06d7f83102d2cfec0231601fa7127f1f72c19a"));

            /* Hardcoded VALUE for INDEX 204. */
            vHashes.push_back(uint512_t("0x01cc7904f9417421b747dfb82a5b662e5a67be7bcf73b91606b09f2a5698eae83b56b6f79ddfa38f7e9dfa12fe3912101a0efd8f4f68b304e6be0db04947cdae"));

            /* Hardcoded VALUE for INDEX 205. */
            vHashes.push_back(uint512_t("0x01f003d4cde34117db5bc22ce9b92f1cc71a1df1767222572284ca401c4e57d615e40a208a7c056c424162d62f4610e7fa2ceb6eca901a8920e014990ce706b7"));

            /* Hardcoded VALUE for INDEX 206. */
            vHashes.push_back(uint512_t("0x0116bd3bfa3df8a250f0ca1fc0d6ba19ed31b3731abc87a65e9c582cc331663bf9469d8493c004b47dfb6ae91f736bbc3d3d85712622bb4fcd02b589b51d082d"));

            /* Hardcoded VALUE for INDEX 207. */
            vHashes.push_back(uint512_t("0x01d3bd9f3a5760364a8a0beef4390c7a8a71bb62a56474d3f77bf36c118885e12165e5437e1de26962c9287dc76e13bdf74e533de5ac0a4f31ea91fb2b79dd0d"));

            /* Hardcoded VALUE for INDEX 208. */
            vHashes.push_back(uint512_t("0x01f02db69b073f37f4b769736aeea919e4b0d84bbef6698d6d9477f79d0ef9fb8018133406968836e8f28a2bdbdfa2b8a19a24f85f1fcb8e7f7cae8bf299fa4b"));

            /* Hardcoded VALUE for INDEX 209. */
            vHashes.push_back(uint512_t("0x01dae47b0ca8724533dbca4d26d178f437c6f4f6a3674a66a45bcb06ebf4109f97ea3173db19f99bd45599e5877562a3698ca03504b19bad293e34496ad0a20f"));

            /* Hardcoded VALUE for INDEX 210. */
            vHashes.push_back(uint512_t("0x014751bd8669c49b8cf42f7fa1919d48be76dd06c316758859a4701c97d9f8df47dae7f14130cd66fb947ca9b4766c53f859fcd846323058dfda06d0215ed296"));

            /* Hardcoded VALUE for INDEX 211. */
            vHashes.push_back(uint512_t("0x01e59ddbe23e9986d797c2196c019c20a0b375de01a6ad15664d5fc7054cfdf2a6a128e23602fcd7888b86bc2c54777039d889ebae3dfd59d1f6c214fdacdc9d"));

            /* Hardcoded VALUE for INDEX 212. */
            vHashes.push_back(uint512_t("0x016e78a3385420d9874496b57d7b458665762b58e734c6dc9b5739dc4afa51ef8ff759b121b981ff15b988fd791364cc81439b7f8cd34d89bb5f1f70ebf5d9dd"));

            /* Hardcoded VALUE for INDEX 213. */
            vHashes.push_back(uint512_t("0x0116f8d1d53feed4a7b0c948dc1c461c90ae55cb68262ce1136ba38305dc46674cc65828c13b3a1a58cd39d8908835f3fed2d8ee22e4f12cadf41d1c759a6d55"));

            /* Hardcoded VALUE for INDEX 214. */
            vHashes.push_back(uint512_t("0x011a38583de608e43a960aa644794507cd1ffeb09f26c2a0061d9a07547d02e07f724b7390ebfab1dc7917715b57e3eecea7a122117436edb6714a6b82b2bf20"));

            /* Hardcoded VALUE for INDEX 215. */
            vHashes.push_back(uint512_t("0x013486e180796851671429c1795f255ab3f944d36f461d36fb9c4d1b5bbef3a3b283deba55416a9e5b55e81d87cf26ad2c09458b702258a9a71c1204be8d7c36"));

            /* Hardcoded VALUE for INDEX 216. */
            vHashes.push_back(uint512_t("0x0118c60e99cb804c5b7ddf5cc5b073fd771436299e3d4f1ba9ffcab7db6777847e2c4ce64dc57255ab99f340e0199dabdca7367b6b12f3f615c4a67b1678d165"));

            /* Hardcoded VALUE for INDEX 217. */
            vHashes.push_back(uint512_t("0x01a3a6959d2520e660c85b9a9ea5d41d847772e21defd60089e770dee6eef21073c08b58e24a25241765661c45ece8ed3d0e0e0eeba349826ad42cadd04ee2ef"));

            /* Hardcoded VALUE for INDEX 218. */
            vHashes.push_back(uint512_t("0x0165aa2b1ff3ae8907bdcd11f4ed10a3c520668eab306835bc404d457788a81dd8ef8e14555356f8c19b7f4dbf0ca44fba24551488d00945c5b09817733ae58a"));

            /* Hardcoded VALUE for INDEX 219. */
            vHashes.push_back(uint512_t("0x01131f35e68ec69264b2e405da076c0194a8c05869bdeb27396f1d6ab14a7477245a604811c09cb3d53f7bf7bc07157227e85976965015a046bd7b8199e7c07d"));

            /* Hardcoded VALUE for INDEX 220. */
            vHashes.push_back(uint512_t("0x0182554646f7ab32dc18d83a02f5ecd1919e993a351c7105e3d707a74a3c2e7b50f5c2e79430e05ba2569a6a8e04d6ed51217a0f078806442e408cb89835b54f"));

            /* Hardcoded VALUE for INDEX 221. */
            vHashes.push_back(uint512_t("0x012c8d55fdf8b444e883db45c4b4d408461dfa9f4838bb41f06febb35b13dc110e5422d69865101682ba14a56eda0d312f4f57ec155a6fe685400a350b96a6a2"));

            /* Hardcoded VALUE for INDEX 222. */
            vHashes.push_back(uint512_t("0x01bc2baf34317c639a466a6726df0d447b45ab282a39c110b6a0b48d0c28b2351cc15214799635b789b9bbe71da1294e58f4dd559fa221799cd81fae56775576"));

            /* Hardcoded VALUE for INDEX 223. */
            vHashes.push_back(uint512_t("0x01fc971735cc01805d374be55df8175c0720563f67abd4dabcfae384ae403f851f09bb22afcae1ee747120b6bd3954a73841d0b5b060317a0103365ea1c75bcf"));

            /* Hardcoded VALUE for INDEX 224. */
            vHashes.push_back(uint512_t("0x0120274d64274d12e3ef6b829ad280b186b919cf2862974114af0b9e41b8fc4a57b69c2417bd1b0e97ecf6fcf99d2efd2e1c8adb826e65cada44f51aa4cf4d43"));

            /* Hardcoded VALUE for INDEX 225. */
            vHashes.push_back(uint512_t("0x01fc56500e0b4fffcf2078465c639b291911055d99d3cd9f14a45fd2c76116ea1c4168e24af24b41f20b1d6f81bd02d5dc6702b0763143e61c705eca054c2adc"));

            /* Hardcoded VALUE for INDEX 226. */
            vHashes.push_back(uint512_t("0x01b3cb2b96093ed46b2cf1ff04442d0a39daca93fc8cd66c5504a7642201a924e7a505c2c8054eefeded32e254323f827ca5ec326f00f6803a466a5dcf1d83de"));

            /* Hardcoded VALUE for INDEX 227. */
            vHashes.push_back(uint512_t("0x01b68c3811429f770ac62ea2f3b62928f2ed9c083db597af549aae408fc82bc49dc8b6cba96f5ec316cc5387c808d266c38b2a0aae4b9c2af42aad69d3231deb"));

            /* Hardcoded VALUE for INDEX 228. */
            vHashes.push_back(uint512_t("0x01a26181bdada9ea9e2c9ec35b88b95dd83ee3be6fae2f2a3bbbe77bb95812132f12071f6be8ff14deda7896c2652f5eabc94a5b5b481d7e84d4acb823fb8857"));

            /* Hardcoded VALUE for INDEX 229. */
            vHashes.push_back(uint512_t("0x01632ffbcb5d0d391d7e5c27a4f0e624d7fd471877e8f6a2c5cff81fd786d2b2006a06024b31e56c0e4081a19d865a4aa27a8815e08b1c784d9e168aa2564605"));

            /* Hardcoded VALUE for INDEX 230. */
            vHashes.push_back(uint512_t("0x01c86553c65a95bd91a317bc03bfa31e3ebc786335d00dd7b71b18d8bd1ae586c444163d9310840b4aedec9d282d5798378ecab6f3ca276417e5f661298a53f2"));

            /* Hardcoded VALUE for INDEX 231. */
            vHashes.push_back(uint512_t("0x013982d6d9730c9b9061e00e833534434787c7a631d90d399035306107ad0d81c282c2da627e10d7fbd6be181bc295d0b7067600212ef4a2e8ec27a9124fb0ec"));

            /* Hardcoded VALUE for INDEX 232. */
            vHashes.push_back(uint512_t("0x01cf9fa35cdf2c854db832d9830e566c9fd25c9758cbaa5dd82196f93be663fbcb6db6b21c17ebb5662e74585a780453cc258737fb0ee05c7274285adb3f9d71"));

            /* Hardcoded VALUE for INDEX 233. */
            vHashes.push_back(uint512_t("0x01288e48292334e925c705dd843adf5eab56c06c27bcf97e25121b4ceaadcf0e83b7769fe325a74a8db71c1e8b2b743d8431062c27028c3f3338e58f08cb06d0"));

            /* Hardcoded VALUE for INDEX 234. */
            vHashes.push_back(uint512_t("0x01558bbe65d654e19674340dc838bd2c305dc25f1be068aa797ed87b96971678ae8d1e35d7a981fdff21e929883bb89823dfcec630e9e390c7bb0e3b42c83cd7"));

            /* Hardcoded VALUE for INDEX 235. */
            vHashes.push_back(uint512_t("0x018d40629bbecd48050a928b689a67e23fc764b6ceadeb786d9e43c745140ba6856f34b7e5f5dd2100ef2f4eda1f17bb326fbbcc77b7390d3e1382e4be5eb719"));

            /* Hardcoded VALUE for INDEX 236. */
            vHashes.push_back(uint512_t("0x0182edbf532843cf6f52e6f911d9fa13f5ea32a3b26ab32815326ad1121890a2668603648ab5fc29176e2395f24cc1bffda4b66cd8a88f39fb93ba0e9bb63842"));

            /* Hardcoded VALUE for INDEX 237. */
            vHashes.push_back(uint512_t("0x018ec3cfd5d8b40f0216ec8ac0fb2d6e521cd5cfcf5f34fab638ca8b17f80d7217813974438478f41d551c44652bb726b9ca3833339547047e60aa0bbd70419e"));

            /* Hardcoded VALUE for INDEX 238. */
            vHashes.push_back(uint512_t("0x016108def3b313c3a18429f6cd21d9b12a567c2d581b89c98085db9c65f68d99c10ffe0c32f836ad4d5cf75827702263f64656728f64383799afca9c7e82b8c5"));

            /* Hardcoded VALUE for INDEX 239. */
            vHashes.push_back(uint512_t("0x01c8e0920cf29ab81a931f6be9915555ac8aea67f3415c433443d6ec5a95414a1e6791979ee3caee8496f53675be95f077cac1e054ae7cc4e51d7487f6c91d24"));

            /* Hardcoded VALUE for INDEX 240. */
            vHashes.push_back(uint512_t("0x016c5d8f04ea4c8d083db29d0f86a8ab52d98e9d67dd3939233db980b5febb0ebf7b2710f11c6323efebaa054a3a850884fa65ee0c4d7157d659a7845360ffe3"));

            /* Hardcoded VALUE for INDEX 241. */
            vHashes.push_back(uint512_t("0x015aaebce88f71a354c4fc316b7086af03f7a85cdd2c24764a4dea0d3373a91c91c1ef26aae5435abe4e2a45abe36da140ebd480223c6f607e1030ec27360d84"));

            /* Hardcoded VALUE for INDEX 242. */
            vHashes.push_back(uint512_t("0x0151e4e26fda03cce5abd163c9d02af2312a6f93e296b538c37bea3c2edd9e6f9ca078ecd146a1f40c494502c27c4906175c0b5f6850f1b38f82700630563c1a"));

            /* Hardcoded VALUE for INDEX 243. */
            vHashes.push_back(uint512_t("0x01e0568a5d8f3a2d1320e2309a691ac76c0c18641176f8d8ac02a3040d88c79391b9b01aff901618318b127272d2f9669c18dca752d6e56473446b3df5d6f3df"));

            /* Hardcoded VALUE for INDEX 244. */
            vHashes.push_back(uint512_t("0x01dad343e414c86505d0ff807aef041b3deae77c0db271c4a9c8832ba3fa1d6aaa5ab6cedb5a4f096a6027338646f8796571eee979e46a52a212f85fd4bc20e8"));

            /* Hardcoded VALUE for INDEX 245. */
            vHashes.push_back(uint512_t("0x017e144a850186ce4080689e07be9a87d8711d0746987f19ec286e4b83e3e0221e1d8b04fda64f22f35f16664e8b8e145d335ebd2ba2b8e1ac69a50f1f6836b1"));

            /* Hardcoded VALUE for INDEX 246. */
            vHashes.push_back(uint512_t("0x0130809248f8d13b528b615fd32771c6bbd6c87a2a27f884b11a15935e52bdcf93c1710545a277b1452e69ae1bc759773def85ccfe7a40a2c9fb06d69726c51a"));

            /* Hardcoded VALUE for INDEX 247. */
            vHashes.push_back(uint512_t("0x01a2947f21f981e2eba855d8b81685894d883628c7d89294efcebc5850cd81899ae4e2040943c672a57873669f3f1bf0c64a69b45b671c1949b20095267210d8"));

            /* Hardcoded VALUE for INDEX 248. */
            vHashes.push_back(uint512_t("0x01bb75cd9ef8cd4ed500cb9dbd135c7d85fb361d5ebbf53b7e1b23b19a79af89c3cb8f22760ee45ceca295e95f99c09eb7eece16528eaf94acc6533c396179bc"));

            /* Hardcoded VALUE for INDEX 249. */
            vHashes.push_back(uint512_t("0x0185e03a7875b171a8ad87494e68958c94ee068585a3ffa25dac55996c6b6bf3c9af406b9d2a2abb962bfb7b76246f8eda082ee458e337402de6b8c358284089"));

            /* Hardcoded VALUE for INDEX 250. */
            vHashes.push_back(uint512_t("0x01a3ac67bdc21f7c735bfa403c41dbc487f609741f8992bf9cc35afb8c1298ede778b120e21e452639c264fcfafd1c634d58c39a7dd1fbcc9a7d4c475d5edc03"));

            /* Hardcoded VALUE for INDEX 251. */
            vHashes.push_back(uint512_t("0x0190bc56f9d2208fed2ce8dd4a4a29b84aa2687e640d2e3efafc6a53dae26cf81d131ae89cdde362f53f00b4f37e2c6a78c93efb9843eda877a4eacb8d4fde07"));

            /* Hardcoded VALUE for INDEX 252. */
            vHashes.push_back(uint512_t("0x014429987c7fbd630ff8f59a2c54f2ea23c32acac238009e268fd7facde7d4f520ee4b2c293920cc8bab662cf72150c75c98a2fce22edb7811685a3b11e39175"));

            /* Hardcoded VALUE for INDEX 253. */
            vHashes.push_back(uint512_t("0x01310beb6ee855ef0ddafa334566023d76e861d93b33f300fec4753fd71ef1972e8ac8de76e943dc8e9fdbf07924e8d9d05c10805233c0d7872cbda118e8c31b"));

            /* Hardcoded VALUE for INDEX 254. */
            vHashes.push_back(uint512_t("0x011ff419ef1a2f9e8e3ceade8f62228e094b800fdcd352dd93496c774ddeae4501e79a802c5e991f304846ce2e4c51df52eb4e84baab91d2520785058c3db1a9"));

            /* Hardcoded VALUE for INDEX 255. */
            vHashes.push_back(uint512_t("0x010a5e193f880b8267531213e6154dca629ce1faa44cb38195f6d0b9b9132098cb8960eda5d0dcc657ef59a7fceccd2ef8b82458197a1ee362848201c31d24d3"));

            /* Hardcoded VALUE for INDEX 256. */
            vHashes.push_back(uint512_t("0x011a4918c0b59a1ada6e7623d47259d142667e9e148919e5c2532d4b8f13e9fc78d566768a11142f62b9639b6ff8e6b4d9a04d781f1908f60aa5f25cf0690258"));

            /* Hardcoded VALUE for INDEX 257. */
            vHashes.push_back(uint512_t("0x015a55a087ed3c74cc2c69da2049cd9db7801b9e29e962c5bd2cb1e4bb6d963a07e5dc017d40b79ee2a71b358cc15e1914c8b66e0a8c6340b1b268a046d1a60f"));

            /* Hardcoded VALUE for INDEX 258. */
            vHashes.push_back(uint512_t("0x019b733e191b1b9e7d5c94abde98a3c2c0c707b1b40ae3a01af1d09fcc1a096c1ce9bd28b9ac9fd43c306b76f6880e9187661fdab4986ff9a2407943b1c4691e"));

            /* Hardcoded VALUE for INDEX 259. */
            vHashes.push_back(uint512_t("0x014eb192c92b630ca3491e7553a21f44d4a95b1ebd7e61e9ef75c1f04367308283af012710f55353944421671af55b6bc28eb5e0fc08dced97d8588385657f8a"));

            /* Hardcoded VALUE for INDEX 260. */
            vHashes.push_back(uint512_t("0x01193d77fcb2050c199b0b9e9d253d8641a8e763ea082d5e09d82642f99931ecd8484bd9fb3d214bdb262406a247fd2d496d6ee7be7a76afd9365297a43ee552"));

            /* Hardcoded VALUE for INDEX 261. */
            vHashes.push_back(uint512_t("0x015870a8459529895b24217aec413db19c1ed3c46735a726697ad678a99da83b1a8200ef069394eedf439a6b6e70814918a1d5a71a46254199a4481bc69739d7"));

            /* Hardcoded VALUE for INDEX 262. */
            vHashes.push_back(uint512_t("0x011e351b0a46002129feee667eff4e0ebff41e3ea1cf3379c32f0e58b0e19004ab39ee72e1e9d976da14f570ca230c48e3bde610946c3dce813842e28900fa78"));

            /* Hardcoded VALUE for INDEX 263. */
            vHashes.push_back(uint512_t("0x01021be79c7c952afe36d04bfbbecd3bd80313e3c746b33b69fc8fb1af8b05ac2f5a43fe61fe0f85b1f8186f9f20d554240d1823ebbd3e32a96536dd8c7cdf37"));

            /* Hardcoded VALUE for INDEX 264. */
            vHashes.push_back(uint512_t("0x01c0686cb05b5c674c81a8d120c69075973b0366da828e0b55fdfbd3894ec5f887e1301519a68ef56cd8319fa10e00866b5f8849b60dc56b85f91ed69ec8c206"));

            /* Hardcoded VALUE for INDEX 265. */
            vHashes.push_back(uint512_t("0x0180e5ded2f8a666fa9fbc303727d801b2e75e749564611bbe2b8f07ba72c36b43e38808e922c470ed57ed1853b341c13d94cf14127448f0d92588745ae651b8"));

            /* Hardcoded VALUE for INDEX 266. */
            vHashes.push_back(uint512_t("0x01960b969d187e9fd232b94e7a4e627d1b2c9adcb97bb9a9cda4a14301dcf88c59e5b027f35b0b2e4795c599b58333e8673028df4feb141dc3cc6cb71dfd622c"));

            /* Hardcoded VALUE for INDEX 267. */
            vHashes.push_back(uint512_t("0x01d0f38c408509b046394b5ef7892880c06dfe83bcc05c80e1e689e2cf0b7bbcc614895c6a750c2b2cab216611c79972c220c9b6a4192a2daae42058512d2b8b"));

            /* Hardcoded VALUE for INDEX 268. */
            vHashes.push_back(uint512_t("0x0148440e9872c60dad8c671fb1d55f8eeb2a4d05c0efbe553defcb25bf9e354634a276a35464820370081f23ac34fcff80716452c399532fce34312957b7216d"));

            /* Hardcoded VALUE for INDEX 269. */
            vHashes.push_back(uint512_t("0x0197b34524000af3f0e8e93199f4f1f7311d5cf2091495944a65fc4adf9e1f1ac9096996b57f1d7a04f17c1ec6fbbc686f384295d532e6341caf4896f58ef21e"));

            /* Hardcoded VALUE for INDEX 270. */
            vHashes.push_back(uint512_t("0x017ff81cd2502ef33afbec491a791c1e1d2c332f74a6d73febb39c416fd0abe0f27054af126763786892d539adff1766f9130c76f5f311e2ea1168ec7e7793ff"));

            /* Hardcoded VALUE for INDEX 271. */
            vHashes.push_back(uint512_t("0x010c6f5157e5e6f06136a9422dad2c796e409eca1c86600200532ebbb88659f439b6f5d028a5fd56dacb5f2206b482ac80317b6d9f0ed07d94a677b71b0b8ed7"));

            /* Hardcoded VALUE for INDEX 272. */
            vHashes.push_back(uint512_t("0x0113e8899e20eefa3898d8c0911bf99cf813453c0eb8d853b71e8bb763a610b90e7045c24ad1454bd58dea52368e3fe93a6346d76c33fa4350e6cddfbff04e32"));

            /* Hardcoded VALUE for INDEX 273. */
            vHashes.push_back(uint512_t("0x018ffaae47c7f96a85f54d3e682aee456d324d106511086450fc2a9427a7accdb80250d5fa5d0b609f4399f8bc078df6cbb82ac399d29aee2f83bcc0f595e71f"));

            /* Hardcoded VALUE for INDEX 274. */
            vHashes.push_back(uint512_t("0x01c9445cdcff8f450bdb59217936fc4f99a8447e6a5aabe9e1268414b9fafc5702f181d50f0446399b0b30b073db9ca55053dfca56ce334d302fa83718ca8378"));

            /* Hardcoded VALUE for INDEX 275. */
            vHashes.push_back(uint512_t("0x0182967f736efbfe69a3ea1dbbd91a31d5e4d26972895b4c62ae123f95907a5c0065b3d54ec2746079087e0e225e141bca4b6b9bc5d57fc3935c1d7e301f47ca"));

            /* Hardcoded VALUE for INDEX 276. */
            vHashes.push_back(uint512_t("0x013f1f5d6a6cf07bf3f9ce010816614cb456a8df8f32dd9b50e2a77ba52de94a5e60a1624642bc9320f5b671eb38afd62cd0983ead653e1d0f4e47dfda25bb4e"));

            /* Hardcoded VALUE for INDEX 277. */
            vHashes.push_back(uint512_t("0x01e54e9c3ac225acbcd247a59ec9cca20353ce99ba6c3b3d60feaaff71e4a734b7b138d548a738677deba9c227a06cb6725f77a4109d99faa7f814c3992e40a7"));

            /* Hardcoded VALUE for INDEX 278. */
            vHashes.push_back(uint512_t("0x01d68d56a43c9e83e7d805233d4f0353b1a229a87d0ec3cdcd19d3532f87c70c133d3b8a5d7ad436451f94181b53059a6cbb6ade31ac7ffbfb379b91b57bd361"));

            /* Hardcoded VALUE for INDEX 279. */
            vHashes.push_back(uint512_t("0x01f1f39f47c0851af86347d5c22903583af3872c68614e19cfbc9f87b5bc8b48d5bf20f962b254203290bc0e83c3dd3d243cf7945519ff0acce8b6b99b82815c"));

            /* Hardcoded VALUE for INDEX 280. */
            vHashes.push_back(uint512_t("0x01143a511e4c057fb5435981d9fa6d9da040f1d74466ee52a8e7f481a90672c17b873dfc380f98a76ba9e91a4d727f50c4bfa223cc32110f4c5ec411a94f1c35"));

            /* Hardcoded VALUE for INDEX 281. */
            vHashes.push_back(uint512_t("0x01a01203b8c5048bd5fd3b7add5f9b684425e591ca9689fdf3fe282965c5f8fc9f07e4f3a45888da3c014030ae984ca5a8553088713d4035c3d6f80a59b222ca"));

            /* Hardcoded VALUE for INDEX 282. */
            vHashes.push_back(uint512_t("0x023989307f5562dcb479b0872198a4aae13ce82f1eaca7dc92b55b4ba17b87a3060b7b8888dd9cdd43f97c1662fda3538575ee000c8a230a7b1f2b99408db0d0"));

            /* Hardcoded VALUE for INDEX 283. */
            vHashes.push_back(uint512_t("0x025c2037ab23d63f25bb1254890d93e3cb924fcc30083d6aec4d07b2c13a61272b6a8984b8ce9a9f009ac7e2517f30b422f3203efabe4792a0264fa49b22d63f"));

            /* Hardcoded VALUE for INDEX 284. */
            vHashes.push_back(uint512_t("0x01cf75ea0b0e3dc5afb793f1a71c3edd81fe8c0319aac3a5f1aad4b222e051349e31a31a2bf293a9c465cf0ef4e5709ad93d86c24b2bd4d079008206b1947bfd"));

            /* Build the proof of work offsets. */
            block.vOffsets.push_back(12);
            block.vOffsets.push_back(4);
            block.vOffsets.push_back(6);
            block.vOffsets.push_back(2);
            block.vOffsets.push_back(6);
            block.vOffsets.push_back(6);
            block.vOffsets.push_back(4);
            block.vOffsets.push_back(2);
            block.vOffsets.push_back(15);
            block.vOffsets.push_back(1);
            block.vOffsets.push_back(242);
            block.vOffsets.push_back(0);

            /* Build the merkle root. */
            block.hashMerkleRoot = block.BuildMerkleTree(vHashes);

            /* Check the merkle root is correct. */
            assert(block.hashMerkleRoot == uint512_t("0x32dd0b923868c75698078b2011b6ba9cdcff757cd79143e5f5a17384d93fc4f257cd72cf573f6d71180ee5467684725ed9166e40f0e77e1fab4f04167498e50c"));

            return block;
        }
    }
}
