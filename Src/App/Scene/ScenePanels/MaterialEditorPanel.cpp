#include "CorePCH.hpp"
#include "MaterialEditorPanel.hpp"
#include <numeric>

namespace Motion
{
    static void UpdateTexture(UI::TextureAction action, std::shared_ptr<ITexture>& texture)
    {
        switch (action)
        {
        case UI::TextureAction::RequestUpload:
        {
            std::filesystem::path texturePath = DialogBoxes::OpenFileDialog();
            if (!texturePath.empty())
            {
                if (texture->ReloadFromFile(texturePath, texture->GetSpecification().Type))
                {
                    MOTION_CORE_INFO("Texture {} loaded successfully from {}", texture->GetSpecification().Name, texturePath.string());
                }
                else
                {
                    MOTION_CORE_ERROR("Failed to load texture {} from {}", texture->GetSpecification().Name, texturePath.string());
                }
            }
            break;
        }
        case UI::TextureAction::RequestReload:
        {
            std::filesystem::path texturePath = texture->GetSpecification().TextureFile;
            if (std::filesystem::exists(texturePath) && !texturePath.empty())
            {
                if (texture->ReloadFromFile(texturePath, texture->GetSpecification().Type))
                {
                    MOTION_CORE_INFO("Texture {} reloaded successfully from {}", texture->GetSpecification().Name, texturePath.string());
                }
                else
                {
                    MOTION_CORE_ERROR("Failed to reload texture {} from {}", texture->GetSpecification().Name, texturePath.string());
                }
            }
            break;
        }
        case UI::TextureAction::RequestClear:
        {
            texture.reset();
            texture = ITexture::Create(1, 1, { 1.0f, 1.0f, 1.0f });
            break;
        }
        }
    }


    void MaterialEditorPanel::RenderUI(ScenePanelContext& ctx)
    {

    }
}