#pragma once

#include "UUID.hpp"

namespace Motion::Core
{
    enum class AssetType : std::uint32_t
    {
        None = 0,
        Shader,
        Texture,
        Material,
        Model,
    };

    struct AssetMetaData
    {
        UUID AssetID{ 0 };
        std::string AssetName{ "Unnamed Asset" };
        std::string AssetSource{ "Unknown" };
        AssetType Type = AssetType::None;
        bool IsAssetInitialized = false;
    };

    class IAsset
    {
    public:
        IAsset() = default;
        virtual ~IAsset() = default;

        virtual AssetMetaData& GetMetaData() = 0;
        virtual AssetType GetType() const = 0;
        virtual UUID GetUUID() const = 0;
        virtual std::string& GetName() = 0;
        virtual std::string& GetSource() const = 0;
        virtual bool IsAssetInitialized() const = 0;
    };

    template<typename T>
    class AssetBase : public T
    {
    public:
        AssetBase(UUID uuid, const std::string& name, AssetType type, const std::string& filePath)
        {
            m_MetaData.AssetID = uuid;
            m_MetaData.AssetName = name;
            m_MetaData.Type = type;
            m_MetaData.AssetSource = filePath;
            m_MetaData.IsAssetInitialized = false;
        }

        virtual ~AssetBase() = default;

        virtual AssetMetaData& GetMetaData() override { return m_MetaData; }
        virtual AssetType GetType() const override { return m_MetaData.Type; }
        virtual UUID GetUUID() const override { return m_MetaData.AssetID; }
        virtual std::string& GetName() override { return m_MetaData.AssetName; }
        virtual std::string& GetSource() const override { return m_MetaData.AssetSource; }
        virtual bool IsAssetInitialized() const override { return m_MetaData.IsAssetInitialized; }

    protected:
        AssetMetaData m_MetaData{};
    };

    template<typename T>
    concept AssetExpected = std::derived_from<T, IAsset>;

    template<AssetExpected T>
    struct AssetBackendsBuilder;
}