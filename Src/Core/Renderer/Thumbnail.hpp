#pragma once

#include "Asset.hpp"
#include "Model.hpp"

namespace Motion::Core
{
    class ThumbnailFactory; // Forward declaration

    struct ThumbnailSpecification
    {
        uint32_t Width{ 128 };
        uint32_t Height{ 128 };
        TextureID Attachment{ 0 };
    };

    class IThumbnail : public IAsset
    {
        public:
            IThumbnail() = default;
            virtual ~IThumbnail() = default;

            virtual TextureID GetAttachment() const = 0;
            virtual ThumbnailSpecification& GetSpecification() = 0;
    };

    class ModelThumbnail : public AssetBase<IThumbnail>
    {
        public:
            ModelThumbnail(const std::string& name, uint32_t width, uint32_t height, const std::shared_ptr<Model::SubMesh>& model);
            ~ModelThumbnail() = default;

            virtual TextureID GetAttachment() const override { return m_Specification.Attachment; }
            virtual ThumbnailSpecification& GetSpecification() override { return m_Specification; }

        private:
            ThumbnailSpecification m_Specification{};
            std::shared_ptr<Model> m_Model{nullptr};

            friend class ThumbnailFactory;
    };

    class TextureThumbnail : public AssetBase<IThumbnail>
    {
        public:
            TextureThumbnail(const std::string& name, uint32_t width, uint32_t height, const std::shared_ptr<Model::SubMeshMaterial>& material);
            ~TextureThumbnail() = default;

            virtual TextureID GetAttachment() const override { return m_Specification.Attachment; }
            virtual ThumbnailSpecification& GetSpecification() override { return m_Specification; }

        private:
            ThumbnailSpecification m_Specification;
            std::shared_ptr<ITexture> m_Texture{nullptr};
            std::shared_ptr<Mesh> m_Mesh{nullptr};

            friend class ThumbnailFactory;
    };

    class ThumbnailFactory
    {
        private:
            ThumbnailFactory();
            ~ThumbnailFactory() = default;

            ThumbnailFactory(const ThumbnailFactory&) = delete;
            ThumbnailFactory& operator=(const ThumbnailFactory&) = delete;
            ThumbnailFactory(ThumbnailFactory&&) = delete;
            ThumbnailFactory& operator=(ThumbnailFactory&&) = delete;

        public:
            static std::shared_ptr<IThumbnail> CreateThumbnail(const std::string& name, const std::shared_ptr<Model>& model, uint32_t width = 128, uint32_t height = 128);
            static std::shared_ptr<IThumbnail> CreateThumbnail(const std::string& name, const std::shared_ptr<ITexture>& texture, uint32_t width = 128, uint32_t height = 128);
            static void UpdateThumbnail(const std::shared_ptr<ModelThumbnail>& thumbnail, uint32_t width = 128, uint32_t height = 128);
            static void UpdateThumbnail(const std::shared_ptr<TextureThumbnail>& thumbnail, uint32_t width = 128, uint32_t height = 128);
    };

    class ThumbnailCache 
    {
        public:
            ThumbnailCache() = default;
            ~ThumbnailCache() = default;

            ImTextureID GetOrGenerateForMesh(UUID id, const std::shared_ptr<Model::SubMesh>& mesh);
            ImTextureID GetOrGenerateForMaterial(UUID id, const std::shared_ptr<Model::SubMeshMaterial>& material);
            void Invalidate(UUID id); // when an asset changes

        private:
            struct ThumbnailEntry {
                std::unique_ptr<ModelThumbnail> MeshThumb;
                std::unique_ptr<TextureThumbnail> MaterialThumb;
            };

            std::unordered_map<UUID, ThumbnailEntry> m_Cache;
            const uint32_t m_ThumbWidth = 128;
            const uint32_t m_ThumbHeight = 128;
    };
}