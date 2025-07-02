#include "CorePCH.hpp"
#include "Thumbnail.hpp"

namespace Motion::Core
{
    ModelThumbnail::ModelThumbnail(const std::string& name, uint32_t width, uint32_t height, const std::shared_ptr<Model::SubMesh>& model) 
    {
        FrameBufferSpecification spec;
        spec.Width = width;
        spec.Height = height;
        spec.SwapChainTarget = false;

        m_FrameBuffer = BuffersBuilder::CreateFrameBuffer(spec);

        m_Camera.ViewportWidth = static_cast<float>(width);
        m_Camera.ViewportHeight = static_cast<float>(height);
        m_Camera.AspectRatio = width / static_cast<float>(height);
        m_Camera.Position = glm::vec3(0.0f, 0.0f, -5.0f);
        m_Camera.RefreshCameraMatrix();

        CreateThumbnail();
    }

    void ModelThumbnail::CreateThumbnail() 
    {
        if (!m_Mesh || !m_Mesh->MeshPtr)
            return;

        m_FrameBuffer->Bind();

        Renderer::SetViewport(0, 0, (int)m_Camera.ViewportWidth, (int)m_Camera.ViewportHeight);
        Renderer::ClearColor(glm::vec4(0.1f, 0.1f, 0.1f, 1.0f));
        Renderer::Clear();
        m_Camera.RefreshCameraMatrix();

        Material::ShadingMethod shadingMethod = Material::ShadingMethod::Unknown;
        std::weak_ptr<Model::SubMeshMaterial> mat = m_Mesh->ParentModel->GetSubMeshMaterial(m_Mesh->MaterialIndex);
        if(!mat.expired())
        {
            auto material = mat.lock();
            shadingMethod = material->Materials->GetShadingMethod();
        }

        std::weak_ptr<IShader> shader;
        switch(shadingMethod)
        {
            case Material::ShadingMethod::Phong:
                shader = AssetManager::GetShader("PhongShader");
                break;
            case Material::ShadingMethod::PBR:
                shader = AssetManager::GetShader("PBRShader");
                break;
            case Material::ShadingMethod::Unlit:
                shader = AssetManager::GetShader("UnlitShader");
                break;
            default:
                shader = AssetManager::GetShader("DefaultShader");
                break;
        };

        if (shader.expired())
        {
            MOTION_CORE_ERROR("Shader not found for shading method: {0}", static_cast<int>(shadingMethod));
            return;
        }

        DrawCommand command;
        command.Shader = shader.lock();
        command.SubMesh = m_Mesh->MeshPtr;
        command.MeshMaterial = mat.lock()->Materials;
        command.RendererPasses = RenderPass::Opaque;
        command.ModelTransform = glm::mat4(1.0f);
        command.CameraMatrix = m_Camera.GetCameraMatrix();

        Renderer::BeginFrame();
        Renderer::Submit(command);
        Renderer::EndFrame();

        m_FrameBuffer->Unbind();
    }

    uint32_t ModelThumbnail::GetColorAttachment() const 
    {
        if (m_FrameBuffer)
        {
            return m_FrameBuffer->GetColorAttachment();
        }

        MOTION_CORE_ERROR("FrameBuffer is not initialized.");
        return 0;   
    }

    MaterialThumbnail::MaterialThumbnail(const std::string& name, uint32_t width, uint32_t height, const std::shared_ptr<Model::SubMeshMaterial>& material)
    {
        FrameBufferSpecification spec;
        spec.Width = width;
        spec.Height = height;
        spec.SwapChainTarget = false;

        m_FrameBuffer = BuffersBuilder::CreateFrameBuffer(spec);
        m_Material = material;

        m_Camera.ViewportWidth = static_cast<float>(width);
        m_Camera.ViewportHeight = static_cast<float>(height);
        m_Camera.AspectRatio = width / static_cast<float>(height);
        m_Camera.Position = glm::vec3(0.0f, 0.0f, -5.0f);
        m_Camera.RefreshCameraMatrix();

        CreateThumbnail();
    }

    uint32_t MaterialThumbnail::GetColorAttachment() const 
    {
        if (m_FrameBuffer)
        {
            return m_FrameBuffer->GetColorAttachment();
        }

        MOTION_CORE_ERROR("FrameBuffer is not initialized.");
        return 0;
    }

    void MaterialThumbnail::CreateThumbnail() 
    {
        if (!m_Material || !m_Material->Materials)
            return;

        m_FrameBuffer->Bind();

        Renderer::SetViewport(0, 0, (int)m_Camera.ViewportWidth, (int)m_Camera.ViewportHeight);
        Renderer::ClearColor(glm::vec4(0.1f, 0.1f, 0.1f, 1.0f));
        Renderer::Clear();
        m_Camera.RefreshCameraMatrix();

        DrawCommand command;
        command.Shader = AssetManager::GetShader("MaterialShader");
        command.SubMesh = Mesh::CreateCube(1.0f, 1.0f, 1.0f); // Create a simple cube mesh for the thumbnail
        command.MeshMaterial = m_Material->Materials;
        command.RendererPasses = RenderPass::Opaque;
        command.ModelTransform = glm::mat4(1.0f);
        command.CameraMatrix = m_Camera.GetCameraMatrix();

        Renderer::BeginFrame();
        Renderer::Submit(command);
        Renderer::EndFrame();

        m_FrameBuffer->Unbind();
    }
}