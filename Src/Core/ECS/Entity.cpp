#include "CorePCH.hpp"

namespace Motion
{
    std::shared_ptr<Entity> Entity::Create(const std::string& name) noexcept
    {
        std::shared_ptr<Entity> entity  = std::make_shared<Entity>(m_EntityRegistry.create());
        auto& tag                       = entity->Emplace<TagComponent>(name);
        auto& node                      = entity->Emplace<NodeComponent>();
        tag.Tag                         = name.empty() ? "unnamed" : name;

        return entity;
    }

    void Entity::Destroy(const std::shared_ptr<Entity>& entity)
    {
        m_EntityRegistry.destroy(entity->Handle());
    }

    std::shared_ptr<Entity> Entity::Empty()
    {
        static std::shared_ptr<Entity> entity = Create("null");
        return entity;
    }
}