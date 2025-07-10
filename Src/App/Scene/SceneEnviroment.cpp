#include "CorePCH.hpp"
#include "SceneEnviroment.hpp"

namespace Motion::App
{
    void PhysicsWorld::Update(std::shared_ptr<Motion::Core::Entity> entity, float deltaTime)
    {
        Motion::Core::TransformComponent& transform = entity->GetComponent<Motion::Core::TransformComponent>();
        Motion::Core::PhysicsBodyComponent& body = entity->GetComponent<Motion::Core::PhysicsBodyComponent>();

        if(body.Type != Motion::Core::PhysicsBodyComponent::BodyType::Dynamic || !body.Active)
            return;

        EnviromentIntegration(transform, body, deltaTime);

        if (glm::length2(body.Velocity) < 0.00001f && glm::length2(body.ForceAccum) < 0.00001f)
        {
            body.Velocity = glm::vec3(0.0f);
            body.Active = false;
        }
    }

    void PhysicsWorld::EnviromentIntegration(Motion::Core::TransformComponent& transform, Motion::Core::PhysicsBodyComponent& body, float deltaTime)
    {
        glm::vec3 acceleration = m_Settings.Gravity + (body.ForceAccum / body.Mass);
        body.Velocity += acceleration * deltaTime;
        transform.Translation += body.Velocity * deltaTime;
        body.ForceAccum = glm::vec3(0.0f);
    }


}