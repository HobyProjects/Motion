#include "CorePCH.hpp"
#include "Environment.hpp"

namespace Motion
{
    static inline void SHBasisL2(const glm::vec3& d, float outY[9])
    {
        const float c0 = 0.28209479177387814f;           
        const float c1 = 0.4886025119029199f;            
        const float c2 = 1.0925484305920792f;            
        const float c3 = 0.31539156525252005f;           
        const float c4 = 0.5462742152960396f;           

        const float x = d.x, y = d.y, z = d.z;

        outY[0] =  c0;
        outY[1] = -c1 * y;
        outY[2] =  c1 * z;
        outY[3] = -c1 * x;
        outY[4] =  c2 * x * y;
        outY[5] = -c2 * y * z;
        outY[6] =  c3 * (3.0f*z*z - 1.0f);
        outY[7] = -c2 * x * z;
        outY[8] =  c4 * (x*x - y*y);
    }

    SH9 SH9::ProjectEquirectHDR(const std::filesystem::path& hdrFile)
    {
        stbi_set_flip_vertically_on_load(false);
        std::int32_t w{0}, h{0}, comp{0};

        const std::string path = hdrFile.string();
        float* data = stbi_loadf(path.c_str(), &w, &h, &comp, 3);

        SH9 sh9{};
        if(!data || w <= 0 || h <= 0) return sh9;

        const float dphi    = (2.0f * glm::pi<float>()) / static_cast<float>(w);
        const float dtheta  = glm::pi<float>() / static_cast<float>(h);
        
        for(std::int32_t y = 0; y < h; ++y)
        {
            const float v           = (static_cast<float>(y) + 0.5f) / static_cast<float>(h);
            const float theta       = v * glm::pi<float>();
            const float sinTheta    = glm::sin(theta);

            for(std::int32_t x = 0; x < w; ++x)
            {
                const float u       = (static_cast<float>(x) + 0.5f) / static_cast<float>(w);
                const float phi     = u * 2.0f * glm::pi<float>();

                glm::vec3 dir = {
                    glm::cos(phi) * glm::sin(theta),
                    glm::cos(theta),
                    glm::sin(phi) * glm::sin(theta)
                };

                float Y[9];
                SHBasisL2(dir, Y);
                const std::int32_t idx = (y * w + x) * 3;
                const glm::vec3 L(data[idx + 0], data[idx + 1], data[idx + 2]);
                
                const float dOmega = dphi * dtheta * sinTheta;
                for(std::int32_t i = 0; i < 9; ++i)
                {
                    sh9.coeff[i] += L * (Y[i] * dOmega);
                }
            }
        }

        stbi_image_free(data);
        return sh9;
    }

    glm::vec3 SH9::Evaluate(const SH9& sh, const glm::vec3& n)
    {
        float Y[9];
        SHBasisL2(glm::normalize(n), Y);

        glm::vec3 r(0.0f);
        for(std::int32_t i = 0; i < 9; ++i) r += sh.coeff[i] * Y[i];
        return r;
    }

    std::shared_ptr<IEnvironment> Motion::IEnvironment::Create(const EnvironmentSpecification & spec)
    {
        switch (Renderer::GetAPI())
        {
            case RenderingAPI::OpenGL:         return std::make_shared<GL_Environment>(spec);
            case RenderingAPI::Vulkan:         MOTION_ASSERT(false, "Vulkan is not implemented yet!");  break;
            case RenderingAPI::DirectX:        MOTION_ASSERT(false, "DirectX is not implemented yet!"); break;
            default:                           MOTION_ASSERT(false, "Unknown rendering API!");          break;
        };

        return nullptr;
    }
}