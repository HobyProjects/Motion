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
        static std::string OpenFileDialog();
        static std::string SaveFileDialog();
    };
}