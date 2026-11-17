#include "util.h"

#include <TAO/API/types/commands/system.h>
#include <unit/catch2/catch.hpp>

TEST_CASE("Mining info returns statistics for HTTP requests", "[review258]")
{
    const encoding::json result = APICall("system/get/mininginfo", encoding::json::object());
    REQUIRE(result.find("result") != result.end());
    REQUIRE(result["result"].is_object());
    REQUIRE(result["result"]["enabled"].is_boolean());
    REQUIRE(result["result"]["height"].is_number());
    REQUIRE(result["result"]["connections"].is_number());
    REQUIRE(result["result"]["channel"].is_number());
    REQUIRE(result["result"]["active_miners"].is_number());
    REQUIRE(result["result"]["authenticated_miners"].is_number());
    REQUIRE(result["result"]["miners_with_reward_bound"].is_number());

    TAO::API::System system;
    REQUIRE(system.GetMiningInfo(encoding::json::object(), true).is_string());
}
