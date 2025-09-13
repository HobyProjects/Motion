#pragma once

#include <vector>
#include "PhyMath.hpp"
#include "RigidBody.hpp"

namespace Motion
{
    struct FixedJointDesc
    {
        std::int32_t A{-1};
        std::int32_t B{-1};
        glm::vec3 AnchorA{0.0f}, AnchorB{0.0f};
        glm::quat RotationA{1.0f, 0.0f, 0.0f, 0.0f}, RotationB{1.0f, 0.0f, 0.0f, 0.0f};

        float LinearCompliance{0.0f};
        float AngularCompliance{0.0f};
        float LinearBaumgrate{0.2f};
        float AngularBaumgrate{0.2f};
    };

    struct FixedJointRunTime
    {
        FixedJointDesc Description{};

        glm::vec3 AccLinearImpulse{0.0f};
        glm::vec3 AccAngularImpulse{0.0f};
    };

    struct JointSettings
    {
        std::int32_t Iterations {12};
        bool WarmStart{true};
    };

    struct BallSocketDesc
    {
        std::int32_t A{-1}, B{-1};
        glm::vec3 AnchorA{0.0f};
        glm::vec3 AnchorB{0.0f};
        float Baumgarte{0.2f};
    };

    struct BallSocketRunTime
    {
        BallSocketDesc Description{};
        glm::vec3 AccImpulse{0.0f};
    };

    struct HingeDesc 
    {
        std::int32_t A{-1}, B{-1};          
        glm::vec3 AnchorA{0};           
        glm::vec3 AnchorB{0};           
        glm::vec3 AxisA{0,1,0};         
        glm::vec3 AxisB{0,1,0};         

        float LinearBaumgarte{0.2f};
        float AngularBaumgarte{0.2f};

        bool  EnableLimits{false};
        float LimitLow{-glm::half_pi<float>()};
        float LimitHigh{+glm::half_pi<float>()};

        bool  EnableMotor{false};
        float MotorSpeed{0.0f};     
        float MotorTorque{0.0f};     
    };

    struct HingeRunTime 
    {
        HingeDesc Description{};

        glm::vec3 AccLinearImpulse{0};    
        float     AccAngularAlign1{0.0f};  
        float     AccAngularAlign2{0.0f};
        float     AccMotor{0.0f};  

        std::int32_t LimitState{0};      
    };

    struct SliderDesc 
    {
        int A{-1}, B{-1};           
        glm::vec3 AnchorA{0};        
        glm::vec3 AnchorB{0};         
        glm::vec3 AxisA{1,0,0};      
        glm::vec3 AxisB{1,0,0};     

        glm::vec3 RefPerpA{0,1,0};   
        glm::vec3 RefPerpB{0,1,0};   

        float LinearBaumgarte{0.2f};    
        float AngularBaumgarte{0.2f};  

        bool  EnableLimits{false};
        float LimitLow{-1.0f};
        float LimitHigh{+1.0f};        

        bool  EnableMotor   = false;  
        float MotorSpeed    = 0.0f;  
        float MotorForce    = 0.0f;  
    };

    struct SliderRuntime 
    {
        SliderDesc Description;

        glm::vec2 AccLinPerp{0};      
        glm::vec3 AccAng{0};        
        float     AccMotor{0.0f};    
        int       LimitState{0};    
    };

    struct JointBatch
    {
        std::vector<FixedJointRunTime> Fixed{};
        std::vector<BallSocketRunTime> Balls{};
        std::vector<HingeRunTime>      Hinges{};
        std::vector<SliderRuntime>     Sliders{};

        void Clear() 
        { 
            Fixed.clear();
            Balls.clear();
            Hinges.clear();
            Sliders.clear();
        }

        void AddFixed(const FixedJointDesc& d){ Fixed.push_back(FixedJointRunTime{ d, {}, {} }); }
        void AddBall(const BallSocketDesc& d){ Balls.push_back(BallSocketRunTime(d, {})); }
        void AddHinge(const HingeDesc& d){ Hinges.push_back(HingeRunTime{ d }); }
        void AddSlider(const SliderDesc& d){ Sliders.push_back(SliderRuntime{ d }); }

