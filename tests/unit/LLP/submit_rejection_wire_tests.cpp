#include <unit/catch2/catch.hpp>
#include <LLP/include/submit_block_rejection.h>
#include <LLP/packets/packet.h>

TEST_CASE("Mined block rejection is exactly one reason byte", "[opcode_utility][submit_block][llp]")
{
    using TAO::Ledger::MinedBlockFailure;
    for(const bool acceptance : {false, true})
    {
        for(const auto& item : {
            std::pair{MinedBlockFailure::WORK_REVOKED, uint8_t(0x0E)},
            std::pair{MinedBlockFailure::STALE, uint8_t(0x01)},
            std::pair{MinedBlockFailure::DUPLICATE, uint8_t(0x04)},
            std::pair{MinedBlockFailure::OTHER, uint8_t(acceptance ? 0x12 : 0x11)}})
        {
            const auto packet = LLP::MinedBlockRejection(item.first, acceptance);
            REQUIRE(packet.HEADER == 0xD0C9);
            REQUIRE(packet.LENGTH == 1);
            REQUIRE(packet.DATA == std::vector<uint8_t>{item.second});
            REQUIRE((packet.GetBytes() ==
                std::vector<uint8_t>{0xD0, 0xC9, 0, 0, 0, 1, item.second}));

            LLP::Packet legacy(LLP::OpcodeUtility::Opcodes::BLOCK_REJECTED);
            legacy.DATA = {LLP::MinedBlockRejectionCode(item.first, acceptance)};
            legacy.LENGTH = 1;
            REQUIRE((legacy.GetBytes() ==
                std::vector<uint8_t>{0xC9, 0, 0, 0, 1, item.second}));
        }
    }
}

TEST_CASE("Submit block early rejection codes are one byte", "[opcode_utility][submit_block][llp]")
{
    using Reason = LLP::OpcodeUtility::SubmitBlockRejectionReason;
    for(const auto& item : {
        std::pair{Reason::UNKNOWN_TEMPLATE, uint8_t(0x09)},
        std::pair{Reason::CHACHA20_DECRYPTION_FAILED, uint8_t(0x0B)},
        std::pair{Reason::ENCRYPTION_REQUIRED, uint8_t(0x0C)},
        std::pair{Reason::NO_SESSION_KEY, uint8_t(0x0D)},
        std::pair{Reason::PACKET_TOO_SMALL, uint8_t(0x0F)},
        std::pair{Reason::RATE_LIMITED, uint8_t(0x13)},
        std::pair{Reason::SESSION_INCONSISTENT, uint8_t(0x14)},
        std::pair{Reason::PACKET_TOO_LARGE, uint8_t(0x15)},
        std::pair{Reason::CANDIDATE_PREPARATION_FAILED, uint8_t(0x16)},
        std::pair{Reason::WRONG_BLOCK_TYPE, uint8_t(0x17)},
        std::pair{Reason::MALFORMED_SUBMISSION, uint8_t(0x18)},
        std::pair{Reason::INTERNAL_ERROR, uint8_t(0xFF)}})
    {
        const auto packet = LLP::SubmitBlockRejection(item.first);
        REQUIRE(packet.LENGTH == 1);
        REQUIRE(packet.DATA == std::vector<uint8_t>{item.second});
        REQUIRE((packet.GetBytes() ==
            std::vector<uint8_t>{0xD0, 0xC9, 0, 0, 0, 1, item.second}));
    }
    REQUIRE(static_cast<uint8_t>(LLP::OpcodeUtility::RejectionReason::RATE_LIMITED) == 0x08);
    REQUIRE(static_cast<uint8_t>(LLP::OpcodeUtility::RejectionReason::MALFORMED_PRODUCER)
        == static_cast<uint8_t>(Reason::CHACHA20_DECRYPTION_FAILED));
    for(const uint8_t code : {uint8_t(0x06), uint8_t(0x08), uint8_t(0x0B)})
        REQUIRE((LLP::SubmitBlockRejection(code).GetBytes() ==
            std::vector<uint8_t>{0xD0, 0xC9, 0, 0, 0, 1, code}));
}
