#include "CorePCH.hpp"
#include "Material.hpp"
namespace Motion
{
    /**
     * @brief Constructs a Material object with the specified UUID, name, and shading method.
     *
     * @param uuid The universally unique identifier for the material.
     * @param name The name of the material.
     * @param shadingMethod The shading method used by the material.
     */
    Material::Material(const UUID& uuid, const std::string& name) :
        AssetBase(uuid, name, AssetType::Material, "Undefined")
    {
        auto& assetManager = AssetManager::GetInstance();
        m_Shader = assetManager.Get<IShader>("PBRShader");
        m_ShaderBuffer = BufferFactory::CreateShaderBuffer(static_cast<std::uint32_t>(sizeof(MaterialLayerData) * MAX_MATERIAL_LAYERS), 0);
        AssetInfo.IsInitialized = true;
    }

    // Offset values for the material buffer
    enum MATERIAL_BUFFER_OFFSET : std::uint32_t
    {
        BASE_COLOR_FACTOR = 0,
        METALLIC_FACTOR = 16,
        ROUGHNESS_FACTOR = 20,
        OPACITY = 24,
        AO_FACTOR = 28,
        CLEAR_COAT_FACTOR = 32,
        CLEAR_COAT_ROUGHNESS = 36,
        SHEEN_FACTOR = 40,
        SHEEN_ROUGHNESS = 44,
        TRANSMISSION = 48,
        IOR = 52,
        BLEND_FACTOR = 56
    };

    // Size of various data types in bytes
    enum MATERIAL_SIZE : std::uint32_t
    {
        VEC3 = sizeof(glm::vec3),
        VEC4 = sizeof(glm::vec4),
        FLOAT = sizeof(float),
    };

    // Size of a single material layer in bytes
    constexpr std::uint32_t LAYER_SIZE = sizeof(MaterialLayerData);

    /**
     * @brief Binds the material and its associated textures.
     *
     * This function binds the shader buffer and sets the uniform values for each material layer.
     * It also binds the textures associated with each layer to the appropriate binding points.
     */
    void Material::Bind() noexcept
    {
        m_ShaderBuffer->Bind();

        for (std::uint32_t i = 0; i < MAX_MATERIAL_LAYERS; i++)
        {
            const auto& layer = m_Layers[i];
            std::uint32_t baseOffset = i * LAYER_SIZE;

            m_ShaderBuffer->SetBufferData(baseOffset + BASE_COLOR_FACTOR, VEC3, layer.Data.BaseColor);
            m_ShaderBuffer->SetBufferData(baseOffset + METALLIC_FACTOR, FLOAT, layer.Data.Metallic);
            m_ShaderBuffer->SetBufferData(baseOffset + ROUGHNESS_FACTOR, FLOAT, layer.Data.Roughness);
            m_ShaderBuffer->SetBufferData(baseOffset + OPACITY, FLOAT, layer.Data.Opacity);
            m_ShaderBuffer->SetBufferData(baseOffset + AO_FACTOR, FLOAT, layer.Data.AmbientOcclusion);
            m_ShaderBuffer->SetBufferData(baseOffset + CLEAR_COAT_FACTOR, FLOAT, layer.Data.ClearCoat);
            m_ShaderBuffer->SetBufferData(baseOffset + CLEAR_COAT_ROUGHNESS, FLOAT, layer.Data.ClearCoatRoughness);
            m_ShaderBuffer->SetBufferData(baseOffset + SHEEN_FACTOR, FLOAT, layer.Data.Sheen);
            m_ShaderBuffer->SetBufferData(baseOffset + SHEEN_ROUGHNESS, FLOAT, layer.Data.SheenRoughness);
            m_ShaderBuffer->SetBufferData(baseOffset + TRANSMISSION, FLOAT, layer.Data.Transmission);
            m_ShaderBuffer->SetBufferData(baseOffset + IOR, FLOAT, layer.Data.IOR);
            m_ShaderBuffer->SetBufferData(baseOffset + BLEND_FACTOR, FLOAT, layer.Data.Blend);

            auto BindTexture =
                [&](std::string_view uniformBase)
                {
                    auto it = layer.Textures.find(uniformBase);
                    if (it != layer.Textures.end() && it->second)
                    {
                        std::uint32_t binding = TextureBinding::Point();
                        it->second->Bind(binding);
                        std::string uniformName = std::format("{}[{}]", uniformBase, i);
                        m_Shader->SetUniform(uniformName, static_cast<std::int32_t>(binding));
                    }
                };

            BindTexture(UniformCache::BaseColorTextures);
            BindTexture(UniformCache::MetallicTextures);
            BindTexture(UniformCache::RoughnessTextures);
            BindTexture(UniformCache::AmbientOcclusionTextures);
            BindTexture(UniformCache::NormalTextures);
            BindTexture(UniformCache::OpacityTextures);
            BindTexture(UniformCache::BlendMaskTextures);
        }
    }

