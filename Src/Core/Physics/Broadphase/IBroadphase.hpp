#pragma once

#include <vector>
#include <utility>

#include "Types.hpp"

namespace Motion
{
    using ProxyID = std::uint32_t;

    struct BroadphaseProxy
    {
        ProxyID ID{0};
        AABB    FatAABB{};
        std::uint32_t Layer{};
    };

    struct IBroadPhase
    {
        IBroadPhase() = default;
        virtual ~IBroadPhase() = default;

        virtual ProxyID CreateProxy(const AABB& aabb, std::uint32_t layer, void* userPtr) = 0;
        virtual void MoveProxy(ProxyID id, const AABB& aabb) = 0;
        virtual void DestroyProxy(ProxyID id) = 0;
        virtual void QueryOverlaps(std::vector<std::pair<void* ,void*>>& outPairs) = 0;
    };
}