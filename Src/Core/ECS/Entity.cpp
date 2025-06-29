#include "CorePCH.hpp"

namespace Motion::Core
{
    entt::registry EntityBuilder::Registry;
	std::shared_ptr<Entity> EntityBuilder::ENULL = EntityBuilder::CreateEntity("Empty Entity");

	std::shared_ptr<Entity> EntityBuilder::CreateEntity(const std::string& name)
	{
		std::shared_ptr<Entity> entity = std::make_shared<Entity>(Registry.create());
		auto& tag = entity->AddComponent<TagComponent>(name);
		tag.Tag = name.empty() ? "unnamed" : name;
		entity->AddComponent<TransformComponent>();
		entity->AddComponent<MeshComponent>();
		return entity;
	}

	void EntityBuilder::DestroyEntity(const std::shared_ptr<Entity>& entity)
	{
		auto handle = entity->GetHandle();
		Registry.destroy(handle);
	}

	std::shared_ptr<Entity> EntityBuilder::Empty()
	{
		return ENULL;
	}
}