#pragma once

#include "UUID.hpp"

namespace Motion::Core
{
    enum class AssetType
    {
        None,
        Texture,
        Shader,
        Material,
        Model,
    };

    struct AssetMetaData
    {
        UUID AssetUUID{UniqueIdentity::GetUniqueID()};
        std::string AssetName{"Unnamed Asset"};
        AssetType Type = AssetType::None;
        std::string FilePath{"Unknown"};
        bool IsLoaded = false;
    };

    class IAsset
    {
        public:
            IAsset() = default;
            virtual ~IAsset() = default;

            virtual const AssetMetaData& GetMetaData() const = 0;
            virtual AssetType GetType() const = 0;
    };

    template<typename T>
    class AssetBase : public T
    {
        public:
            AssetBase(const std::string& name, AssetType type, const std::string& filePath)
            {
                m_MetaData.AssetName = name;
                m_MetaData.Type = type;
                m_MetaData.FilePath = filePath;
                m_MetaData.IsLoaded = false;
            }

            virtual ~AssetBase() = default;

            virtual const AssetMetaData& GetMetaData() const override
            {
                return m_MetaData;
            }
            
            virtual AssetType GetType() const override
            {
                return m_MetaData.Type;
            }

        protected:
            AssetMetaData m_MetaData{};
    };

}