    /**
     * @brief Unbinds the material and its associated textures.
     *
     * This function unbinds the shader buffer and all textures associated with the material layers.
     * It ensures that the GPU resources are released and no longer used in rendering.
     */
    void Material::Unbind() noexcept
    {
        m_ShaderBuffer->Unbind();
        for (std::uint32_t i = 0; i < MAX_MATERIAL_LAYERS; i++)
        {
            const auto& layer = m_Layers[i];
            for (const auto& [textureName, texture] : layer.Textures)
            {
                if (texture)
                {
                    texture->Unbind();
                }
            }
        }
    }

    /**
     * @brief Inserts a new material layer into the material.
     *
     * This function adds a new material layer to the material's layer array. It checks for an empty slot
     * and inserts the layer if found. If no empty slot is available, it asserts an error.
     *
     * @param layer The MaterialLayer to be inserted.
     */
    void Material::InsertLayer(const MaterialLayer& layer) noexcept
    {
        for (std::size_t i = 0; i < MAX_MATERIAL_LAYERS; ++i)
        {
            // Check if the slot is unused by testing a property (e.g., Blend == default)
            if (m_Layers[i].Data.Blend == MaterialDefaultValues::Blend &&
                m_Layers[i].Textures.empty())
            {
                m_Layers[i] = layer;
                return;
            }
        }
        MOTION_ASSERT(false, "Maximum number of material layers exceeded!");
    }

    /**
     * @brief Inserts a texture into a specific layer of the material.
     *
     * This function adds a texture to the specified layer of the material. If the texture already exists
     * in that layer, it overwrites the existing texture.
     *
     * @param layerIndex The index of the layer where the texture will be inserted.
     * @param textureName The name of the texture to be inserted.
     * @param texture The shared pointer to the ITexture to be inserted.
     */
    void Material::InsertTexture(std::uint32_t layerIndex, std::string_view textureName, const std::shared_ptr<ITexture>& texture) noexcept
    {
        auto& layer = m_Layers[layerIndex];
        if (layer.Textures.find(textureName) != layer.Textures.end())
        {
            MOTION_CORE_WARN("Texture {0} already exists in layer {1}. Overwriting.", textureName, layerIndex);
        }

        layer.Textures[textureName] = std::move(texture);
    }


    /**
     * @brief Retrieves a material layer by index.
     *
     * This function provides access to a specific material layer by its index. It asserts that the index
     * is within bounds and returns a reference to the requested layer.
     *
     * @param index The index of the material layer to retrieve.
     * @return MaterialLayer& Reference to the requested material layer.
     */
    MaterialLayer& Material::GetLayer(std::uint32_t index) noexcept
    {
        MOTION_ASSERT(index < MAX_MATERIAL_LAYERS, "Index out of bounds for material layers.");
        return m_Layers[index];
    }


}


