#include "CorePCH.hpp"
#include "Model.hpp"

namespace Motion
{
    Model::Model(UUID uuid, const std::string& name, const std::filesystem::path& modelFile) :
        AssetBase<IAsset>(uuid, name, AssetType::Model, modelFile.string())
    {
        AssetInfo.AssetSource = modelFile.string();
        AssetInfo.IsInitialized = false;
    }

    void Model::Render() const
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


