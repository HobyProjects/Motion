#pragma once

#include <filesystem>
#include <memory>

#include "Asset.hpp"

namespace Motion::Core
{
    using TextureID = uint32_t;

    enum class TextureType : uint32_t
    {
        // Legacy Texture Types
        DiffuseTexture = 0,
        AmbientTexture,
        SpecularTexture,
        EmissiveTexture,
        NormalMapsTexture,
        HeightMaps,
        ShininessTexture,
        OpacityMapsTexture,
        LightMapsTexture,

        // PBR Texture Type
        BaseColorMapsTexture,
        MetallicMapsTexture,
        RoughnessMapsTexture,
        AOMapsTexture,
        EmissiveMapsTexture,
        ClearCoatMapsTexture,
        SheenMapsTexture,
        TransmissionMapsTexture,
        UnknownTextureType,

        // Cube Maps
        CubeMapTexture
    };

    inline uint32_t operator|(TextureType a, TextureType b) { return static_cast<uint32_t>(a) | static_cast<uint32_t>(b); }
    inline uint32_t operator&(TextureType a, TextureType b) { return static_cast<uint32_t>(a) & static_cast<uint32_t>(b); }
    inline uint32_t operator^(TextureType a, TextureType b) { return static_cast<uint32_t>(a) ^ static_cast<uint32_t>(b); }
    inline uint32_t operator~(TextureType a) { return ~static_cast<uint32_t>(a); }

    struct TextureSpecification
    {
        std::string Name{};
        uint8_t* TextureData{nullptr};
        int32_t Width{0}, Height{0}, NumberOfChannels{0};
        uint32_t InternalDataFormat{0}, TextureDataFormat{0}, TexID{0};
        TextureType Type {TextureType::BaseColorMapsTexture};
        static uint32_t GlobalAnisotropyLevel; 
    };

    class ITexture : public IAsset
    {
        public:
            ITexture() = default;
            virtual ~ITexture() = default;

            virtual void Bind() const = 0;
            virtual void Bind(uint32_t bindingPoint) const = 0;
            virtual void Unbind() const = 0;

            virtual TextureID GetID() const = 0;
            virtual TextureSpecification GetSpecification() const = 0;
            virtual bool IsFromFile() const = 0;

            virtual void SetGlobalAnisotropy(uint32_t level) const = 0;
            virtual uint32_t GetGlobalAnisotropy() const  = 0;

        protected:
            virtual bool LoadTextureFromFile(const std::filesystem::path& textureFile, bool flip) = 0;
            virtual bool GenerateTexture2D(uint32_t width, uint32_t height) = 0;
    };

    class ITextureCubeMap : public ITexture
    {
        public:
            ITextureCubeMap() = default;
            virtual ~ITextureCubeMap() = default;

            virtual void SetFace(uint32_t face, int mipLevel, uint32_t width, uint32_t height, uint32_t format, const void* data) = 0;
    };

    class TextureManager
    {
        private:
            TextureManager() = default;
            ~TextureManager() = default;

            TextureManager(const TextureManager&) = delete;
            TextureManager& operator=(const TextureManager&) = delete;
            TextureManager(TextureManager&&) = delete;
            TextureManager& operator=(TextureManager&&) = delete;

        public:
            static void InsertTexture(const UUID& uuid, const std::shared_ptr<ITexture>& texture);
            static std::shared_ptr<ITexture> GetTexture(const UUID& uuid);
            static std::shared_ptr<ITexture> GetTexture(const std::string& name);
            static std::unordered_map<UUID, std::shared_ptr<ITexture>>::const_iterator GetTextures();
    };
}