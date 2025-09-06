#include "CorePCH.hpp"
#include "Scene.hpp"

namespace Motion
{
    Scene::Scene(const SceneSpecification& spec)
    {
        m_Specification = spec;
        m_Camera = SceneCamera(spec.Viewport.Size.x, spec.Viewport.Size.y, false);
    }

    void Scene::OnUpdate(WindowHandle handle, Timer deltaTime) noexcept
    {
        m_Camera.OnUpdate(handle, deltaTime);

#if 0
        const float dtSeconds = deltaTime.GetDeltaTimeSeconds();

        static double accumulator                   = 0.0;
        static constexpr double dtPhysics           = 1.0 / 120.0;
        static constexpr std::int32_t maxSteps      = 8;

        accumulator += static_cast<double>(dtSeconds);

        std::int32_t steps{0};
        PhyX& phy = PhyX::GetInstance();

        while(accumulator >= dtPhysics && steps < maxSteps)
        {
            phy.Setp(m_Entities, static_cast<float>(dtPhysics));
            accumulator -= dtPhysics;
            ++steps;
        }

#endif

    }

    void Scene::OnEvent(WindowHandle handle, IEvent& e) noexcept
    {
        m_Camera.OnEvents(handle, e);
    }

    void Scene::OnViewportSizeChanges(const glm::vec2& size) noexcept
    {
        m_Camera.SetAspectRatio(size.x, size.y);
    }

    void Scene::SelectEntityIf()
    {
        if (m_SelectedEntity != EntityFactory::EMPTYENTITY && !m_Entities.empty())
            m_SelectedEntity = m_Entities.front();
        else
            m_SelectedEntity = EntityFactory::EMPTYENTITY;
    }

    std::shared_ptr<Entity> Scene::PickEntity(const glm::vec2& mousePos, const glm::vec2& viewportSize)
    {
        const glm::mat4& projection = m_Camera.Camera.Projection;
        const glm::mat4& view = m_Camera.Camera.View;

        // 1) Screen -> NDC
        float x = (2.0f * mousePos.x) / viewportSize.x - 1.0f;
        float y = 1.0f - (2.0f * mousePos.y) / viewportSize.y; // GL Y is inverted

        // 2) NDC -> world ray
        glm::vec4 rayStartNDC(x, y, -1.0f, 1.0f);
        glm::vec4 rayEndNDC(x, y, 1.0f, 1.0f);

        glm::mat4 invVP = glm::inverse(projection * view);
        glm::vec4 rayStartWorld = invVP * rayStartNDC; rayStartWorld /= rayStartWorld.w;
        glm::vec4 rayEndWorld = invVP * rayEndNDC;   rayEndWorld /= rayEndWorld.w;

        glm::vec3 rayOrigin = glm::vec3(rayStartWorld);
        glm::vec3 rayDir = glm::normalize(glm::vec3(rayEndWorld - rayStartWorld));

        // 3) Find closest entity hit by ray
        float closestT = std::numeric_limits<float>::infinity();
        std::shared_ptr<Entity> pickedEntity = nullptr;

        for (const auto& entity : m_Entities)
        {
            if (!entity->HasComponent<StaticMeshComponent>() || !entity->HasComponent<TransformComponent>())
                continue;

            auto& meshComp = entity->GetComponent<StaticMeshComponent>();
            auto& transComp = entity->GetComponent<TransformComponent>();
            if (!meshComp.Model) continue;

            // Local-space AABB from model
            const glm::vec3 localMin = meshComp.Model->GetMinBounds();
            const glm::vec3 localMax = meshComp.Model->GetMaxBounds();

            // Build world-space AABB by transforming all 8 corners
            const glm::mat4 model = transComp.GetTransform();

            const glm::vec3 corners[8] =
            {
                {localMin.x, localMin.y, localMin.z},
                {localMax.x, localMin.y, localMin.z},
                {localMin.x, localMax.y, localMin.z},
                {localMax.x, localMax.y, localMin.z},
                {localMin.x, localMin.y, localMax.z},
                {localMax.x, localMin.y, localMax.z},
                {localMin.x, localMax.y, localMax.z},
                {localMax.x, localMax.y, localMax.z}
            };

            glm::vec3 worldMin(std::numeric_limits<float>::max());
            glm::vec3 worldMax(-std::numeric_limits<float>::max());
            for (int i = 0; i < 8; ++i)
            {
                glm::vec3 w = glm::vec3(model * glm::vec4(corners[i], 1.0f));
                worldMin = glm::min(worldMin, w);
                worldMax = glm::max(worldMax, w);
            }

            float tmin, tmax;
            if (RayIntersectsAABB(rayOrigin, rayDir, worldMin, worldMax, tmin, tmax))
            {
                float hitDist = (tmin > 0.0f) ? tmin : tmax; // prefer the front hit
                if (hitDist > 0.0f && hitDist < closestT)
                {
                    closestT = hitDist;
                    pickedEntity = entity;
                }
            }
        }

        return pickedEntity;
    }

    std::shared_ptr<Entity> Scene::PickEntityRay(const glm::vec3& origin, const glm::vec3& dir, float maxDist)
    {
        Ray ray; 
        ray.Origin      = origin; 
        ray.Direction   = glm::normalize(dir); 
        ray.MaxDistance = maxDist;

        // If you might be calling this outside your physics Step, ensure AABBs are fresh
        // (optional) recompute worldAABB for dirty transforms here similar to Broadphase()

        auto hit = RayCast(m_Entities, ray);
        return hit.Hit ? hit.EnTT : nullptr;
    }
}