#pragma once

#include "UUID.hpp"

namespace Motion
{
    enum class AssetType : std::uint32_t
    {
        None = 0,
        Shader,
        Texture,
        Material,
        Mesh,
        StaticMesh,
    };

    struct AssetProperties
    {
        UUID AssetIdentifier{ 0 };
        std::string AssetName{ "Unnamed Asset" };
        std::string AssetSource{ "Unknown" };
        AssetType Type{ AssetType::None };
        bool IsInitialized{ false };

        AssetProperties() = default;
        ~AssetProperties() = default;
    };

    class IAsset
    {
    public:
        IAsset() = default;
        virtual ~IAsset() = default;

        [[nodiscard]] virtual AssetProperties& GetMetaData() = 0;
        [[nodiscard]] virtual AssetType GetType() const = 0;
        [[nodiscard]] virtual UUID GetUUID() const = 0;
        [[nodiscard]] virtual std::string GetName() = 0;
        [[nodiscard]] virtual std::string GetSource() const = 0;
        [[nodiscard]] virtual bool IsInitialized() const = 0;
    };

    template<typename T>
    class AssetBase : public T
    {
    public:
        /**
         * @brief Constructs an AssetBase with the given UUID, name, type, and file path.
         *
         * This constructor initializes the asset's metadata, including its unique identifier,
         * name, type, source file path, and initialization status.
         *
         * @param uuid The unique identifier for the asset.
         * @param name The name of the asset.
         * @param type The type of the asset (e.g., Shader, Texture).
         * @param filePath The filesystem path to the asset's source file.
         */
        AssetBase(UUID uuid, const std::string& name, AssetType type, const std::string& filePath)
        {
            AssetInfo.AssetIdentifier = uuid;
            AssetInfo.AssetName = name;
            AssetInfo.Type = type;
            AssetInfo.AssetSource = filePath;
            AssetInfo.IsInitialized = false;
        }

        virtual ~AssetBase() = default;


        /**
         * @brief Returns the metadata of the asset.
         *
         * This function provides access to the asset's metadata, including its UUID, name, type, source,
         * and initialization status. It allows for introspection of the asset's properties.
         *
         * @return AssetProperties& Reference to the asset's metadata.
         */
        [[nodiscard]] virtual AssetProperties& GetMetaData() override { return AssetInfo; }

        /**
         * @brief Returns the type of the asset.
         *
         * This function retrieves the type of the asset, which is defined by the AssetType enum.
         * It allows for type checking and categorization of assets.
         *
         * @return AssetType The type of the asset.
         */
        [[nodiscard]] virtual AssetType GetType() const override { return AssetInfo.Type; }

        /**
         * @brief Returns the unique identifier (UUID) of the asset.
         *
         * This function provides the UUID associated with the asset, which is used for identification
         * and management of assets within the system.
         *
         * @return UUID The unique identifier of the asset.
         */
        [[nodiscard]] virtual UUID GetUUID() const override { return AssetInfo.AssetIdentifier; }

        /**
         * @brief Returns the name of the asset.
         *
         * This function retrieves the name assigned to the asset, which is used for identification
         * and display purposes within the application.
         *
         * @return std::string to the asset's name.
         */
        [[nodiscard]] virtual std::string GetName() override { return AssetInfo.AssetName; }

        /**
         * @brief Returns the source file path of the asset.
         *
         * This function provides the filesystem path to the asset's source file, which can be used
         * for loading or referencing the asset in the application.
         *
         * @return std::string to the asset's source file path.
         */
        [[nodiscard]] virtual std::string GetSource() const override { return AssetInfo.AssetSource; }

        /**
         * @brief Checks if the asset has been initialized.
         *
         * This function returns a boolean indicating whether the asset has been successfully initialized
         * and is ready for use within the application.
         *
         * @return bool True if the asset is initialized, false otherwise.
         */
        [[nodiscard]] virtual bool IsInitialized() const override { return AssetInfo.IsInitialized; }

    protected:
        AssetProperties AssetInfo{};
    };

    /**
     * @brief Concept to check if a type is derived from IAsset.
     *
     * This concept ensures that the type T is derived from the IAsset interface,
     * allowing for type-safe operations on asset-related classes.
     */
    template<typename T>
    concept AssetExpected = std::derived_from<T, IAsset>;

    /**
     * @brief Builder for creating asset backends.
     *
     * This struct is used to define the expected type of asset backend that can be built.
     * It is specialized for types that meet the AssetExpected concept.
     *
     * @tparam T The type of asset backend to be built, which must derive from IAsset.
     */
    template<AssetExpected T>
    struct AssetBackendsBuilder;
}