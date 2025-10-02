#pragma once

#include <string>
#include <string_view>
#include <memory>
#include <unordered_map>
#include <vector>
#include <glm/glm.hpp>
#include <entt/entt.hpp>

namespace Motion
{
    struct BaseMaterial 
    {
        glm::vec3 BaseColor{ 1.0f, 1.0f, 1.0f };
        float MetallicFactor{ 0.0f };
        float RoughnessFactor{ 0.0f };
        float OpacityFactor{ 1.0f };

        std::string Name;
        std::unordered_map<TextureType, std::shared_ptr<ITexture>> Textures{};

        BaseMaterial(const std::string& materialName) : Name(materialName) {};
        ~BaseMaterial() = default;
    };

    using MaterialHandle = entt::entity;

    class Material
    {
        public:
            Material() = default;
            Material(MaterialHandle handle, const std::shared_ptr<BaseMaterial>& baseMaterial) :
                m_Handle(handle), m_BaseMaterial(baseMaterial) {}
            ~Material() = default;

            template<typename T>
            bool Has() const
            {
                return m_MaterialRegistry.any_of<T>(m_Handle);
            }

            template<typename T, typename... Args>
            T& Emplace(Args&&... args)
            {
                return m_MaterialRegistry.emplace<T>(m_Handle, std::forward<Args>(args)...);
            }

            template<typename T>
            T& Get() const
            {
                return m_MaterialRegistry.get<T>(m_Handle);
            }

            std::shared_ptr<BaseMaterial> GetBaseMaterial() const
            {
                return m_BaseMaterial;
            }

            void SetBaseMaterial(const std::shared_ptr<BaseMaterial>& baseMaterial)
            {
                m_BaseMaterial = baseMaterial;
            }

            template<typename T>
            void Remove()
            {
                m_MaterialRegistry.remove<T>(m_Handle);
            }

            [[nodiscard]] MaterialHandle Handle() const { return m_Handle; }
            [[nodiscard]] operator bool() const { return m_Handle != entt::null; }
            [[nodiscard]] operator std::uint32_t() const { return static_cast<std::uint32_t>(m_Handle); }
            [[nodiscard]] operator entt::entity() const { return m_Handle; }
            [[nodiscard]] bool operator==(const Material& other) const { return m_Handle == other.m_Handle; }
            [[nodiscard]] bool operator!=(const Material& other) const { return m_Handle != other.m_Handle; }

        public:
            static std::shared_ptr<Material> Create(const std::shared_ptr<BaseMaterial>& baseMaterial = nullptr);
            static std::shared_ptr<BaseMaterial> CreateBase(const std::filesystem::path& materialYAML);
            static void Destroy(const std::shared_ptr<Material>& material);

        private:
            inline static entt::registry m_MaterialRegistry;

        private:
            MaterialHandle m_Handle{ entt::null };
            std::shared_ptr<BaseMaterial> m_BaseMaterial{ nullptr };
    };
}