#pragma once

#include "UUID.hpp"
#include "Asset.hpp"
#include "Buffers.hpp"
#include "Arrays.hpp"
#include "Shaders.hpp"

namespace Motion::Core
{
    class StaticMesh; // Forward declaration

    struct Vertex
    {
        glm::vec3 Position{ 0.0f, 0.0f, 0.0f };
        glm::vec2 TexCoord{ 0.0f, 0.0f };
        glm::vec3 Normal{ 0.0f, 0.0f, 0.0f };
        glm::vec3 Tangent{ 0.0f, 0.0f, 0.0f };
        glm::vec3 Bitangent{ 0.0f, 0.0f, 0.0f };
    };

    class Mesh : public AssetBase<IAsset>
    {
    public:
        Mesh() = default;
        Mesh(UUID uuid, const std::string& name, float* vertices, std::uint32_t verticesSize, std::uint32_t* indices, std::uint32_t indicesCount, const BufferLayout& layout, const std::shared_ptr<StaticMesh>& parentModel);
        ~Mesh() = default;

        void Bind() const noexcept;
        void Unbind() const noexcept;
        void Render();
        std::uint32_t GetIndicesCount() const noexcept;
        std::shared_ptr<StaticMesh> GetParentModel() const noexcept;

    private:
        std::shared_ptr<IVertexBuffer> m_VertexBuffer{ nullptr };
        std::shared_ptr<IElementBuffer> m_ElementBuffer{ nullptr };
        std::shared_ptr<IVertexArray> m_VertexArray{ nullptr };
        std::shared_ptr<StaticMesh> m_ParentModel{ nullptr };
        uint32_t m_IndicesCount{ 0 };

        friend class StaticMesh; // Allow StaticMesh to access private members
    };

    class QuickMesh
    {
    public:
        QuickMesh() = default;
        ~QuickMesh() = default;

        QuickMesh(const QuickMesh&) = delete;
        QuickMesh& operator=(const QuickMesh&) = delete;
        QuickMesh(QuickMesh&&) = delete;
        QuickMesh& operator=(QuickMesh&&) = delete;

        static std::shared_ptr<Mesh> CreatePlane(bool isRegistered, const std::string name, float width, float height, std::uint32_t widthSegments = 1, std::uint32_t heightSegments = 1);
        static std::shared_ptr<Mesh> CreateCube(bool isRegistered, const std::string name, float width, float height, float depth);
        static std::shared_ptr<Mesh> CreateSphere(bool isRegistered, const std::string name, std::uint32_t sectorCount, std::uint32_t stackCount);
        static std::shared_ptr<Mesh> CreateQuad(bool isRegistered, const std::string name, std::uint32_t width, std::uint32_t height);
    };
}