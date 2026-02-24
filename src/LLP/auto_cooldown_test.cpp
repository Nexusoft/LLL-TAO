/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

/**
 * auto_cooldown_test.cpp
 *
 * Stand-alone unit tests for LLP::AutoCoolDown.
 *
 * Build and run independently (no framework required):
 *   g++ -std=c++17 -I<repo_root>/src auto_cooldown_test.cpp -o auto_cooldown_test
 *   ./auto_cooldown_test
 *
 * Expected output on success:
 *   ✓ Test 1 PASSED: AutoCoolDown::Ready() returns true when never used
 *   ✓ Test 2 PASSED: AutoCoolDown::Ready() returns false immediately after Reset()
 *   ✓ Test 3 PASSED: AutoCoolDown::Remaining() returns a value <= 200 s after Reset()
 *   ✓ ALL TESTS PASSED
 */

#include <LLP/include/auto_cooldown.h>

#include <cassert>
#include <iostream>

/** Test 1
 *
 *  AutoCoolDown::Ready() must return true when the cooldown has never been started.
 *
 **/
static void test_ready_when_never_used()
{
    LLP::AutoCoolDown cd(std::chrono::seconds(200));

    assert(cd.Ready() && "Ready() must return true on a freshly constructed AutoCoolDown");

    std::cout << "  \u2713 Test 1 PASSED: AutoCoolDown::Ready() returns true when never used\n";
}

/** Test 2
 *
 *  AutoCoolDown::Ready() must return false immediately after Reset() when the
 *  configured duration has not yet elapsed.
 *
 **/
static void test_ready_false_after_reset()
{
    LLP::AutoCoolDown cd(std::chrono::seconds(200));

    cd.Reset();

    assert(!cd.Ready() && "Ready() must return false immediately after Reset() with a 200 s duration");

    std::cout << "  \u2713 Test 2 PASSED: AutoCoolDown::Ready() returns false immediately after Reset()\n";
}

/** Test 3
 *
 *  AutoCoolDown::Remaining() must return a value <= the configured duration
 *  immediately after Reset().
 *
 **/
static void test_remaining_leq_duration_after_reset()
{
    const std::chrono::seconds DURATION(200);
    LLP::AutoCoolDown cd(DURATION);

    cd.Reset();

    std::chrono::milliseconds remaining = cd.Remaining();

    assert(remaining <= DURATION && "Remaining() must be <= configured duration after Reset()");
    assert(remaining > std::chrono::milliseconds(0) && "Remaining() must be positive immediately after Reset()");

    std::cout << "  \u2713 Test 3 PASSED: AutoCoolDown::Remaining() returns a value <= 200 s after Reset()\n";
}

int main()
{
    std::cout << "\nRunning AutoCoolDown unit tests...\n\n";

    test_ready_when_never_used();
    test_ready_false_after_reset();
    test_remaining_leq_duration_after_reset();

    std::cout << "\n\u2713 ALL TESTS PASSED\n\n";
    return 0;
}
