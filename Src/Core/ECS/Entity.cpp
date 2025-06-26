#include "CorePCH.hpp"

namespace Motion::Core
{
    entt::registry EntityBuilder::Registry;
	Entity EntityBuilder::ENULL = EntityBuilder::CreateEntity("Empty Entity");

	Entity EntityBuilder::CreateEntity(const std::string& name)
	{
		Entity entity{ Registry.create() };
		auto& tag = entity.AddComponent<TagComponent>(name);
		tag.Tag = name.empty() ? "unnamed" : name;
		entity.AddComponent<TransformComponent>();
		return entity;
	}

	void EntityBuilder::DestroyEntity(Entity entity)
	{
		Registry.destroy(entity);
	}

	Entity EntityBuilder::Empty()
	{
		return Entity();
	}
}