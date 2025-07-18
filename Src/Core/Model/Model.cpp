#include "CorePCH.hpp"
#include "Model.hpp"

namespace Motion::Core
{
    StaticMesh::StaticMesh(const std::string& name, const std::filesystem::path& modelFile) :
        AssetBase<IAsset>(UniqueIdentity::GetUniqueID(), name, AssetType::StaticMesh, modelFile.string())
    {
        m_MetaData.AssetSource = modelFile.string();
        m_MetaData.IsAssetInitialized = false;
    }

    StaticMesh::StaticMesh(UUID uuid, const std::string& name, const std::filesystem::path& modelFile) :
        AssetBase<IAsset>(uuid, name, AssetType::StaticMesh, modelFile.string())
    {
        m_MetaData.AssetSource = modelFile.string();
        m_MetaData.IsAssetInitialized = false;
    }

    void StaticMesh::Render(const glm::mat4& modelTransForm, const glm::mat4& cameraMatrix)
    {
        for (const auto& segments : m_Meshes)
        {
            if (segments->MeshSelf->IsAssetInitialized())
            {
                DrawCommand drawCommand;
                drawCommand.SortKey = UniqueIdentity::GetUniqueID();
                drawCommand.MaterialID = segments->Materials->GetUUID();
                drawCommand.MeshID = segments->MeshSelf->GetUUID();
                drawCommand.ModelMatrix = modelTransForm;
                drawCommand.ViewProjectionMatrix = cameraMatrix;

                auto& renderer = Renderer::GetInstance();
                renderer.Submit(drawCommand);
            }
        }
    }


}


