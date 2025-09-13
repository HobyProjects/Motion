#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

namespace Motion 
{
    struct RigidBody 
    {
        glm::vec3 Position{0};
        glm::quat Rotation{1,0,0,0};
        glm::vec3 LinearVelocity{0};
        glm::vec3 AngularVelocity{0};

        float InvMass = 0.0f;       
        glm::mat3 InvInertiaLocal{0}; 
        glm::mat3 InvInertiaWorld{0}; 

        float LinearDamping  = 0.01f;
        float AngularDamping = 0.05f;

        bool CanSleep{true};
        bool Sleeping{false};
        float SleepTimer{0.0f};

        float SleepLinearThreshold{0.05f};
        float SleepAngularThreshold{0.05f};
        float SleepTime{0.5f};

        void WakeUp()
        {
            if(!Sleeping) return;
            Sleeping    = false;
            SleepTimer  = 0.0f;
        }

        void PutToSleep()
        {
            if(!CanSleep) return;
            
            Sleeping            = true;
            SleepTimer          = 0.0f;
            LinearVelocity      = glm::vec3(0.0f);
            AngularVelocity     = glm::vec3(0.0f);
        }

        glm::mat4 WorldTransform() const 
        {
            return glm::translate(glm::mat4(1.0f), Position) * glm::toMat4(Rotation);
        }

        void SyncInertiaWorld() 
        {
            const glm::mat3 R = glm::toMat3(Rotation);
            InvInertiaWorld = R * InvInertiaLocal * glm::transpose(R);
        }

        inline void ApplyLinearImpulse(const glm::vec3& P) 
        {
            LinearVelocity += P * InvMass;
        }

        inline void ApplyAngularImpulse(const glm::vec3& L) 
        {
            AngularVelocity += InvInertiaWorld * L;
        }

        void Integrate(float dt) 
        {
            LinearVelocity  *= glm::clamp(1.0f - LinearDamping, 0.0f, 1.0f);
            AngularVelocity *= glm::clamp(1.0f - AngularDamping, 0.0f, 1.0f);

            Position += LinearVelocity * dt;
            Rotation  = glm::normalize(glm::quat(1,0,0,0) + 0.5f * glm::quat(0, AngularVelocity) * Rotation * dt);
        }
    };

} 
