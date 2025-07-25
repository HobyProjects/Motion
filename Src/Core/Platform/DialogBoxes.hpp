#pragma once

#include <filesystem>
#include <string>
#include <optional>

#include "base.hpp"
#include "Window.hpp"

namespace Motion
{
    class DialogBoxes
    {
    private:
        DialogBoxes() = default;
        ~DialogBoxes() = default;

        DialogBoxes(const DialogBoxes&) = delete;
        DialogBoxes& operator=(const DialogBoxes&) = delete;
        DialogBoxes(DialogBoxes&&) = delete;
        DialogBoxes& operator=(DialogBoxes&&) = delete;

    public:
        static std::optional<std::string> OpenFileDialog(const std::wstring& filter = L"All Files\0*.*\0", const std::wstring& title = L"Open File");
        static std::optional<std::string> SaveFileDialog(const std::wstring& filter = L"All Files\0*.*\0", const std::wstring& title = L"Save File");
    };
}