#pragma once

#include <array>
#include <string>
#include <glm/glm.hpp>
namespace Motion
{
    struct SH9
    {
        std::array<glm::vec3, 9> coeff{};
        static SH9 ProjectEquirectHDR(const std::filesystem::path& hdrFile);
        static glm::vec3 Evaluate(const SH9& sh, const glm::vec3& n);
    };

    struct IBLTextureBinding
    {
        std::int32_t SlotPrefiltered{0};
        std::int32_t SlotBRDFLUT{0};
        std::int32_t SlotIrradiance{0};
    };

    struct EnvironmentSpecification
    {
        std::filesystem::path HDRfile{};
        bool UseSHDiffuse{true};
        bool BuildBRDFLUT{true};
        bool CompressBC6H{false};

        float DiffuseIntensity{1.0f};
        float SpecularIntensity{1.0f};
    };

    class IEnvironment
    {
        public:
            IEnvironment() = default;
            virtual ~IEnvironment() = default;

            virtual void RenderSkyBox(const glm::mat4& proj, const glm::mat4& view, float yawnRadians = 0.0f) const = 0;
            virtual void BindIBL(const IBLTextureBinding& params)  = 0;
            virtual void SetIntensity(float diffuse, float specular) = 0;

            [[nodiscard]] virtual SH9& GetDiffuseSH() = 0;
            [[nodiscard]] virtual bool IsUsingSH() = 0;
    
        protected:
            virtual void BakeHDR() = 0;

        public:
            static constexpr std::int32_t SH_BUFFER_BINDING_POINT = 5;
            
        public:
            static std::shared_ptr<IEnvironment> Create(const EnvironmentSpecification& spec);
    };
}