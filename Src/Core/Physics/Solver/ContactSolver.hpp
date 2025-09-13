#pragma once

#include <vector>
#include <cmath>

#include "RigidBody.hpp"
#include "Contact.hpp"

namespace Motion 
{
    struct ContactConstraint 
    {
        std::int32_t A{-1}, B{-1};

        glm::vec3 P{0.0f};        
        glm::vec3 N{0.0f};        
        glm::vec3 T1{0.0f}, T2{0.0f};   
        glm::vec3 RA{0.0f}, RB{0.0f};

        float MassN{0.0f};
        float MassT1{0.0f};
        float MassT2{0.0f};

        float Restitution{0.0f};
        float Friction{0.6f};
        float Bias{0.0f};

        float  ImpulseN{0.0f};
        float  ImpulseT1{0.0f};
        float  ImpulseT2{0.0f};

        float vRelN0{0.0f};
    };

    struct SolverSettings 
    {
        int   Iterations                = 12;
        float Baumgarte                 = 0.2f;    
        float AllowedPenetration        = 0.01f;
        float RestitutionThreshold      = 1.0f; 
        bool  WarmStart                 = true;
    };

    inline void OrthonormalBasis(const glm::vec3& n, glm::vec3& t1, glm::vec3& t2) 
    {
        glm::vec3 h = (std::abs(n.x) < 0.577f) ? glm::vec3(1,0,0) :
                      (std::abs(n.y) < 0.577f) ? glm::vec3(0,1,0) : glm::vec3(0,0,1);

        t1 = glm::normalize(glm::cross(h, n));
        t2 = glm::cross(n, t1);
    }

    inline float EffectiveMass(const RigidBody& A, const RigidBody& B, const glm::vec3& rA, const glm::vec3& rB, const glm::vec3& j)
    {
        float k =
            A.InvMass + B.InvMass +
            glm::dot( j, glm::cross(A.InvInertiaWorld * glm::cross(rA, j), rA) ) +
            glm::dot( j, glm::cross(B.InvInertiaWorld * glm::cross(rB, j), rB) );

        return (k > 0.0f) ? 1.0f / k : 0.0f;
    }

    struct ContactBatch 
    {
        std::vector<ContactConstraint> CONS;

        void Build(const std::vector<ContactManifold>& manifolds, const std::vector<std::pair<int,int>>& bodyPairs, const std::vector<RigidBody>& bodies, const SolverSettings& settings)
        {
            CONS.clear();
            CONS.reserve(manifolds.size() * 2);

            for (size_t m = 0; m < manifolds.size(); ++m) 
            {
                const auto& M   = manifolds[m];
                const int ia    = bodyPairs[m].first;
                const int ib    = bodyPairs[m].second;

                const RigidBody& A = bodies[ia];
                const RigidBody& B = bodies[ib];

                for (int i = 0; i < M.Count; ++i) 
                {
                    ContactConstraint c{};
                    c.A = ia; c.B = ib;

                    c.P = M.Points[i].PositionWS;
                    c.N = glm::normalize(M.Points[i].NormalWS);

                    OrthonormalBasis(c.N, c.T1, c.T2);

                    c.RA = c.P - A.Position;
                    c.RB = c.P - B.Position;

                    c.MassN  = EffectiveMass(A, B, c.RA, c.RB, c.N);
                    c.MassT1 = EffectiveMass(A, B, c.RA, c.RB, c.T1);
                    c.MassT2 = EffectiveMass(A, B, c.RA, c.RB, c.T2);

                    c.Friction    = 0.5f * (/*A.mat.friction*/ 0.6f + /*B.mat.friction*/ 0.6f);
                    c.Restitution = 0.5f * (/*A.mat.restitution*/0.1f + /*B.mat.restitution*/0.1f);

                    const float pen         = M.Points[i].Penetration;
                    float penetrationError  = glm::max(0.0f, pen - settings.AllowedPenetration);
                    c.Bias = (settings.Baumgarte / (float)settings.Iterations) * penetrationError;

                    // Initial normal relative velocity for restitution decision
                    glm::vec3 vA = A.LinearVelocity + glm::cross(A.AngularVelocity, c.RA);
                    glm::vec3 vB = B.LinearVelocity + glm::cross(B.AngularVelocity, c.RB);
                    c.vRelN0 = glm::dot(c.N, vB - vA);

                    CONS.push_back(c);
                }
            }
        }

