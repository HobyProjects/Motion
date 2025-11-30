#include "CorePCH.hpp"

namespace Motion
{
    bool TextureSlotUtils::LoadTextureFromDialog(std::shared_ptr<ITexture>& texture, TextureType type)
    {
        DialogBoxes::InitializeCOM();

        OpenDialogOptions options{};
        options.Title = L"Import Texture";
        options.DefaultExtension = L"png";
        options.AllowMultiSelect = false;
        options.InitialDirectory = std::filesystem::current_path();
        options.Filters = 
        {
            {L"Image Files", L"*.jpg;*.jpeg;*.png;*.bmp;*.tga;*.hdr"},
            {L"All Files",   L"*.*"}
        };

        std::filesystem::path path = DialogBoxes::OpenFileDialog(options);
        DialogBoxes::UninitializeCOM();
        
        if (!path.empty())
        {
            if (!texture)
                texture = ITexture::Create(path, type);
            else
                texture->ReloadFromFile(path, type);
            
            return true;
        }
        
        return false;
    }
}