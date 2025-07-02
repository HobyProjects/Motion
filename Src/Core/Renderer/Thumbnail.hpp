#pragma once

#include "Renderer.hpp"
#include "Buffers.hpp"
#include "Camera3D.hpp"
#include "Model.hpp"

namespace Motion::Core
{
    class ModelThumbnail
    {
        public:
            ModelThumbnail() = default;
            ModelThumbnail(const std::string& name,uint32_t width, uint32_t height, const std::shared_ptr<Model::SubMesh>& model);
            ~ModelThumbnail() = default;

            uint32_t GetColorAttachment() const;

        private:
            void CreateThumbnail();

        private:
            std::shared_ptr<Model::SubMesh> m_Mesh{nullptr};
            std::shared_ptr<IFrameBuffer> m_FrameBuffer{nullptr};
            Camera3D m_Camera;
    };

    class MaterialThumbnail
    {
        public:
            MaterialThumbnail() = default;
            MaterialThumbnail(const std::string& name, uint32_t width, uint32_t height, const std::shared_ptr<Model::SubMeshMaterial>& material);
            ~MaterialThumbnail() = default;

            uint32_t GetColorAttachment() const;

        private:
            void CreateThumbnail();

        private:
            std::shared_ptr<Model> m_Mesh{nullptr};
            std::shared_ptr<Model::SubMeshMaterial> m_Material{nullptr};
            std::shared_ptr<IFrameBuffer> m_FrameBuffer{nullptr};
            Camera3D m_Camera;
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
                std::unique_ptr<MaterialThumbnail> MaterialThumb;
            };

            std::unordered_map<UUID, ThumbnailEntry> m_Cache;
            const uint32_t m_ThumbWidth = 128;
            const uint32_t m_ThumbHeight = 128;
    };
}