        void WarmStart(std::vector<RigidBody>& bodies, const JointSettings& set) 
        {
            if (!set.WarmStart) return;

            for (auto& J : Fixed) 
            {
                auto& A = bodies[J.Description.A];
                auto& B = bodies[J.Description.B];

                A.ApplyLinearImpulse( -J.AccLinearImpulse );
                A.ApplyAngularImpulse( -J.AccAngularImpulse );
                B.ApplyLinearImpulse(  J.AccLinearImpulse );
                B.ApplyAngularImpulse(  J.AccAngularImpulse );
            }

            for (auto& J : Balls) 
            {
                auto& A = bodies[J.Description.A]; auto& B = bodies[J.Description.B];
                const glm::vec3 P = J.AccImpulse;

                
                const glm::mat3 RA = glm::toMat3(A.Rotation);
                const glm::mat3 RB = glm::toMat3(B.Rotation);
                const glm::vec3 rA = RA * J.Description.AnchorA;
                const glm::vec3 rB = RB * J.Description.AnchorB;

                A.ApplyLinearImpulse( -P );
                A.ApplyAngularImpulse( -glm::cross(rA, P) );
                B.ApplyLinearImpulse(  P );
                B.ApplyAngularImpulse(  glm::cross(rB, P) );
            }

            for (auto& J : Hinges) 
            {
                auto& A = bodies[J.Description.A];
                auto& B = bodies[J.Description.B];

                const glm::mat3 RA = glm::toMat3(A.Rotation);
                const glm::mat3 RB = glm::toMat3(B.Rotation);
                const glm::vec3 rA = RA * J.Description.AnchorA;
                const glm::vec3 rB = RB * J.Description.AnchorB;

                const glm::vec3 P = J.AccLinearImpulse;
                A.ApplyLinearImpulse( -P ); A.ApplyAngularImpulse( -glm::cross(rA, P) );
                B.ApplyLinearImpulse(  P ); B.ApplyAngularImpulse(  glm::cross(rB, P) );

                const glm::vec3 axisA_ws = glm::normalize(RA * J.Description.AxisA);
                glm::vec3 t1, t2; OrthonormalBasis(axisA_ws, t1, t2);

                A.ApplyAngularImpulse( -(t1 * J.AccAngularAlign1 + t2 * J.AccAngularAlign2) );
                B.ApplyAngularImpulse(  (t1 * J.AccAngularAlign1 + t2 * J.AccAngularAlign2) );
                A.ApplyAngularImpulse( -(axisA_ws * J.AccMotor) );
                B.ApplyAngularImpulse(  (axisA_ws * J.AccMotor) );
            }

            for (auto& J : Sliders) 
            {
                auto& A = bodies[J.Description.A];
                auto& B = bodies[J.Description.B];

                const glm::mat3 RA = glm::toMat3(A.Rotation);
                const glm::mat3 RB = glm::toMat3(B.Rotation);

                const glm::vec3 rA = RA * J.Description.AnchorA;
                const glm::vec3 rB = RB * J.Description.AnchorB;

                const glm::vec3 axis = glm::normalize(RA * J.Description.AnchorA);
                glm::vec3 t1, t2; OrthonormalBasis(axis, t1, t2);

                glm::vec3 P = t1 * J.AccLinPerp.x + t2 * J.AccLinPerp.y;
                A.ApplyLinearImpulse( -P ); A.ApplyAngularImpulse( -glm::cross(rA, P) );
                B.ApplyLinearImpulse(  P ); B.ApplyAngularImpulse(  glm::cross(rB, P) );

                const glm::vec3 Lw = J.AccAng;
                A.ApplyAngularImpulse( -(t1*Lw.x + t2*Lw.y + axis*Lw.z) );
                B.ApplyAngularImpulse(  (t1*Lw.x + t2*Lw.y + axis*Lw.z) );

                A.ApplyAngularImpulse( glm::vec3(0) );
                glm::vec3 Pm = axis * J.AccMotor;
                A.ApplyLinearImpulse( -Pm ); A.ApplyAngularImpulse( -glm::cross(rA, Pm) );
                B.ApplyLinearImpulse(  Pm ); B.ApplyAngularImpulse(  glm::cross(rB, Pm) );
            }
        }

