#pragma once

#include <vector>
#include <unordered_map>
#include <cstdint>
#include <entt/entt.hpp>
#include <glm/glm.hpp>

#include "Contacts.hpp"
#include "Entity.hpp"

namespace Motion
{
    class PhyX
    {
        public:
            [[nodiscard]] static PhyX& GetInstance()
            {
                static PhyX instance;
                return instance;
            }

            void Setp(const std::vector<std::shared_ptr<Entity>>& e, float deltaTime);
            void SetGravity(const glm::vec3& g) { m_Gravity = g; }
            const glm::vec3& Gravity() const { return m_Gravity; }

        private:
            PhyX() = default;
            ~PhyX() = default;

            void IntegrateForces(const std::vector<std::shared_ptr<Entity>>& e, float deltaTime);
            void Broadphase(const std::vector<std::shared_ptr<Entity>>& e);
            void Narrowphase();
            void SolveContacts(float deltaTime);
            void IntegrateVelocities(const std::vector<std::shared_ptr<Entity>>& e, float deltaTime);
            void ClearAccumulators(const std::vector<std::shared_ptr<Entity>>& e);

        private:
            glm::vec3 m_Gravity{0.0f, -9.80665f, 0.0f};
            std::vector<std::pair<std::shared_ptr<Entity>, std::shared_ptr<Entity>>> m_Pairs{};
            std::vector<Contact> m_Contacts{};
            
    };
}