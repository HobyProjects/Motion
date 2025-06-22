#pragma once

#include "Layer.hpp"
#include "Window.hpp"
#include "UI.hpp"

namespace Motion::App
{
    enum class ImGuiColorScheme
    {
        Light,
        Dark,
    };

    class ImGuiLayer final : public Motion::Core::Layer
    {
        public:
            ImGuiLayer() : Motion::Core::Layer("ImGuiLayer") {}
    }

}