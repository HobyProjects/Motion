#pragma once

#include <vector>
#include <limits>

#include "AABB.hpp"
#include "PhyCore.hpp"
#include "ManifoldClip.hpp"

namespace Motion
{
    struct Tri { std::uint32_t I0{0}, I1{0}, I2{0}; };

    struct ConvexHullShape : Shape
    {
        std::vector<glm::vec3> Vertice{};
        std::vector<Tri> Faces{};
        float ConvexRadius{0.0f};

        explicit ConvexHullShape(std::vector<glm::vec3> v = {}, std::vector<Tri> f = {}, float cr = 0.02f)
            : Vertice(std::move(v)), Faces(std::move(f)), ConvexRadius(cr)
        {
            Type = ShapeType::Convex; 
        }

        std::uint32_t SupportIndex(const glm::vec3& d) const 
        {
            float bestDot = -std::numeric_limits<float>::infinity();
            std::uint32_t best = 0u;
            for (std::uint32_t i = 0;i < Vertice.size(); ++i) 
            {
                float s = glm::dot(Vertice[i], d);
                if (s > bestDot) { bestDot = s; best = i; }
            }

            return best;
        }
        glm::vec3 SupportPoint(const glm::vec3& d) const 
        {
            return Vertice[SupportIndex(d)];
        }

        static AABB WorldAABB(const ConvexHullShape& h, const glm::mat4& M) 
        {
            glm::vec3 mn(+FLT_MAX), mx(-FLT_MAX);
            for (const auto& v : h.Vertice) 
            {
                glm::vec4 w = M * glm::vec4(v, 1.0f);
                glm::vec3 p = glm::vec3(w);
                mn = glm::min(mn, p);
                mx = glm::max(mx, p);
            }
            mn -= glm::vec3(h.ConvexRadius);
            mx += glm::vec3(h.ConvexRadius);
            return { mn, mx };
        }

        MassProperties Mass(float density) const 
        {
            double int_x    = 0, int_y  = 0, int_z  = 0;
            double int_xx   = 0, int_yy = 0, int_zz = 0;
            double int_xy   = 0, int_yz = 0, int_zx = 0;
            double volume   = 0;

            for (const auto& f : Faces) 
            {
                const glm::dvec3 a = Vertice[f.I0];
                const glm::dvec3 b = Vertice[f.I1];
                const glm::dvec3 c = Vertice[f.I2];

                const glm::dvec3 n2A = glm::cross(b - a, c - a);

                auto F1 = [](double a, double b, double c) { return a + b + c; };
                auto F2 = [](double a, double b, double c) { return a*a + b*b + c*c + a*b + b*c + c*a; };
                auto F3 = [](double a, double b, double c) { return a*a*a + b*b*b + c*c*c + a*a*(b+c) + b*b*(c+a) + c*c*(a+b) + a*b*c; };

                double x0 = a.x, y0 = a.y, z0 = a.z;
                double x1 = b.x, y1 = b.y, z1 = b.z;
                double x2 = c.x, y2 = c.y, z2 = c.z;

                double f1x = F1(x0,x1,x2);
                double f1y = F1(y0,y1,y2);
                double f1z = F1(z0,z1,z2);

                double f2x = F2(x0,x1,x2);
                double f2y = F2(y0,y1,y2);
                double f2z = F2(z0,z1,z2);

                double f3x = F3(x0,x1,x2);
                double f3y = F3(y0,y1,y2);
                double f3z = F3(z0,z1,z2);

                double nx   = n2A.x, ny = n2A.y, nz = n2A.z;
                double tmp  = (x0 * ny - y0 * nx) + (x1 * ny - y1 * nx) + (x2 * ny - y2 * nx); // not used
                (void)tmp;

                double aX = nx; double aY = ny; double aZ = nz;

                volume += (aX * f1x) / 6.0 + (aY * f1y) / 6.0 + (aZ * f1z) / 6.0;

                int_x  += aX * f2x / 24.0;
                int_y  += aY * f2y / 24.0;
                int_z  += aZ * f2z / 24.0;

                int_xx += aX * f3x / 60.0;
                int_yy += aY * f3y / 60.0;
                int_zz += aZ * f3z / 60.0;

                double gxy = (aX * (y0*y0 + y1*y1 + y2*y2 + y0*y1 + y1*y2 + y2*y0) + aY * (x0*x0 + x1*x1 + x2*x2 + x0*x1 + x1*x2 + x2*x0)) / 48.0;
                double gyz = (aY * (z0*z0 + z1*z1 + z2*z2 + z0*z1 + z1*z2 + z2*z0) + aZ * (y0*y0 + y1*y1 + y2*y2 + y0*y1 + y1*y2 + y2*y0)) / 48.0;
                double gzx = (aZ * (x0*x0 + x1*x1 + x2*x2 + x0*x1 + x1*x2 + x2*x0) + aX * (z0*z0 + z1*z1 + z2*z2 + z0*z1 + z1*z2 + z2*z0)) / 48.0;

                int_xy += gxy;
                int_yz += gyz;
                int_zx += gzx;
            }

            if (std::abs(volume) < 1e-12) 
            {
                glm::vec3 mn(+FLT_MAX), mx(-FLT_MAX);
                for (auto& v : Vertice) { mn = glm::min(mn, v); mx = glm::max(mx, v); }
                glm::vec3 he = 0.5f*(mx - mn);
                float mass = density * ( (mx.x-mn.x)*(mx.y-mn.y)*(mx.z-mn.z) );
                glm::vec3 com = 0.5f*(mx + mn);
                glm::mat3 I(0.0f);
                I[0][0] = (1.0f/12.0f)*mass*( (2*he.y)*(2*he.y) + (2*he.z)*(2*he.z) );
                I[1][1] = (1.0f/12.0f)*mass*( (2*he.x)*(2*he.x) + (2*he.z)*(2*he.z) );
                I[2][2] = (1.0f/12.0f)*mass*( (2*he.x)*(2*he.x) + (2*he.y)*(2*he.y) );
                return { mass, I, com };
            }

            double mass = density * volume;

            glm::dvec3 com( int_x, int_y, int_z );
            com /= volume * 2.0; 

            glm::dmat3 I0(0.0);
            I0[0][0] = density * (int_yy + int_zz);
            I0[1][1] = density * (int_zz + int_xx);
            I0[2][2] = density * (int_xx + int_yy);
            I0[0][1] = I0[1][0] = -density * int_xy;
            I0[1][2] = I0[2][1] = -density * int_yz;
            I0[2][0] = I0[0][2] = -density * int_zx;

            glm::dmat3 I = I0;
            glm::dvec3 r = com;
            double m = mass;
            glm::dmat3 Rxx(  r.y*r.y + r.z*r.z, -r.x*r.y,            -r.x*r.z,
                            -r.y*r.x,            r.x*r.x + r.z*r.z,  -r.y*r.z,
                            -r.z*r.x,           -r.z*r.y,            r.x*r.x + r.y*r.y );
            I -= m * Rxx;

            MassProperties mp{};
            mp.MASS     = (float)mass;
            mp.COM      = glm::vec3(com);
            mp.Inertia  = glm::mat3(I);
            return mp;
        }

    };
}