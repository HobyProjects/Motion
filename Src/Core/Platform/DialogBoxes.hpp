#pragma once

#include <filesystem>
#include <string>

#include "base.hpp"
#include "Window.hpp"

namespace Motion::Core
{
    class DialogBoxes
    {
        public:
            enum class FileType : uint32_t
            {
                AllFiles = 0,
                TextFile,
                TextureFile,
                ModelFile,
                ShaderFile,
                FontFile,
                AudioFile,
                VideoFile,
                ImageFile
            };

        public:
            static std::filesystem::path OpenFileDialog(NativeWindow window, const std::string& caption, FileType fileType, const std::filesystem::path& defaultPath = std::filesystem::current_path());
            static std::filesystem::path SaveFileDialog(NativeWindow window, const std::string& caption, FileType fileType, const std::filesystem::path& defaultPath = std::filesystem::current_path());
    };
}