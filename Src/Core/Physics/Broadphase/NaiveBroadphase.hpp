#pragma once

#include <vector>
#include <utility>

#include "IBroadphase.hpp"
#include "AABB.hpp"

namespace Motion
{
    struct NaiveBroadPhase final : IBroadPhase
    {
        struct Node
        {
            ProxyID ID{0};
            AABB FAT{};
            void* UserPointer{nullptr};
            std::uint32_t Layer{0};
        };

        std::vector<Node> Nodes{};
        ProxyID NextID{1};

        ProxyID CreateProxy(const AABB& aabb, std::uint32_t layer, void* userPtr) override
        {
            Nodes.push_back({NextID, aabb, userPtr, layer});
            return NextID++;
        }

        void MoveProxy(ProxyID id, const AABB& aabb) override
        {
            for(auto& n : Nodes) if(n.ID == id) { n.FAT = aabb;  return; }
        }

        void DestroyProxy(ProxyID id) override 
        {
            for (size_t i = 0;i < Nodes.size();++i)
            {
                if (Nodes[i].ID == id) 
                { 
                    Nodes.erase(Nodes.begin()+i); 
                    return; 
                }
            }
        }

        void QueryOverlaps(std::vector<std::pair<void*, void*>>& outPairs) override 
        {
            outPairs.clear();
            for (size_t i = 0; i < Nodes.size();++i)
                for (size_t j = i + 1; j < Nodes.size();++j)
                    if (Overlap(Nodes[i].FAT, Nodes[j].FAT) && (Nodes[i].Layer & Nodes[j].Layer))
                        outPairs.emplace_back(Nodes[i].UserPointer, Nodes[j].UserPointer);
        }
    };
}