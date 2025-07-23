#include "CorePCH.hpp"

namespace Motion
{
    /**
     * @brief Constructs a StaticMesh asset with the specified UUID, name, and model file path.
     *
     * Initializes the StaticMesh by assigning the provided UUID, setting its name, asset type,
     * and the source file path. Also sets up the asset metadata, including the source path
     * and initialization state.
     *
     * @param uuid The unique identifier for the StaticMesh asset.
     * @param name The name to assign to the StaticMesh asset.
     * @param modelFile The filesystem path to the model file associated with this asset.
     */
    StaticMesh::StaticMesh(UUID uuid, const std::string& name, const std::filesystem::path& modelFile) :
        AssetBase<IAsset>(uuid, name, AssetType::StaticMesh, modelFile.string())
    {
        AssetInfo.AssetSource = modelFile.string();
        AssetInfo.IsInitialized = false;
    }

}


