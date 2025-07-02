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
            static std::filesystem::path OpenFileDialog(NativeWindow window, const std::string& caption, const std::string& filter, const std::filesystem::path& defaultPath = std::filesystem::current_path());
            static std::filesystem::path SaveFileDialog(NativeWindow window, const std::string& caption, const std::string& filter, const std::filesystem::path& defaultPath = std::filesystem::current_path());
    };
}