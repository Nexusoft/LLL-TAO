/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLC_INCLUDE_QTV_ENGINE_H
#define NEXUS_LLC_INCLUDE_QTV_ENGINE_H

#include <LLC/include/qtv_julia_bridge.h>

namespace LLC
{
namespace QTV
{
    class IQTVEngine
    {
    public:
        virtual ~IQTVEngine() = default;
        virtual bool RunFixture(int caseId) = 0;
        virtual bool CompareParity(int caseId) = 0;
    };

    class NullQTVEngine final : public IQTVEngine
    {
    public:
        bool RunFixture(int) override
        {
            return false;
        }

        bool CompareParity(int) override
        {
            return false;
        }
    };

    class JuliaQTVEngine final : public IQTVEngine
    {
    public:
        explicit JuliaQTVEngine(QTVJuliaBridge bridge = QTVJuliaBridge()) noexcept
        : bridge_(bridge)
        {
        }

        bool Available() const noexcept
        {
            return bridge_.available();
        }

        bool RunFixture(int caseId) override
        {
            return bridge_.run_fixture(caseId) == HOOK_STATUS_OK;
        }

        bool CompareParity(int caseId) override
        {
            return bridge_.compare_parity(caseId) == HOOK_STATUS_OK;
        }

    private:
        QTVJuliaBridge bridge_;
    };

    class CppQTVEngine final : public IQTVEngine
    {
    public:
        using Operation = bool (*)(int);

        CppQTVEngine(Operation runFixture = nullptr, Operation compareParity = nullptr) noexcept
        : run_fixture_(runFixture)
        , compare_parity_(compareParity)
        {
        }

        bool Available() const noexcept
        {
            return run_fixture_ != nullptr || compare_parity_ != nullptr;
        }

        bool RunFixture(int caseId) override
        {
            return run_fixture_ != nullptr && run_fixture_(caseId);
        }

        bool CompareParity(int caseId) override
        {
            return compare_parity_ != nullptr && compare_parity_(caseId);
        }

    private:
        Operation run_fixture_;
        Operation compare_parity_;
    };
} // namespace QTV
} // namespace LLC

#endif
