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
        bool UseSHDiffuse{false};
        bool BuildBRDFLUT{true};
        bool CompressBC6H{false};

        float Intensity{1.0f};      // Control range 0.0 - 5.0
        float Exposure{0.0f};       // Control range -5.0 - 5.0
        float Gamma{2.2f};          // Control range 1.8 - 2.4
        float MaxMipLevel{-1.0f};   // Control range -1.0 - 0.0
        std::int32_t Tonemap{2};    // 0=None, 1=Reinhard, 2=ACESFitted
    };


    class IEnvironment
    {
        public:
            IEnvironment() = default;
            virtual ~IEnvironment() = default;

            virtual void RenderSkyBox(const glm::mat4& proj, const glm::mat4& view, float yawnRadians = 0.0f) const = 0;
            virtual void BindIBL(const IBLTextureBinding& params) = 0;

            virtual void UpdateSpecification(const EnvironmentSpecification& spec) = 0;
            virtual EnvironmentSpecification& GetSpecification() = 0;

            [[nodiscard]] virtual SH9& GetDiffuseSH() = 0;
            [[nodiscard]] virtual bool IsUsingSH() = 0;

        protected:
            virtual void BakeHDR() = 0;

        public:
            static constexpr std::int32_t SH_BUFFER_BINDING_POINT = 5;
            static std::shared_ptr<IEnvironment> Create(const EnvironmentSpecification& spec);
    };
}