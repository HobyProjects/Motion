#include "CorePCH.hpp"
#include "Model.hpp"

namespace Motion
{
    StaticMesh::StaticMesh(UUID uuid, const std::string& name, const std::filesystem::path& modelFile) :
        AssetBase<IAsset>(uuid, name, AssetType::StaticMesh, modelFile.string())
    {
        AssetInfo.AssetSource = modelFile.string();
        AssetInfo.IsInitialized = false;
    }

    void StaticMesh::Render() const
    {
        for (const auto& mesh : m_Meshes)
        {
            if (mesh)
            {
                mesh->Bind();

                mesh->Render();

                mesh->Unbind();
            }
        }
    }
}