        void Solve(std::vector<RigidBody>& bodies, const JointSettings& set, float dt) 
        {
            for (int it = 0; it < set.Iterations; ++it) 
            {
                for (auto& J : Fixed) 
                {
                    auto& A = bodies[J.Description.A];
                    auto& B = bodies[J.Description.B];

                    const glm::mat3 RA = glm::toMat3(A.Rotation);
                    const glm::mat3 RB = glm::toMat3(B.Rotation);
                    const glm::vec3 rA = RA * J.Description.AnchorA;
                    const glm::vec3 rB = RB * J.Description.AnchorB; 

                    const glm::vec3 pA = A.Position + rA;
                    const glm::vec3 pB = B.Position + rB;

                    const glm::vec3 C_lin = pB - pA;
                    glm::mat3 K = glm::mat3( (A.InvMass + B.InvMass) );

                    
                    auto SKew = [&](const glm::vec3& v) -> glm::mat3
                    {
                        return glm::mat3(
                            0.0f,   -v.z,    v.y,
                            v.z,     0.0f,  -v.x,
                           -v.y,     v.x,    0.0f
                        );
                    };
                    
                    K += SKew(rA) * A.InvInertiaWorld * glm::transpose(SKew(rA));
                    K += SKew(rB) * B.InvInertiaWorld * glm::transpose(SKew(rB));

                    const float betaL       = J.Description.LinearBaumgrate;
                    const glm::vec3 bLin    = (betaL / (float)set.Iterations) * C_lin;

                    const glm::vec3 vA = A.LinearVelocity + glm::cross(A.AngularVelocity, rA);
                    const glm::vec3 vB = B.LinearVelocity + glm::cross(B.AngularVelocity, rB);
                    const glm::vec3 vRel = vB - vA;

                    const float det =
                        K[0][0]*(K[1][1]*K[2][2]-K[1][2]*K[2][1]) -
                        K[0][1]*(K[1][0]*K[2][2]-K[1][2]*K[2][0]) +
                        K[0][2]*(K[1][0]*K[2][1]-K[1][1]*K[2][0]);

                    glm::mat3 invK(0.0f);
                    if (std::abs(det) > 1e-9f) 
                    {
                        const float invDet = 1.0f/det;
                        invK[0][0] =  (K[1][1]*K[2][2]-K[1][2]*K[2][1])*invDet;
                        invK[0][1] = -(K[0][1]*K[2][2]-K[0][2]*K[2][1])*invDet;
                        invK[0][2] =  (K[0][1]*K[1][2]-K[0][2]*K[1][1])*invDet;
                        invK[1][0] = -(K[1][0]*K[2][2]-K[1][2]*K[2][0])*invDet;
                        invK[1][1] =  (K[0][0]*K[2][2]-K[0][2]*K[2][0])*invDet;
                        invK[1][2] = -(K[0][0]*K[1][2]-K[0][2]*K[1][0])*invDet;
                        invK[2][0] =  (K[1][0]*K[2][1]-K[1][1]*K[2][0])*invDet;
                        invK[2][1] = -(K[0][0]*K[2][1]-K[0][1]*K[2][0])*invDet;
                        invK[2][2] =  (K[0][0]*K[1][1]-K[0][1]*K[1][0])*invDet;
                    }
                    
                    const glm::vec3 lambdaL = -invK * (vRel + bLin);
                    A.ApplyLinearImpulse( -lambdaL );
                    A.ApplyAngularImpulse( -glm::cross(rA, lambdaL) );
                    B.ApplyLinearImpulse(  lambdaL );
                    B.ApplyAngularImpulse(  glm::cross(rB, lambdaL) );

                    J.AccLinearImpulse += lambdaL; 

                    const glm::quat qA = A.Rotation * J.Description.RotationA; 
                    const glm::quat qB = B.Rotation * J.Description.RotationB; 
                    
                    glm::quat qErr = qA * glm::conjugate(qB);
                    if (qErr.w < 0) qErr = -qErr;
                    glm::vec3 C_ang = 2.0f * glm::vec3(qErr.x, qErr.y, qErr.z);

                    glm::mat3 KA    = A.InvInertiaWorld;
                    glm::mat3 KB    = B.InvInertiaWorld;
                    glm::mat3 KAng  = KA + KB;

                    const float betaA = J.Description.AngularBaumgrate;
                    const glm::vec3 bAng = (betaA / (float)set.Iterations) * C_ang;

                    const glm::vec3 wRel = B.AngularVelocity - A.AngularVelocity;

                    float detW =
                        KAng[0][0]*(KAng[1][1]*KAng[2][2]-KAng[1][2]*KAng[2][1]) -
                        KAng[0][1]*(KAng[1][0]*KAng[2][2]-KAng[1][2]*KAng[2][0]) +
                        KAng[0][2]*(KAng[1][0]*KAng[2][1]-KAng[1][1]*KAng[2][0]);
                    
                        glm::mat3 invKAng(0.0f);
                    if (std::abs(detW) > 1e-9f) 
                    {
                        float invDet = 1.0f/detW;
                        invKAng[0][0] =  (KAng[1][1]*KAng[2][2]-KAng[1][2]*KAng[2][1])*invDet;
                        invKAng[0][1] = -(KAng[0][1]*KAng[2][2]-KAng[0][2]*KAng[2][1])*invDet;
                        invKAng[0][2] =  (KAng[0][1]*KAng[1][2]-KAng[0][2]*KAng[1][1])*invDet;
                        invKAng[1][0] = -(KAng[1][0]*KAng[2][2]-KAng[1][2]*KAng[2][0])*invDet;
                        invKAng[1][1] =  (KAng[0][0]*KAng[2][2]-KAng[0][2]*KAng[2][0])*invDet;
                        invKAng[1][2] = -(KAng[0][0]*KAng[1][2]-KAng[0][2]*KAng[1][0])*invDet;
                        invKAng[2][0] =  (KAng[1][0]*KAng[2][1]-KAng[1][1]*KAng[2][0])*invDet;
                        invKAng[2][1] = -(KAng[0][0]*KAng[2][1]-KAng[0][1]*KAng[2][0])*invDet;
                        invKAng[2][2] =  (KAng[0][0]*KAng[1][1]-KAng[0][1]*KAng[1][0])*invDet;
                    }
                    
                    const glm::vec3 lambdaW = -invKAng * (wRel + bAng);

                    A.ApplyAngularImpulse( -lambdaW );
                    B.ApplyAngularImpulse(  lambdaW );
                    J.AccAngularImpulse += lambdaW;
                }
            }


            for (int it = 0; it < set.Iterations; ++it) 
            {
                for (auto& J : Balls) 
                {
                    auto& A = bodies[J.Description.A];
                    auto& B = bodies[J.Description.B];

                    const glm::mat3 RA = glm::toMat3(A.Rotation);
                    const glm::mat3 RB = glm::toMat3(B.Rotation);
                    const glm::vec3 rA = RA * J.Description.AnchorA;
                    const glm::vec3 rB = RB * J.Description.AnchorB;

                    const glm::vec3 pA = A.Position + rA;
                    const glm::vec3 pB = B.Position + rB;

                    glm::mat3 K = glm::mat3( (A.InvMass + B.InvMass) );
                    auto Skew = [](const glm::vec3& v)->glm::mat3
                    {
                        return glm::mat3(  0,  -v.z,  v.y,
                                          v.z,   0 , -v.x,
                                         -v.y,  v.x,   0 );
                    };

                    K += Skew(rA) * A.InvInertiaWorld * glm::transpose(Skew(rA));
                    K += Skew(rB) * B.InvInertiaWorld * glm::transpose(Skew(rB));

                    const glm::vec3 C       = pB - pA;
                    const glm::vec3 vA      = A.LinearVelocity + glm::cross(A.AngularVelocity, rA);
                    const glm::vec3 vB      = B.LinearVelocity + glm::cross(B.AngularVelocity, rB);
                    const glm::vec3 vRel    = vB - vA;
                    const glm::vec3 b       = (J.Description.Baumgarte / (float)set.Iterations) * C;

                    const float det =
                        K[0][0]*(K[1][1]*K[2][2]-K[1][2]*K[2][1]) -
                        K[0][1]*(K[1][0]*K[2][2]-K[1][2]*K[2][0]) +
                        K[0][2]*(K[1][0]*K[2][1]-K[1][1]*K[2][0]);
                    
                    glm::mat3 invK(0);
                    if (std::abs(det) > 1e-9f) 
                    {
                        const float invDet = 1.0f/det;
                        invK[0][0] =  (K[1][1]*K[2][2]-K[1][2]*K[2][1])*invDet;
                        invK[0][1] = -(K[0][1]*K[2][2]-K[0][2]*K[2][1])*invDet;
                        invK[0][2] =  (K[0][1]*K[1][2]-K[0][2]*K[1][1])*invDet;
                        invK[1][0] = -(K[1][0]*K[2][2]-K[1][2]*K[2][0])*invDet;
                        invK[1][1] =  (K[0][0]*K[2][2]-K[0][2]*K[2][0])*invDet;
                        invK[1][2] = -(K[0][0]*K[1][2]-K[0][2]*K[1][0])*invDet;
                        invK[2][0] =  (K[1][0]*K[2][1]-K[1][1]*K[2][0])*invDet;
                        invK[2][1] = -(K[0][0]*K[2][1]-K[0][1]*K[2][0])*invDet;
                        invK[2][2] =  (K[0][0]*K[1][1]-K[0][1]*K[1][0])*invDet;
                    }

                    const glm::vec3 lambda = -invK * (vRel + b);

                    A.ApplyLinearImpulse( -lambda );
                    A.ApplyAngularImpulse( -glm::cross(rA, lambda) );
                    B.ApplyLinearImpulse(  lambda );
                    B.ApplyAngularImpulse(  glm::cross(rB, lambda) );

                    J.AccImpulse += lambda;
                }
            }

            for (int it = 0; it < set.Iterations; ++it) 
            {
                for (auto& J : Hinges) 
                {
                    auto& A = bodies[J.Description.A];
                    auto& B = bodies[J.Description.B];

                    const glm::mat3 RA = glm::toMat3(A.Rotation);
                    const glm::mat3 RB = glm::toMat3(B.Rotation);
                    const glm::vec3 rA = RA * J.Description.AnchorA;
                    const glm::vec3 rB = RB * J.Description.AnchorB;

                    const glm::vec3 pA = A.Position + rA;
                    const glm::vec3 pB = B.Position + rB;

                    auto Skew = [](const glm::vec3& v) ->glm::mat3
                    {
                        return glm::mat3(  0, -v.z,  v.y, v.z, 0 , -v.x, -v.y,  v.x,   0 );
                    };

                    glm::mat3 K = glm::mat3( (A.InvMass + B.InvMass) );
                    K += Skew(rA) * A.InvInertiaWorld * glm::transpose(Skew(rA));
                    K += Skew(rB) * B.InvInertiaWorld * glm::transpose(Skew(rB));

                    const glm::vec3 C       = pB - pA;
                    const glm::vec3 vA      = A.LinearVelocity + glm::cross(A.AngularVelocity, rA);
                    const glm::vec3 vB      = B.LinearVelocity + glm::cross(B.AngularVelocity, rB);
                    const glm::vec3 vRel    = vB - vA;
                    const glm::vec3 bLin    = (J.Description.LinearBaumgarte / (float)set.Iterations) * C;

                    float det =
                        K[0][0]*(K[1][1]*K[2][2]-K[1][2]*K[2][1]) -
                        K[0][1]*(K[1][0]*K[2][2]-K[1][2]*K[2][0]) +
                        K[0][2]*(K[1][0]*K[2][1]-K[1][1]*K[2][0]);

                    glm::mat3 invK(0);
                    if (std::abs(det) > 1e-9f) 
                    {
                        float invDet = 1.0f/det;
                        invK[0][0] =  (K[1][1]*K[2][2]-K[1][2]*K[2][1])*invDet;
                        invK[0][1] = -(K[0][1]*K[2][2]-K[0][2]*K[2][1])*invDet;
                        invK[0][2] =  (K[0][1]*K[1][2]-K[0][2]*K[1][1])*invDet;
                        invK[1][0] = -(K[1][0]*K[2][2]-K[1][2]*K[2][0])*invDet;
                        invK[1][1] =  (K[0][0]*K[2][2]-K[0][2]*K[2][0])*invDet;
                        invK[1][2] = -(K[0][0]*K[1][2]-K[0][2]*K[1][0])*invDet;
                        invK[2][0] =  (K[1][0]*K[2][1]-K[1][1]*K[2][0])*invDet;
                        invK[2][1] = -(K[0][0]*K[2][1]-K[0][1]*K[2][0])*invDet;
                        invK[2][2] =  (K[0][0]*K[1][1]-K[0][1]*K[1][0])*invDet;
                    }

                    const glm::vec3 lambdaL = -invK * (vRel + bLin);
                    A.ApplyLinearImpulse( -lambdaL ); A.ApplyAngularImpulse( -glm::cross(rA, lambdaL) );
                    B.ApplyLinearImpulse(  lambdaL ); B.ApplyAngularImpulse(  glm::cross(rB, lambdaL) );
                    J.AccLinearImpulse += lambdaL;

                    const glm::vec3 axisA_ws = glm::normalize(RA * J.Description.AxisA);
                    const glm::vec3 axisB_ws = glm::normalize(RB * J.Description.AxisB);

                    glm::vec3 t1, t2; OrthonormalBasis(axisA_ws, t1, t2);
                    float err1 = glm::dot(glm::cross(axisB_ws, axisA_ws), t1); 
                    float err2 = glm::dot(glm::cross(axisB_ws, axisA_ws), t2);

                    float k1 = glm::dot(t1, A.InvInertiaWorld * t1) + glm::dot(t1, B.InvInertiaWorld * t1);
                    float k2 = glm::dot(t2, A.InvInertiaWorld * t2) + glm::dot(t2, B.InvInertiaWorld * t2);
                    float m1 = (k1 > 1e-9f) ? 1.0f/k1 : 0.0f;
                    float m2 = (k2 > 1e-9f) ? 1.0f/k2 : 0.0f;

                    float wRel1 = glm::dot(B.AngularVelocity - A.AngularVelocity, t1);
                    float wRel2 = glm::dot(B.AngularVelocity - A.AngularVelocity, t2);

                    float bAng = J.Description.AngularBaumgarte / (float)set.Iterations;
                    float lambda1 = -m1 * (wRel1 + bAng * err1);
                    float lambda2 = -m2 * (wRel2 + bAng * err2);

                    A.ApplyAngularImpulse( -(t1 * lambda1 + t2 * lambda2) );
                    B.ApplyAngularImpulse(  (t1 * lambda1 + t2 * lambda2) );
                    J.AccAngularAlign1 += lambda1; J.AccAngularAlign2 += lambda2;

                    const glm::vec3 wRel = B.AngularVelocity - A.AngularVelocity;
                    float wHinge = glm::dot(wRel, axisA_ws);

                    float lambdaH = 0.0f;
                    if (J.Description.EnableMotor && J.Description.MotorTorque > 0.0f) 
                    {
                        float desired = J.Description.MotorSpeed;
                        float errorV  = wHinge - desired;

                        float kH = glm::dot(axisA_ws, A.InvInertiaWorld * axisA_ws) +
                                   glm::dot(axisA_ws, B.InvInertiaWorld * axisA_ws);

                        float mH = (kH > 1e-9f) ? 1.0f/kH : 0.0f;

                        float impulse   = -mH * errorV;
                        float maxImp    = J.Description.MotorTorque * dt; 
                        float prev      = J.AccMotor;
                        float next      = glm::clamp(prev + impulse, -maxImp, +maxImp);
                        lambdaH         = next - prev;
                        J.AccMotor      = next;

                        A.ApplyAngularImpulse( -(axisA_ws * lambdaH) );
                        B.ApplyAngularImpulse(  (axisA_ws * lambdaH) );
                    }

                    if (J.Description.EnableLimits) 
                    {
                        glm::vec3 refA      = t1;
                        glm::vec3 refA_onB  = refA - axisA_ws * glm::dot(refA, axisA_ws);
                        glm::vec3 refB      = axisB_ws; 

                        glm::vec3 bPerp = (RB * J.Description.AxisB);
                        bPerp           = bPerp - axisA_ws * glm::dot(bPerp, axisA_ws);

                        if (glm::length2(refA_onB) > 1e-12f && glm::length2(bPerp) > 1e-12f) 
                        {
                            refA_onB        = glm::normalize(refA_onB);
                            bPerp           = glm::normalize(bPerp);
                            float cosang    = glm::clamp(glm::dot(refA_onB, bPerp), -1.0f, 1.0f);
                            float sign      = glm::sign(glm::dot(glm::cross(refA_onB, bPerp), axisA_ws));
                            float angle     = sign * std::acos(cosang);

                            float kH = glm::dot(axisA_ws, A.InvInertiaWorld * axisA_ws) +
                                       glm::dot(axisA_ws, B.InvInertiaWorld * axisA_ws);
                            float mH = (kH > 1e-9f) ? 1.0f/kH : 0.0f;

                            float correction = 0.0f;
                            if (angle < J.Description.LimitLow) 
                            {
                                correction  = (J.Description.LimitLow - angle);
                                float imp   = -mH * ( wHinge + (J.Description.AngularBaumgarte / (float)set.Iterations) * correction );
                                if (imp < 0.0f) 
                                {
                                    A.ApplyAngularImpulse( -(axisA_ws * imp) );
                                    B.ApplyAngularImpulse(  (axisA_ws * imp) );
                                }
                            } 
                            else if (angle > J.Description.LimitHigh) 
                            {
                                correction = (J.Description.LimitHigh - angle);
                                float imp = -mH * ( wHinge + (J.Description.AngularBaumgarte / (float)set.Iterations) * correction );
                                if (imp > 0.0f) 
                                {
                                    A.ApplyAngularImpulse( -(axisA_ws * imp) );
                                    B.ApplyAngularImpulse(  (axisA_ws * imp) );
                                }
                            }
                        }
                    }
                }
            }

            for (int it = 0; it < set.Iterations; ++it) 
            {
                for (auto& J : Sliders) 
                {
                    auto& A = bodies[J.Description.A];
                    auto& B = bodies[J.Description.B];

                    const glm::mat3 RA = glm::toMat3(A.Rotation);
                    const glm::mat3 RB = glm::toMat3(B.Rotation);
                    const glm::vec3 rA = RA * J.Description.AnchorA;
                    const glm::vec3 rB = RB * J.Description.AnchorB;
                    const glm::vec3 pA = A.Position + rA;
                    const glm::vec3 pB = B.Position + rB;
                    
                    const glm::vec3 axis = glm::normalize(RA * J.Description.AxisA);
                    glm::vec3 t1, t2; OrthonormalBasis(axis, t1, t2);

                    auto Skew = [](const glm::vec3& v) -> glm::mat3 
                    {
                        return glm::mat3( 0.0f, -v.z,    v.y,
                                          v.z,   0.0f , -v.x,
                                         -v.y,   v.x,    0.0f );
                    };

                    glm::mat3 K = glm::mat3( (A.InvMass + B.InvMass) );
                    K += Skew(rA) * A.InvInertiaWorld * glm::transpose(Skew(rA));
                    K += Skew(rB) * B.InvInertiaWorld * glm::transpose(Skew(rB));

                    auto effMass1D = [&](const glm::vec3& dir)->float 
                    {
                        glm::vec3 Kdir  = K * dir; 
                        float k         = glm::dot(dir, Kdir);

                        return (k > 1e-9f) ? 1.0f/k : 0.0f;
                    };

                    const glm::vec3 C       = pB - pA;
                    const glm::vec3 vA      = A.LinearVelocity + glm::cross(A.AngularVelocity, rA);
                    const glm::vec3 vB      = B.LinearVelocity + glm::cross(B.AngularVelocity, rB);
                    const glm::vec3 vRel    = vB - vA;

                    {
                        float m         = effMass1D(t1);
                        float posErr    = glm::dot(C, t1);
                        float velErr    = glm::dot(vRel, t1);
                        float b         = (J.Description.LinearBaumgarte / (float)set.Iterations) * posErr;
                        float lambda    = -m * (velErr + b);
                        glm::vec3 P     = t1 * lambda;

                        A.ApplyLinearImpulse( -P ); A.ApplyAngularImpulse( -glm::cross(rA, P) );
                        B.ApplyLinearImpulse(  P ); B.ApplyAngularImpulse(  glm::cross(rB, P) );
                        J.AccLinPerp.x += lambda;
                    }

                    {
                        float m         = effMass1D(t2);
                        float posErr    = glm::dot(C, t2);
                        float velErr    = glm::dot(vRel, t2);
                        float b         = (J.Description.LinearBaumgarte / (float)set.Iterations) * posErr;
                        float lambda    = -m * (velErr + b);
                        glm::vec3 P     = t2 * lambda;

                        A.ApplyLinearImpulse( -P ); A.ApplyAngularImpulse( -glm::cross(rA, P) );
                        B.ApplyLinearImpulse(  P ); B.ApplyAngularImpulse(  glm::cross(rB, P) );
                        J.AccLinPerp.y += lambda;
                    }

                    const glm::vec3 axisB_ws    = glm::normalize(RB * J.Description.AxisB);
                    glm::vec3 errAxis           = glm::cross(axisB_ws, axis);

                    float err1 = glm::dot(errAxis, t1);
                    float err2 = glm::dot(errAxis, t2);

                    glm::vec3 refA_ws   = glm::normalize(RA * J.Description.RefPerpA - axis * glm::dot(RA * J.Description.RefPerpA, axis));
                    glm::vec3 refB_ws   = glm::normalize(RB * J.Description.RefPerpB - axis * glm::dot(RB * J.Description.RefPerpB, axis));
                    float errTwist      = glm::dot(glm::cross(refB_ws, refA_ws), axis); 

                    float k1 = glm::dot(t1, A.InvInertiaWorld * t1) + glm::dot(t1, B.InvInertiaWorld * t1);
                    float k2 = glm::dot(t2, A.InvInertiaWorld * t2) + glm::dot(t2, B.InvInertiaWorld * t2);
                    float kT = glm::dot(axis, A.InvInertiaWorld * axis) + glm::dot(axis, B.InvInertiaWorld * axis);
                    float m1 = (k1>1e-9f)? 1.0f/k1 : 0.0f;
                    float m2 = (k2>1e-9f)? 1.0f/k2 : 0.0f;
                    float mT = (kT>1e-9f)? 1.0f/kT : 0.0f;

                    float w1 = glm::dot(B.AngularVelocity - A.AngularVelocity, t1);
                    float w2 = glm::dot(B.AngularVelocity - A.AngularVelocity, t2);
                    float wT = glm::dot(B.AngularVelocity - A.AngularVelocity, axis);

                    float beta = J.Description.AngularBaumgarte / (float)set.Iterations;
                    float l1 = -m1 * (w1 + beta * err1);
                    float l2 = -m2 * (w2 + beta * err2);
                    float lT = -mT * (wT + beta * errTwist);

                    A.ApplyAngularImpulse( -(t1*l1 + t2*l2 + axis*lT) );
                    B.ApplyAngularImpulse(  (t1*l1 + t2*l2 + axis*lT) );
                    J.AccAng += glm::vec3(l1, l2, lT);

                    float posAxis = glm::dot(C, axis);    
                    float velAxis = glm::dot(vRel, axis);

                    if (J.Description.EnableMotor && J.Description.MotorForce > 0.0f) 
                    {
                        float mA        = effMass1D(axis);
                        float desired   = J.Description.MotorSpeed;
                        float lambda    = -mA * (velAxis - desired);
                        float maxImp    = J.Description.MotorForce * dt;
                        float prev      = J.AccMotor;
                        float next      = glm::clamp(prev + lambda, -maxImp, +maxImp);
                        float dImp      = next - prev;
                        J.AccMotor      = next;
                        glm::vec3 Pm    = axis * dImp;

                        A.ApplyLinearImpulse( -Pm ); A.ApplyAngularImpulse( -glm::cross(rA, Pm) );
                        B.ApplyLinearImpulse(  Pm ); B.ApplyAngularImpulse(  glm::cross(rB, Pm) );
                    }

                    if (J.Description.EnableLimits) 
                    {
                        float mA    = effMass1D(axis);
                        float corr  = 0.0f;
                        if (posAxis < J.Description.LimitLow) 
                        {
                            corr            = (J.Description.LimitLow - posAxis);
                            float b         = (J.Description.LinearBaumgarte / (float)set.Iterations) * corr;
                            float lambda    = -mA * (velAxis + b);

                            if (lambda > 0.0f) 
                            { 
                                glm::vec3 P = axis * lambda;
                                A.ApplyLinearImpulse( -P ); A.ApplyAngularImpulse( -glm::cross(rA, P) );
                                B.ApplyLinearImpulse(  P ); B.ApplyAngularImpulse(  glm::cross(rB, P) );
                            }
                        } 
                        else if (posAxis > J.Description.LimitHigh) 
                        {
                            corr            = (J.Description.LimitHigh - posAxis);
                            float b         = (J.Description.LinearBaumgarte / (float)set.Iterations) * corr;
                            float lambda    = -mA * (velAxis + b);

                            if (lambda < 0.0f) 
                            { 
                                glm::vec3 P = axis * lambda;
                                A.ApplyLinearImpulse( -P ); A.ApplyAngularImpulse( -glm::cross(rA, P) );
                                B.ApplyLinearImpulse(  P ); B.ApplyAngularImpulse(  glm::cross(rB, P) );
                            }
                        }
                    }
                }
            }



        }
    };
}