        void WarmStart(std::vector<RigidBody>& bodies, const SolverSettings& settings) 
        {
            if (!settings.WarmStart) return;
            for (auto& c : CONS) {
                RigidBody& A = bodies[c.A];
                RigidBody& B = bodies[c.B];
                const glm::vec3 P = c.N * c.ImpulseN + c.T1 * c.ImpulseT1 + c.T2 * c.ImpulseT2;

                A.ApplyLinearImpulse(-P);
                A.ApplyAngularImpulse(-glm::cross(c.RA, P));

                B.ApplyLinearImpulse(P);
                B.ApplyAngularImpulse(glm::cross(c.RB, P));
            }
        }

        void Solve(std::vector<RigidBody>& bodies, const SolverSettings& settings) 
        {
            const int iters = settings.Iterations;
            for (int it = 0; it < iters; ++it) 
            {
                for (auto& c : CONS) {
                    RigidBody& A = bodies[c.A];
                    RigidBody& B = bodies[c.B];

                    glm::vec3 vA = A.LinearVelocity + glm::cross(A.AngularVelocity, c.RA);
                    glm::vec3 vB = B.LinearVelocity + glm::cross(B.AngularVelocity, c.RB);
                    glm::vec3 vRel = vB - vA;

                    float vRelN = glm::dot(c.N, vRel);

                    float bounce = 0.0f;
                    if (-vRelN > settings.RestitutionThreshold)
                        bounce = c.Restitution * (-vRelN);

                    float lambdaN = c.MassN * ( -(vRelN - bounce) + c.Bias );

                    float oldN  = c.ImpulseN;
                    c.ImpulseN  = glm::max(oldN + lambdaN, 0.0f);
                    lambdaN     = c.ImpulseN - oldN;

                    glm::vec3 PN = c.N * lambdaN;
                    A.ApplyLinearImpulse(-PN);
                    A.ApplyAngularImpulse(-glm::cross(c.RA, PN));
                    B.ApplyLinearImpulse(PN);
                    B.ApplyAngularImpulse(glm::cross(c.RB, PN));

                    vA      = A.LinearVelocity + glm::cross(A.AngularVelocity, c.RA);
                    vB      = B.LinearVelocity + glm::cross(B.AngularVelocity, c.RB);
                    vRel    = vB - vA;

                    float vT1 = glm::dot(c.T1, vRel);
                    float vT2 = glm::dot(c.T2, vRel);

                    float lambdaT1 = -c.MassT1 * vT1;
                    float lambdaT2 = -c.MassT2 * vT2;

                    float oldT1 = c.ImpulseT1, oldT2 = c.ImpulseT2;
                    float newT1 = oldT1 + lambdaT1;
                    float newT2 = oldT2 + lambdaT2;

                    float maxFriction = c.Friction * c.ImpulseN;
                    // Project (newT1,newT2) into disc of radius maxFriction
                    float mag = std::sqrt(newT1*newT1 + newT2*newT2);
                    if (mag > maxFriction + 1e-6f) {
                        float scale = maxFriction / (mag + 1e-12f);
                        newT1 *= scale; newT2 *= scale;
                    }
                    lambdaT1 = newT1 - oldT1;
                    lambdaT2 = newT2 - oldT2;
                    c.ImpulseT1 = newT1;
                    c.ImpulseT2 = newT2;

                    glm::vec3 PT = c.T1 * lambdaT1 + c.T2 * lambdaT2;
                    A.ApplyLinearImpulse(-PT);
                    A.ApplyAngularImpulse(-glm::cross(c.RA, PT));
                    B.ApplyLinearImpulse(PT);
                    B.ApplyAngularImpulse(glm::cross(c.RB, PT));
                }
            }
        }
    };

} 
