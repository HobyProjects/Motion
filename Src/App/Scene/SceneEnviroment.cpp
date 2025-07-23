#include "CorePCH.hpp"
#include "SceneEnviroment.hpp"

namespace Motion
{
    void PhysicsWorld::Update(std::shared_ptr<Entity> entity, float deltaTime)
    {
        TransformComponent& transform = entity->GetComponent<TransformComponent>();
        PhysicsBodyComponent& body = entity->GetComponent<PhysicsBodyComponent>();

        if (body.Type != PhysicsBodyComponent::BodyType::Dynamic || !body.Active)
            return;

        EnvironmentIntegration(transform, body, deltaTime);

        if (glm::length2(body.Velocity) < 0.00001f && glm::length2(body.ForceAccum) < 0.00001f)
        {
            body.Velocity = glm::vec3(0.0f);
            body.Active = false;
        }
    }

    void PhysicsWorld::EnvironmentIntegration(TransformComponent& transform, PhysicsBodyComponent& body, float deltaTime)
    {
        glm::vec3 acceleration = m_Settings.Gravity + (body.ForceAccum / body.Mass);
        body.Velocity += acceleration * deltaTime;
        transform.Translation += body.Velocity * deltaTime;
        body.ForceAccum = glm::vec3(0.0f);
    }


}