#pragma once

#include <string>
#include <string_view>
#include <memory>
#include <unordered_map>
#include <vector>
#include <glm/glm.hpp>
#include <entt/entt.hpp>

#include "Texture.hpp"
#include "Shaders.hpp"
#include "Buffers.hpp"
#include "Asset.hpp"

namespace Motion
{
    struct BaseMaterial : public AssetBase<IAsset>
    {
        glm::vec3 BaseColor{ 1.0f, 1.0f, 1.0f };
        float MetallicFactor{ 0.0f };
        float RoughnessFactor{ 0.0f };
        float OpacityFactor{ 1.0f };

        std::unordered_map<TextureType, std::shared_ptr<ITexture>> Textures{};

        BaseMaterial(UUID uniqueID, const std::string& materialName, const std::filesystem::path& materialFile);
        ~BaseMaterial() = default;

        static void Import(const std::filesystem::path& materialYAML);
    };

    using MaterialHandle = entt::entity;
    class Material;

    class MaterialBuilder
    {
    private:
        MaterialBuilder() = default;
        ~MaterialBuilder() = default;

        MaterialBuilder(const MaterialBuilder&) = delete;
        MaterialBuilder& operator=(const MaterialBuilder&) = delete;
        MaterialBuilder(MaterialBuilder&&) = delete;
        MaterialBuilder& operator=(MaterialBuilder&&) = delete;

    public:
        static MaterialBuilder& GetInstance()
        {
            static MaterialBuilder instance;
            return instance;
        }

    public:
        [[nodiscard]] std::shared_ptr<Material> Create(const std::shared_ptr<BaseMaterial>& baseMaterial);
        void Destroy(const std::shared_ptr<Material>& material);

    private:
        entt::registry _Registry;
        friend class Material;
    };

    class Material
    {
    public:
        Material() = default;
        Material(MaterialHandle handle, const std::shared_ptr<BaseMaterial>& baseMaterial) :
            _Handle(handle), _BaseMaterial(baseMaterial), _IsAlive(true) {
        }
        ~Material() = default;

        template<typename T>
        bool HasTexture() const
        {
            if (_IsAlive)
            {
                auto& materialFactory = MaterialBuilder::GetInstance();
                return materialFactory._Registry.any_of<T>(_Handle);
            }
            return false;
        }

        template<typename T, typename... Args>
        T& AddTexture(Args&&... args)
        {
            MOTION_ASSERT(!HasTexture<T>(), "Material already has this texture type!");
            if (_IsAlive)
            {
                auto& materialFactory = MaterialBuilder::GetInstance();
                return materialFactory._Registry.emplace<T>(_Handle, std::forward<Args>(args)...);
            }

            MOTION_ASSERT(false, "Material is not alive!");
            static T dummy{};
            return dummy;
        }

        template<typename T>
        T& GetTexture() const
        {
            MOTION_ASSERT(HasTexture<T>(), "Material does not have this texture type!");
            if (_IsAlive)
            {
                auto& materialFactory = MaterialBuilder::GetInstance();
                return materialFactory._Registry.get<T>(_Handle);
            }

            static T dummy{};
            return dummy;
        }

        std::shared_ptr<BaseMaterial> GetBaseMaterial() const
        {
            return _BaseMaterial;
        }

        template<typename T>
        void RemoveTexture()
        {
            MOTION_ASSERT(HasTexture<T>(), "Material does not have this texture type!");

            if (_IsAlive)
            {
                auto& materialFactory = MaterialBuilder::GetInstance();
                materialFactory._Registry.remove<T>(_Handle);
                return;
            }

            MOTION_ASSERT(false, "Material is not alive!");
        }

        void Destroy()
        {
            auto& materialFactory = MaterialBuilder::GetInstance();
            materialFactory._Registry.destroy(_Handle);
            _Handle = entt::null;
            _IsAlive = false;
        }

        [[nodiscard]] bool IsAlive() const { return _IsAlive; }
        [[nodiscard]] MaterialHandle GetHandle() const { return _Handle; }
        [[nodiscard]] operator bool() const { return _Handle != entt::null; }
        [[nodiscard]] operator std::uint32_t() const { return static_cast<std::uint32_t>(_Handle); }
        [[nodiscard]] operator entt::entity() const { return _Handle; }
        [[nodiscard]] bool operator==(const Material& other) const { return _Handle == other._Handle; }
        [[nodiscard]] bool operator!=(const Material& other) const { return _Handle != other._Handle; }

    private:
        MaterialHandle _Handle{ entt::null };
        std::shared_ptr<BaseMaterial> _BaseMaterial{ nullptr };
        bool _IsAlive{ false };
    };
}