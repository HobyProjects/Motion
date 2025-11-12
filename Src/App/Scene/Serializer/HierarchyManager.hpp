#pragma once

#include "SceneSerializer.hpp"

namespace Motion
{
    class HierarchyManager : public IHierarchyManager
    {
        public:
            HierarchyManager() = default;
            ~HierarchyManager() override = default;

            void BuildHierarchy(
                entt::registry& registry, 
                entt::entity root, 
                const std::vector<entt::entity>& children
            ) override
            {
                if (children.empty())
                {
                    MOTION_CORE_WARN("No children to build hierarchy for entity: {}", static_cast<uint32_t>(root));
                    return;
                }

                auto& rootHierarchy = registry.emplace_or_replace<HierarchyComponent>(root);
                rootHierarchy.Parent = entt::null;
                rootHierarchy.FirstChild = children.front();
                rootHierarchy.NextSibling = entt::null;

                for (size_t i = 0; i < children.size(); ++i)
                {
                    auto& childHierarchy = registry.emplace_or_replace<HierarchyComponent>(children[i]);
                    childHierarchy.Parent = root;
                    childHierarchy.FirstChild = entt::null;
                    childHierarchy.NextSibling = (i + 1 < children.size()) ? children[i + 1] : entt::null;
                }
            }

            std::vector<entt::entity> GetChildren(const entt::registry& registry, entt::entity root) const override
            {
                std::vector<entt::entity> children;

                const auto* rootHierarchy = registry.try_get<HierarchyComponent>(root);
                if (!rootHierarchy)
                    return children;

                for (entt::entity child = rootHierarchy->FirstChild; child != entt::null; )
                {
                    children.push_back(child);

                    const auto* childHierarchy = registry.try_get<HierarchyComponent>(child);
                    child = childHierarchy ? childHierarchy->NextSibling : entt::null;
                }

                return children;
            }

            bool IsRoot(const entt::registry& registry, entt::entity entity) const override
            {
                const auto* hierarchy = registry.try_get<HierarchyComponent>(entity);
                return hierarchy && (hierarchy->Parent == entt::null);
            }

            void DetachFromParent(entt::registry& registry, entt::entity entity)
            {
                auto* hierarchy = registry.try_get<HierarchyComponent>(entity);
                if (!hierarchy || hierarchy->Parent == entt::null)
                    return;

                entt::entity parent = hierarchy->Parent;
                auto* parentHierarchy = registry.try_get<HierarchyComponent>(parent);
                if (!parentHierarchy)
                    return;

                if (parentHierarchy->FirstChild == entity)
                {
                    parentHierarchy->FirstChild = hierarchy->NextSibling;
                }
                else
                {
                    for (entt::entity sibling = parentHierarchy->FirstChild; sibling != entt::null; )
                    {
                        auto* siblingHierarchy = registry.try_get<HierarchyComponent>(sibling);
                        if (siblingHierarchy && siblingHierarchy->NextSibling == entity)
                        {
                            siblingHierarchy->NextSibling = hierarchy->NextSibling;
                            break;
                        }
                        sibling = siblingHierarchy ? siblingHierarchy->NextSibling : entt::null;
                    }
                }

                hierarchy->Parent = entt::null;
                hierarchy->NextSibling = entt::null;
            }

            void AttachToParent(entt::registry& registry, entt::entity child, entt::entity parent)
            {
                DetachFromParent(registry, child);
                auto& childHierarchy = registry.get_or_emplace<HierarchyComponent>(child);
                auto& parentHierarchy = registry.get_or_emplace<HierarchyComponent>(parent);

                childHierarchy.Parent = parent;
                childHierarchy.NextSibling = parentHierarchy.FirstChild;
                parentHierarchy.FirstChild = child;
            }

            size_t GetChildCount(const entt::registry& registry, entt::entity entity) const
            {
                return GetChildren(registry, entity).size();
            }

            void TraverseDepthFirst(
                const entt::registry& registry,
                entt::entity root,
                const std::function<void(entt::entity, size_t depth)>& callback
            ) const
            {
                TraverseDepthFirstImpl(registry, root, callback, 0);
            }

        private:
            void TraverseDepthFirstImpl(
                const entt::registry& registry,
                entt::entity entity,
                const std::function<void(entt::entity, size_t depth)>& callback,
                size_t depth
            ) const
            {
                callback(entity, depth);

                const auto* hierarchy = registry.try_get<HierarchyComponent>(entity);
                if (!hierarchy)
                    return;

                for (entt::entity child = hierarchy->FirstChild; child != entt::null; )
                {
                    TraverseDepthFirstImpl(registry, child, callback, depth + 1);

                    const auto* childHierarchy = registry.try_get<HierarchyComponent>(child);
                    child = childHierarchy ? childHierarchy->NextSibling : entt::null;
                }
            }
    };

}