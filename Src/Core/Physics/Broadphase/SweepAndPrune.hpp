#pragma once

#include <vector>
#include <algorithm>
#include <utility>
#include <cstdint>

#include "IBroadphase.hpp"
#include "AABB.hpp"

namespace Motion
{
    struct SweepAndPrune final : IBroadPhase
    {
        using INDEX = std::uint32_t;

        struct Proxy
        {
            ProxyID ID{0};
            AABB Body{};
            std::uint32_t Layer{0};
            void* UserPointer{nullptr};
            INDEX StartIndex{std::numeric_limits<std::uint32_t>::max()};
            INDEX EndIndex{std::numeric_limits<std::uint32_t>::max()};
            bool Active{true};
        };

        struct EndPoint
        {
            float Value{0.0f};
            INDEX ProxyIndex{0};
            bool Start{false};
        };

        std::vector<Proxy> Proxies{};
        std::vector<EndPoint> EndPoints{};
        ProxyID NextID{1};

        bool Dirty{false};

        void Resort() 
        {
            std::stable_sort(EndPoints.begin(), EndPoints.end(), [](const EndPoint& a, const EndPoint& b){ return a.Value < b.Value; });

            for (INDEX idx = 0; idx < (INDEX)EndPoints.size(); ++idx) 
            {
                Proxy& P = Proxies[EndPoints[idx].ProxyIndex];
                if (EndPoints[idx].Start)   P.StartIndex = idx;
                else                        P.EndIndex   = idx;
            }

            Dirty = false;
        }

        INDEX FindProxyIndex(ProxyID id) const 
        {
            for (INDEX i = 0; i < (INDEX)Proxies.size(); ++i)
                if (Proxies[i].ID == id) return i;

            return std::numeric_limits<std::uint32_t>::max();
        }

        void RemoveEndpointAt(INDEX idx) 
        {
            const INDEX last = (INDEX)EndPoints.size() - 1;
            if (idx != last) 
            {
                EndPoint moved = EndPoints[last];
                EndPoints[idx] = moved;

                Proxy& P = Proxies[moved.ProxyIndex];
                if (moved.Start)   P.StartIndex = idx;
                else               P.EndIndex   = idx;
            }

            EndPoints.pop_back();
        }

        ProxyID CreateProxy(const AABB& aabb, uint32_t layer, void* userPtr) override 
        {
            Proxy p; p.ID = NextID++; p.Body = aabb; p.Layer = layer; p.UserPointer = userPtr; p.Active = true;
            INDEX proxyIdx = (INDEX)Proxies.size();
            Proxies.push_back(p);

            // Add endpoints (start <= end)
            const float s = aabb.MIN.x;
            const float e = aabb.MAX.x;
            INDEX si = (INDEX)EndPoints.size();
            INDEX ei = si + 1;
            EndPoints.push_back(EndPoint{ s, proxyIdx, true  });
            EndPoints.push_back(EndPoint{ e, proxyIdx, false });

            Proxies[proxyIdx].StartIndex = si;
            Proxies[proxyIdx].EndIndex   = ei;

            Dirty = true;
            return Proxies[proxyIdx].ID;
        }

        void MoveProxy(ProxyID id, const AABB& aabb) override 
        {
            const INDEX i = FindProxyIndex(id);
            if (i == std::numeric_limits<std::uint32_t>::max()) return;
            Proxies[i].Body = aabb;

            // Update X endpoints
            EndPoints[Proxies[i].StartIndex].Value = aabb.MIN.x;
            EndPoints[Proxies[i].EndIndex  ].Value = aabb.MAX.x;
            Dirty = true;
        }

        void DestroyProxy(ProxyID id) override 
        {
            const INDEX i = FindProxyIndex(id);
            if (i == std::numeric_limits<std::uint32_t>::max()) return;

            const INDEX si = Proxies[i].StartIndex;
            const INDEX ei = Proxies[i].EndIndex;

            RemoveEndpointAt(ei); 
            RemoveEndpointAt(si);

            const INDEX last = (INDEX)Proxies.size() - 1;
            if (i != last) 
            {
                Proxy moved = Proxies[last];
                Proxies[i] = moved;

                EndPoints[Proxies[i].StartIndex].ProxyIndex = i;
                EndPoints[Proxies[i].EndIndex  ].ProxyIndex = i;
            }

            Proxies.pop_back();
            Dirty = true;
        }

        void QueryOverlaps(std::vector<std::pair<void*, void*>>& outPairs) override 
        {
            outPairs.clear();
            if (EndPoints.empty()) return;
            if (Dirty) Resort();

            std::vector<INDEX> active;
            active.reserve(128);

            for (const EndPoint& ep : EndPoints) 
            {
                const INDEX pi = ep.ProxyIndex;
                const Proxy& P = Proxies[pi];
                if (!P.Active) continue;

                if (ep.Start) 
                {
                    for (INDEX aj : active) 
                    {
                        const Proxy& Q = Proxies[aj];
                        if (!(P.Layer & Q.Layer)) continue;

                        if (OverlapYZ(P.Body, Q.Body)) 
                        {
                            if (Overlap(P.Body, Q.Body)) 
                            {
                                void* aPtr = P.UserPointer;
                                void* bPtr = Q.UserPointer;

                                if (aPtr < bPtr) outPairs.emplace_back(aPtr, bPtr);
                                else             outPairs.emplace_back(bPtr, aPtr);
                            }
                        }
                    }
                    active.push_back(pi);
                } 
                else 
                {
                    for (size_t k = 0; k < active.size(); ++k) 
                    {
                        if (active[k] == pi) { active[k] = active.back(); active.pop_back(); break; }
                    }
                }
            }

            std::sort(outPairs.begin(), outPairs.end(), [](auto& a, auto& b){ return a.first == b.first ? a.second < b.second : a.first < b.first; });
            outPairs.erase(std::unique(outPairs.begin(), outPairs.end(), [](auto& a, auto& b){ return a.first == b.first && a.second == b.second; }), outPairs.end());
        }
    };
}