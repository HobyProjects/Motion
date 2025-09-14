#pragma once

#include <glm/glm.hpp>
#include <cmath>

namespace Motion
{
    inline void OrthonormalBasis(const glm::vec3& n, glm::vec3& t1, glm::vec3& t2) 
    {
        glm::vec3 h = (std::abs(n.x) < 0.577f) ? glm::vec3(1,0,0) :
                      (std::abs(n.y) < 0.577f) ? glm::vec3(0,1,0) : glm::vec3(0,0,1);

        t1 = glm::normalize(glm::cross(h, n));
        t2 = glm::cross(n, t1);
    }

    inline glm::mat3 Mat3FromMat4(const glm::mat4& M) 
    {
        return glm::mat3(glm::vec3(M[0]), glm::vec3(M[1]), glm::vec3(M[2]));
    }

    inline glm::mat3 AbsMat3(const glm::mat3& M) 
    {
        return glm::mat3(glm::abs(M[0]), glm::abs(M[1]), glm::abs(M[2]));
    }

    inline glm::vec3 AxisFromCol(const glm::mat4& M, int col, float& lenOut) 
    {
        glm::vec3 v = glm::vec3(M[col]);
        lenOut      = glm::length(v);
        return (lenOut > 0.f) ? (v / lenOut) : glm::vec3(0,1,0);
    }

    inline float MaxAxisScale(const glm::mat4& M) 
    {
        const float sx = glm::length(glm::vec3(M[0]));
        const float sy = glm::length(glm::vec3(M[1]));
        const float sz = glm::length(glm::vec3(M[2]));
        return glm::max(sx, glm::max(sy, sz));
    }

    inline glm::vec3 TransformPoint(const glm::mat4& M, const glm::vec3& p) 
    {
        return glm::vec3(M * glm::vec4(p, 1.0f));
    }

    inline glm::vec3 TransformVector(const glm::mat4& M, const glm::vec3& v) 
    {
        return glm::vec3(M * glm::vec4(v, 0.0f));
    }

    inline float LengthSq(const glm::vec3& v) 
    { 
        return glm::dot(v,v); 
    }

    inline glm::vec3 ClampVec(const glm::vec3& v, const glm::vec3& mn, const glm::vec3& mx) 
    {
        return glm::clamp(v, mn, mx);
    }

    inline float AbsDot(const glm::vec3& a, const glm::vec3& b) 
    {
        return glm::abs(glm::dot(a, b));
    }

    inline glm::vec3 RotateVector(const glm::mat4& M, const glm::vec3& v) 
    {
        glm::mat3 R = Mat3FromMat4(M);
        return R * v;
    }

    inline glm::vec3 TriNormalWS(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c)
    {
        return glm::normalize(glm::cross(b - a, c - a)); 
    }



}