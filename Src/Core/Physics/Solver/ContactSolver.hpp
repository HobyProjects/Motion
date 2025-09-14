#pragma once

#include <vector>
#include <cmath>

#include "RigidBody.hpp"
#include "Contact.hpp"
#include "Pairwise.hpp"

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

        PairKey Key{};        
        int     PointCount{0};

        struct Pt 
        {
            float AccumNormal{0.0f};
            float AccumTanU{0.0f};
            float AccumTanV{0.0f};
            glm::vec3 PositionWS{0.0f};
            glm::vec3 NormalWS{0.0f};

        } Points[4];
    };

    struct SolverSettings 
    {
        int   Iterations                = 12;
        float Baumgarte                 = 0.2f;    
        float AllowedPenetration        = 0.01f;
        float RestitutionThreshold      = 1.0f; 
        bool  WarmStart                 = true;
    };

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
        std::vector<ContactConstraint> Constraints;
        const ManifoldCache* Cache{nullptr};

        void Build(const std::vector<ContactManifold>& manifolds, const std::vector<std::pair<int,int>>& bodyPairs, const std::vector<RigidBody>& bodies, const SolverSettings& settings)
        {
            Constraints.clear();
            Constraints.reserve(manifolds.size() * 2);

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

                    c.Friction    = 0.5f * M.SharedFriction;
                    c.Restitution = 0.5f * M.SharedRestitution;

                    const float pen         = M.Points[i].Penetration;
                    float penetrationError  = glm::max(0.0f, pen - settings.AllowedPenetration);
                    c.Bias = (settings.Baumgarte / (float)settings.Iterations) * penetrationError;

                    glm::vec3 vA    = A.LinearVelocity + glm::cross(A.AngularVelocity, c.RA);
                    glm::vec3 vB    = B.LinearVelocity + glm::cross(B.AngularVelocity, c.RB);
                    c.vRelN0        = glm::dot(c.N, vB - vA);

                    c.Key = { (uint32_t)std::min(ia, ib), (uint32_t)std::max(ia, ib) };
                    c.PointCount = manifolds[i].Count;

                    for (int k = 0; k < c.PointCount; ++k) 
                    {
                        c.Points[k].AccumNormal = 0.0f;
                        c.Points[k].AccumTanU   = 0.0f;
                        c.Points[k].AccumTanV   = 0.0f;

                        c.Points[k].PositionWS  = manifolds[i].Points[k].PositionWS;
                        c.Points[k].NormalWS    = manifolds[i].Points[k].NormalWS;
                    }

                    Constraints.push_back(c);
                }
            }
        }

        void Build(const std::vector<ContactManifold>& manifolds, const std::vector<std::pair<int,int>>& bodyPairs, std::vector<RigidBody>& bodies, const SolverSettings& settings, const ManifoldCache* cachePtr)
        {
            Cache = cachePtr;
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

                    c.Friction    = 0.5f * M.SharedFriction;
                    c.Restitution = 0.5f * M.SharedRestitution;

                    const float pen         = M.Points[i].Penetration;
                    float penetrationError  = glm::max(0.0f, pen - settings.AllowedPenetration);
                    c.Bias = (settings.Baumgarte / (float)settings.Iterations) * penetrationError;

                    glm::vec3 vA    = A.LinearVelocity + glm::cross(A.AngularVelocity, c.RA);
                    glm::vec3 vB    = B.LinearVelocity + glm::cross(B.AngularVelocity, c.RB);
                    c.vRelN0        = glm::dot(c.N, vB - vA);

                    c.Key = { (uint32_t)std::min(ia, ib), (uint32_t)std::max(ia, ib) };
                    c.PointCount = manifolds[i].Count;

                    for (int k = 0; k < c.PointCount; ++k) 
                    {
                        c.Points[k].AccumNormal = 0.0f;
                        c.Points[k].AccumTanU   = 0.0f;
                        c.Points[k].AccumTanV   = 0.0f;

                        c.Points[k].PositionWS  = manifolds[i].Points[k].PositionWS;
                        c.Points[k].NormalWS    = manifolds[i].Points[k].NormalWS;
                    }

                    Constraints.push_back(c);
                }
            }
        }

        void WarmStart(std::vector<RigidBody>& bodies, const SolverSettings& settings) 
        {
            if (!settings.WarmStart) return;
            for (auto& c : Constraints) {
                RigidBody& A = bodies[c.A];
                RigidBody& B = bodies[c.B];
                const glm::vec3 P = c.N * c.ImpulseN + c.T1 * c.ImpulseT1 + c.T2 * c.ImpulseT2;

                A.ApplyLinearImpulse(-P);
                A.ApplyAngularImpulse(-glm::cross(c.RA, P));

                B.ApplyLinearImpulse(P);
                B.ApplyAngularImpulse(glm::cross(c.RB, P));
            }
        }

        void ApplyImpulseToBodies(std::vector<RigidBody>& bodies, const ContactConstraint& C, int idx, float accumN, float accumU, float accumV)
        {
            RigidBody& A = bodies[C.A];
            RigidBody& B = bodies[C.B];
            glm::vec3 P = C.N * accumN + C.T1 * accumU + C.T2 * accumV;

            A.ApplyLinearImpulse(-P);
            A.ApplyAngularImpulse(-glm::cross(C.RA, P));

            B.ApplyLinearImpulse(P);
            B.ApplyAngularImpulse(glm::cross(C.RB, P));
        }

        void WarmStartCached(std::vector<RigidBody>& bodies, const SolverSettings& set)
        {
            if (!Cache) return;
            for (auto& C : Constraints) 
            {
                auto it = Cache->find(C.Key);
                if (it == Cache->end()) continue;
                const PersistentManifold& pm = it->second;

                bool usedPM[4]{ false, false, false, false};
                for (int i = 0; i < C.PointCount; ++i)
                {
                    int best = -1; float bestD2 = FLT_MAX;
                    for (int j=0; j<pm.Count; ++j) 
                    {
                        if (usedPM[j]) continue;
                        float c = glm::dot(C.Points[i].NormalWS, pm.P[j].CP.NormalWS);

                        if (c < 0.95f) continue;
                        float d2 = glm::length2(C.Points[i].PositionWS - pm.P[j].CP.PositionWS);

                        if (d2 < bestD2) { bestD2 = d2; best = j; }
                    }

                    if (best == -1) continue;
                    usedPM[best] = true;

                    const ContactWarm& w = pm.P[best].Warm;
                    C.Points[i].AccumNormal = w.NormalImpulse;
                    C.Points[i].AccumTanU   = w.TangentImpulseU;
                    C.Points[i].AccumTanV   = w.TangentImpulseV;

                    ApplyImpulseToBodies(bodies, C, i, C.Points[i].AccumNormal, C.Points[i].AccumTanU, C.Points[i].AccumTanV);
                }
            }
        }

        void WriteBackToCache()
        {
            if (!Cache) return;
            for (auto& C : Constraints)
            {
                auto it = Cache->find(C.Key);
                if (it == Cache->end()) continue;
                PersistentManifold& pm = const_cast<PersistentManifold&>(it->second);

                bool usedPM[4]{ false, false, false, false};
                for (int i=0; i<C.PointCount; ++i)
                {
                    int best = -1; float bestD2 = FLT_MAX;
                    for (int j=0; j<pm.Count; ++j) 
                    {
                        if (usedPM[j]) continue;
                        float c = glm::dot(C.Points[i].NormalWS, pm.P[j].CP.NormalWS);

                        if (c < 0.95f) continue;
                        float d2 = glm::length2(C.Points[i].PositionWS - pm.P[j].CP.PositionWS);

                        if (d2 < bestD2) { bestD2 = d2; best = j; }
                    }

                    if (best == -1) continue;
                    usedPM[best] = true;

                    pm.P[best].Warm.NormalImpulse       = C.Points[i].AccumNormal;
                    pm.P[best].Warm.TangentImpulseU     = C.Points[i].AccumTanU;
                    pm.P[best].Warm.TangentImpulseV     = C.Points[i].AccumTanV;
                    pm.P[best].CP.PositionWS            = C.Points[i].PositionWS;
                    pm.P[best].CP.NormalWS              = C.Points[i].NormalWS;
                }
            }
        }



        void Solve(std::vector<RigidBody>& bodies, const SolverSettings& settings) 
        {
            const int iters = settings.Iterations;
            for (int it = 0; it < iters; ++it) 
            {
                for (auto& c : Constraints) 
                {
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

                    float maxFriction   = c.Friction * c.ImpulseN;
                    float mag           = std::sqrt(newT1*newT1 + newT2*newT2);

                    if (mag > maxFriction + 1e-6f) 
                    {
                        float scale = maxFriction / (mag + 1e-12f);
                        newT1 *= scale; newT2 *= scale;
                    }

                    lambdaT1        = newT1 - oldT1;
                    lambdaT2        = newT2 - oldT2;
                    c.ImpulseT1     = newT1;
                    c.ImpulseT2     = newT2;
                    glm::vec3 PT    = c.T1 * lambdaT1 + c.T2 * lambdaT2;

                    A.ApplyLinearImpulse(-PT);
                    A.ApplyAngularImpulse(-glm::cross(c.RA, PT));

                    B.ApplyLinearImpulse(PT);
                    B.ApplyAngularImpulse(glm::cross(c.RB, PT));
                }
            }
        }
    };

} 
